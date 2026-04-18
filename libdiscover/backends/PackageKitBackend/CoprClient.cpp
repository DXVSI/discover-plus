#include "CoprClient.h"
#include "libdiscover_backend_packagekit_debug.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>

CoprClient::CoprClient(QObject *parent)
    : QObject(parent)
    , m_baseUrl(QStringLiteral("https://copr.fedorainfracloud.org/api_3"))
{
    m_fedoraVersion = getFedoraVersion();
    m_currentChroot = getCurrentChroot();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprClient initialized. Fedora version:" << m_fedoraVersion << "Chroot:" << m_currentChroot;
}

CoprClient::~CoprClient()
{
}

QString CoprClient::getFedoraVersion() const
{
    QFile file(QStringLiteral("/etc/os-release"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(QStringLiteral("VERSION_ID="))) {
                QString version = line.section(QLatin1Char('='), 1).remove(QLatin1Char('"'));
                return version;
            }
        }
    }
    return QStringLiteral("43");
}

QString CoprClient::getCurrentChroot() const
{
    QString arch = QSysInfo::currentCpuArchitecture();

    if (arch == QStringLiteral("x86_64") || arch == QStringLiteral("i386")) {
        arch = QStringLiteral("x86_64");
    }

    if (m_fedoraVersion == QStringLiteral("rawhide")) {
        return QStringLiteral("fedora-rawhide-%1").arg(arch);
    }

    return QStringLiteral("fedora-%1-%2").arg(m_fedoraVersion, arch);
}

void CoprClient::searchProjects(const QString &query, int limit, int offset)
{
    QString endpoint = QStringLiteral("/project/search");
    QUrl url(m_baseUrl + endpoint);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("query"), query);
    urlQuery.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    urlQuery.addQueryItem(QStringLiteral("offset"), QString::number(offset));
    url.setQuery(urlQuery);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprClient: Searching projects, query:" << query << "limit:" << limit << "offset:" << offset;
    queueRequest(url, QStringLiteral("searchProjects"));
}

void CoprClient::getLatestProjects(int limit, int offset)
{
    QString endpoint = QStringLiteral("/project/list");
    QUrl url(m_baseUrl + endpoint);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    urlQuery.addQueryItem(QStringLiteral("offset"), QString::number(offset));
    url.setQuery(urlQuery);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprClient: Getting latest projects, limit:" << limit << "offset:" << offset;
    queueRequest(url, QStringLiteral("getLatestProjects"));
}

void CoprClient::getPopularProjects(int limit, int offset)
{
    getLatestProjects(limit, offset);
}

void CoprClient::getProjectInfo(const QString &owner, const QString &project)
{
    QString endpoint = QStringLiteral("/project");
    QUrl url(m_baseUrl + endpoint);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("ownername"), owner);
    urlQuery.addQueryItem(QStringLiteral("projectname"), project);
    url.setQuery(urlQuery);

    queueRequest(url, QStringLiteral("getProjectInfo"));
}

void CoprClient::getProjectPackages(const QString &owner, const QString &project)
{
    QString endpoint = QStringLiteral("/package/list");
    QUrl url(m_baseUrl + endpoint);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("ownername"), owner);
    urlQuery.addQueryItem(QStringLiteral("projectname"), project);
    urlQuery.addQueryItem(QStringLiteral("with_latest_build"), QStringLiteral("True"));
    urlQuery.addQueryItem(QStringLiteral("limit"), QStringLiteral("10"));
    url.setQuery(urlQuery);

    queueRequest(url, QStringLiteral("getProjectPackages:") + owner + QStringLiteral(":") + project);
}

void CoprClient::searchPackages(const QString &query, int limit)
{
    searchProjects(query, limit);
}

void CoprClient::cancelAllRequests()
{
    for (auto &proc : std::as_const(m_activeProcesses)) {
        if (proc) {
            proc->disconnect();
            proc->kill();
            proc->deleteLater();
        }
    }
    m_activeProcesses.clear();
    m_requestQueue.clear();
    m_activeRequests = 0;

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Cancelled all COPR pending requests";
}

void CoprClient::clearCache()
{
    m_cache.clear();
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "COPR response cache cleared";
}

void CoprClient::queueRequest(const QUrl &url, const QString &requestType)
{
    QString urlString = url.toString();

    // Check cache first
    if (m_cache.contains(urlString)) {
        const CacheEntry &entry = m_cache[urlString];
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - entry.timestamp < CacheTtlMs) {
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "COPR cache hit for:" << requestType;
            QTimer::singleShot(0, this, [this, requestType, json = entry.data]() {
                emitResultForRequest(requestType, json);
            });
            return;
        }
        m_cache.remove(urlString);
    }

    // Deduplication - skip if same URL already queued
    for (const auto &queued : m_requestQueue) {
        if (queued.first == url) {
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "COPR request deduped:" << requestType;
            return;
        }
    }

    m_requestQueue.enqueue(qMakePair(url, requestType));
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Queued COPR request:" << requestType << "queue size:" << m_requestQueue.size();

    if (m_activeRequests < MaxConcurrentRequests) {
        processNextRequest();
    }
}

void CoprClient::emitResultForRequest(const QString &requestType, const QJsonObject &json)
{
    if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
        || requestType == QStringLiteral("getLatestProjects")) {
        Q_EMIT projectsFound(parseProjectsResponse(json));
    } else if (requestType == QStringLiteral("getProjectInfo")) {
        Q_EMIT projectInfoReceived(parseProjectResponse(json));
    } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
        QStringList parts = requestType.split(QLatin1Char(':'));
        if (parts.size() >= 3) {
            Q_EMIT packagesFound(parsePackagesResponse(json, parts[1], parts[2]));
        }
    }
}

void CoprClient::processNextRequest()
{
    if (m_requestQueue.isEmpty() || m_activeRequests >= MaxConcurrentRequests) {
        return;
    }

    m_activeRequests++;
    auto request_data = m_requestQueue.dequeue();
    QUrl url = request_data.first;
    QString requestType = request_data.second;
    QString urlString = url.toString();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Processing COPR request via curl:" << requestType << "active:" << m_activeRequests
                                                << "queued:" << m_requestQueue.size();

    QProcess *curlProcess = new QProcess(this);
    curlProcess->setProperty("requestType", requestType);
    m_activeProcesses.append(curlProcess);

    connect(curlProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, curlProcess, urlString](int exitCode, QProcess::ExitStatus) {
                m_activeProcesses.removeAll(curlProcess);

                QString requestType = curlProcess->property("requestType").toString();
                QByteArray data = curlProcess->readAllStandardOutput();

                m_activeRequests--;

                if (exitCode != 0 || data.isEmpty()) {
                    // curl exit code 28 = timeout
                    QString errorMsg =
                        exitCode == 28 ? QStringLiteral("COPR request timed out") : QStringLiteral("COPR request failed (exit code: %1)").arg(exitCode);
                    qWarning() << errorMsg << "for" << requestType;
                    Q_EMIT errorOccurred(errorMsg);

                    if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
                        || requestType == QStringLiteral("getLatestProjects")) {
                        Q_EMIT projectsFound(QList<CoprProjectInfo>());
                    } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
                        Q_EMIT packagesFound(QList<CoprPackageInfo>());
                    }
                    curlProcess->deleteLater();
                    processNextRequest();
                    return;
                }

                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (!doc.isObject()) {
                    qWarning() << "Invalid JSON from COPR for" << requestType << "- data:" << data.left(200);
                    Q_EMIT errorOccurred(QStringLiteral("Invalid response from COPR API"));

                    if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
                        || requestType == QStringLiteral("getLatestProjects")) {
                        Q_EMIT projectsFound(QList<CoprProjectInfo>());
                    } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
                        Q_EMIT packagesFound(QList<CoprPackageInfo>());
                    }
                    curlProcess->deleteLater();
                    processNextRequest();
                    return;
                }

                QJsonObject json = doc.object();

                // Cache the successful response
                m_cache[urlString] = CacheEntry{json, QDateTime::currentMSecsSinceEpoch()};

                emitResultForRequest(requestType, json);

                curlProcess->deleteLater();
                processNextRequest();
            });

    curlProcess->start(QStringLiteral("curl"),
                       {QStringLiteral("-s"),
                        QStringLiteral("-H"),
                        QStringLiteral("Accept: application/json"),
                        QStringLiteral("--max-time"),
                        QStringLiteral("15"),
                        url.toString()});

    // Try to start more concurrent requests from the queue
    if (!m_requestQueue.isEmpty() && m_activeRequests < MaxConcurrentRequests) {
        processNextRequest();
    }
}

QString CoprClient::convertMarkdownToHtml(const QString &markdown) const
{
    QString html = markdown;

    // Convert image badges with nested links like [![alt](image)](url) to clickable images
    html.replace(QRegularExpression(QStringLiteral("\\[!\\[([^\\]]*)\\]\\(([^\\)]+)\\)\\]\\(([^\\)]+)\\)")),
                 QStringLiteral("<a href=\"\\3\"><img src=\"\\2\" alt=\"\\1\" height=\"20\"></a>"));

    // Convert simple image badges like ![alt](url) to images
    html.replace(QRegularExpression(QStringLiteral("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)")), QStringLiteral("<img src=\"\\2\" alt=\"\\1\" height=\"20\">"));

    // Convert markdown links [text](url) to HTML links
    html.replace(QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\(([^\\)]+)\\)")), QStringLiteral("<a href=\"\\2\">\\1</a>"));

    // Convert headers (###, ##, #) to bold with sizes
    html.replace(QRegularExpression(QStringLiteral("^#{3}\\s+(.*)$"), QRegularExpression::MultilineOption), QStringLiteral("<b>\\1</b>"));
    html.replace(QRegularExpression(QStringLiteral("^#{2}\\s+(.*)$"), QRegularExpression::MultilineOption),
                 QStringLiteral("<b style='font-size: 110%;'>\\1</b>"));
    html.replace(QRegularExpression(QStringLiteral("^#\\s+(.*)$"), QRegularExpression::MultilineOption), QStringLiteral("<b style='font-size: 120%;'>\\1</b>"));

    // Convert **text** to bold
    html.replace(QRegularExpression(QStringLiteral("\\*\\*([^\\*]+)\\*\\*")), QStringLiteral("<b>\\1</b>"));

    // Convert `code` to monospace
    html.replace(QRegularExpression(QStringLiteral("`([^`]+)`")), QStringLiteral("<code>\\1</code>"));

    // Convert line breaks, but keep paragraph spacing
    html.replace(QStringLiteral("\n\n"), QStringLiteral("<br><br>"));
    html.replace(QStringLiteral("\n"), QStringLiteral("<br>"));

    return html;
}

QList<CoprProjectInfo> CoprClient::parseProjectsResponse(const QJsonObject &json)
{
    QList<CoprProjectInfo> projects;

    QJsonArray items = json.value(QStringLiteral("items")).toArray();

    for (const QJsonValue &value : items) {
        QJsonObject obj = value.toObject();

        CoprProjectInfo project;
        project.owner = obj.value(QStringLiteral("ownername")).toString();
        project.name = obj.value(QStringLiteral("name")).toString();
        project.description = convertMarkdownToHtml(obj.value(QStringLiteral("description")).toString());
        project.id = obj.value(QStringLiteral("id")).toInt();
        project.homepage = obj.value(QStringLiteral("homepage")).toString();

        // Try to get chroots from different possible fields
        QJsonObject chrootRepos = obj.value(QStringLiteral("chroot_repos")).toObject();
        if (!chrootRepos.isEmpty()) {
            project.chroots = chrootRepos.keys();
        } else {
            // For search results, chroots might be in a different field
            QJsonArray chrootsArray = obj.value(QStringLiteral("chroots")).toArray();
            for (const QJsonValue &chrootVal : chrootsArray) {
                QString chrootName = chrootVal.toString();
                if (!chrootName.isEmpty()) {
                    project.chroots.append(chrootName);
                }
            }
        }

        projects.append(project);
    }

    return projects;
}

CoprProjectInfo CoprClient::parseProjectResponse(const QJsonObject &json)
{
    CoprProjectInfo project;

    QJsonObject obj = json.value(QStringLiteral("project")).toObject();

    project.owner = obj.value(QStringLiteral("ownername")).toString();
    project.name = obj.value(QStringLiteral("name")).toString();
    project.description = convertMarkdownToHtml(obj.value(QStringLiteral("description")).toString());
    project.id = obj.value(QStringLiteral("id")).toInt();
    project.homepage = obj.value(QStringLiteral("homepage")).toString();

    project.chroots = obj.value(QStringLiteral("chroot_repos")).toObject().keys();

    return project;
}

QList<CoprPackageInfo> CoprClient::parsePackagesResponse(const QJsonObject &json, const QString &owner, const QString &project)
{
    QList<CoprPackageInfo> packages;

    QJsonArray items = json.value(QStringLiteral("items")).toArray();

    for (const QJsonValue &value : items) {
        QJsonObject obj = value.toObject();

        CoprPackageInfo package;
        package.name = obj.value(QStringLiteral("name")).toString();
        package.owner = owner;
        package.projectName = project;

        // Extract info from latest build if available
        QJsonObject builds = obj.value(QStringLiteral("builds")).toObject();
        QJsonObject latestBuild = builds.value(QStringLiteral("latest")).toObject();

        if (!latestBuild.isEmpty()) {
            // Get build chroots to determine availability
            QJsonArray chroots = latestBuild.value(QStringLiteral("chroots")).toArray();
            for (const QJsonValue &chrootVal : chroots) {
                QString chrootName = chrootVal.toString();
                if (!chrootName.isEmpty()) {
                    package.availableChroots.append(chrootName);
                }
            }

            // Check if available for current Fedora
            package.isAvailableForCurrentFedora = package.availableChroots.contains(m_currentChroot);
        }

        // Get source info if available
        QJsonObject source = obj.value(QStringLiteral("source_dict")).toObject();
        if (!source.isEmpty()) {
            QString url = source.value(QStringLiteral("url")).toString();
            if (!url.isEmpty()) {
                package.homepage = url;
            }
        }

        packages.append(package);
    }

    return packages;
}
