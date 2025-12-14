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
    // Deprecated - just use getLatestProjects instead
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
    // Clear the request queue
    m_requestQueue.clear();
    m_requestInProgress = false;

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Cancelled all COPR pending requests";
}

void CoprClient::queueRequest(const QUrl &url, const QString &requestType)
{
    m_requestQueue.enqueue(qMakePair(url, requestType));
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Queued COPR request:" << requestType << "queue size:" << m_requestQueue.size();

    if (!m_requestInProgress) {
        processNextRequest();
    }
}

void CoprClient::processNextRequest()
{
    if (m_requestQueue.isEmpty()) {
        m_requestInProgress = false;
        return;
    }

    m_requestInProgress = true;
    auto request_data = m_requestQueue.dequeue();
    QUrl url = request_data.first;
    QString requestType = request_data.second;

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Processing COPR request via curl:" << requestType << "URL:" << url.toString();

    // Use curl to bypass TLS fingerprinting and anti-bot protection
    QProcess *curlProcess = new QProcess(this);
    curlProcess->setProperty("requestType", requestType);

    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, curlProcess](int exitCode, QProcess::ExitStatus) {
        QString requestType = curlProcess->property("requestType").toString();
        QByteArray data = curlProcess->readAllStandardOutput();

        if (exitCode != 0 || data.isEmpty()) {
            qWarning() << "COPR curl request failed for" << requestType << "exit code:" << exitCode;
            if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
                || requestType == QStringLiteral("getLatestProjects")) {
                Q_EMIT projectsFound(QList<CoprProjectInfo>());
            } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
                Q_EMIT packagesFound(QList<CoprPackageInfo>());
            }
            curlProcess->deleteLater();
            QTimer::singleShot(100, this, &CoprClient::processNextRequest);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            qWarning() << "Invalid JSON from curl for" << requestType << "- data:" << data.left(200);
            if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
                || requestType == QStringLiteral("getLatestProjects")) {
                Q_EMIT projectsFound(QList<CoprProjectInfo>());
            } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
                Q_EMIT packagesFound(QList<CoprPackageInfo>());
            }
            curlProcess->deleteLater();
            QTimer::singleShot(100, this, &CoprClient::processNextRequest);
            return;
        }

        QJsonObject json = doc.object();

        if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
            || requestType == QStringLiteral("getLatestProjects")) {
            QList<CoprProjectInfo> projects = parseProjectsResponse(json);
            Q_EMIT projectsFound(projects);
        } else if (requestType == QStringLiteral("getProjectInfo")) {
            CoprProjectInfo project = parseProjectResponse(json);
            Q_EMIT projectInfoReceived(project);
        } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
            QStringList parts = requestType.split(QLatin1Char(':'));
            if (parts.size() >= 3) {
                QString owner = parts[1];
                QString project = parts[2];
                QList<CoprPackageInfo> packages = parsePackagesResponse(json, owner, project);
                Q_EMIT packagesFound(packages);
            }
        }

        curlProcess->deleteLater();
        QTimer::singleShot(100, this, &CoprClient::processNextRequest);
    });

    curlProcess->start(QStringLiteral("curl"),
                       {QStringLiteral("-s"),
                        QStringLiteral("-H"),
                        QStringLiteral("Accept: application/json"),
                        QStringLiteral("--max-time"),
                        QStringLiteral("30"),
                        url.toString()});
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

        // The description from COPR API is in Markdown format
        // We need to convert it to HTML for display
        QString rawDesc = obj.value(QStringLiteral("description")).toString();

        // Convert image badges with nested links like [![alt](image)](url) to clickable images
        rawDesc.replace(QRegularExpression(QStringLiteral("\\[!\\[([^\\]]*)\\]\\(([^\\)]+)\\)\\]\\(([^\\)]+)\\)")),
                       QStringLiteral("<a href=\"\\3\"><img src=\"\\2\" alt=\"\\1\" height=\"20\"></a>"));

        // Convert simple image badges like ![alt](url) to images
        rawDesc.replace(QRegularExpression(QStringLiteral("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)")),
                       QStringLiteral("<img src=\"\\2\" alt=\"\\1\" height=\"20\">"));

        // Convert markdown links [text](url) to HTML links
        rawDesc.replace(QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\(([^\\)]+)\\)")),
                       QStringLiteral("<a href=\"\\2\">\\1</a>"));

        // Convert headers (###, ##, #) to bold with sizes
        rawDesc.replace(QRegularExpression(QStringLiteral("^#{3}\\s+(.*)$"), QRegularExpression::MultilineOption),
                       QStringLiteral("<b>\\1</b>"));
        rawDesc.replace(QRegularExpression(QStringLiteral("^#{2}\\s+(.*)$"), QRegularExpression::MultilineOption),
                       QStringLiteral("<b style='font-size: 110%;'>\\1</b>"));
        rawDesc.replace(QRegularExpression(QStringLiteral("^#\\s+(.*)$"), QRegularExpression::MultilineOption),
                       QStringLiteral("<b style='font-size: 120%;'>\\1</b>"));

        // Convert **text** to bold
        rawDesc.replace(QRegularExpression(QStringLiteral("\\*\\*([^\\*]+)\\*\\*")), QStringLiteral("<b>\\1</b>"));

        // Convert `code` to monospace
        rawDesc.replace(QRegularExpression(QStringLiteral("`([^`]+)`")), QStringLiteral("<code>\\1</code>"));

        // Convert line breaks, but keep paragraph spacing
        rawDesc.replace(QStringLiteral("\n\n"), QStringLiteral("<br><br>"));
        rawDesc.replace(QStringLiteral("\n"), QStringLiteral("<br>"));

        project.description = rawDesc;

        project.id = obj.value(QStringLiteral("id")).toInt();
        project.homepage = obj.value(QStringLiteral("homepage")).toString();

        // Try to get chroots from different possible fields
        QJsonObject chrootRepos = obj.value(QStringLiteral("chroot_repos")).toObject();
        if (!chrootRepos.isEmpty()) {
            QStringList chrootKeys = chrootRepos.keys();
            for (const QString &chroot : chrootKeys) {
                project.chroots.append(chroot);
            }
        } else {
            // For search results, chroots might be in a different field
            QJsonArray chrootsArray = obj.value(QStringLiteral("chroots")).toArray();
            if (!chrootsArray.isEmpty()) {
                for (const QJsonValue &chrootVal : chrootsArray) {
                    QString chrootName = chrootVal.toString();
                    if (!chrootName.isEmpty()) {
                        project.chroots.append(chrootName);
                    }
                }
            } else {
                qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "No chroots found for project:" << project.owner << "/" << project.name;
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

    // The description from COPR API is in Markdown format
    // We need to convert it to HTML for display
    QString rawDesc = obj.value(QStringLiteral("description")).toString();

    // Convert image badges with nested links like [![alt](image)](url) to clickable images
    rawDesc.replace(QRegularExpression(QStringLiteral("\\[!\\[([^\\]]*)\\]\\(([^\\)]+)\\)\\]\\(([^\\)]+)\\)")),
                   QStringLiteral("<a href=\"\\3\"><img src=\"\\2\" alt=\"\\1\" height=\"20\"></a>"));

    // Convert simple image badges like ![alt](url) to images
    rawDesc.replace(QRegularExpression(QStringLiteral("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)")),
                   QStringLiteral("<img src=\"\\2\" alt=\"\\1\" height=\"20\">"));

    // Convert markdown links [text](url) to HTML links
    rawDesc.replace(QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\(([^\\)]+)\\)")),
                   QStringLiteral("<a href=\"\\2\">\\1</a>"));

    // Convert headers (###, ##, #) to bold with sizes
    rawDesc.replace(QRegularExpression(QStringLiteral("^#{3}\\s+(.*)$"), QRegularExpression::MultilineOption),
                   QStringLiteral("<b>\\1</b>"));
    rawDesc.replace(QRegularExpression(QStringLiteral("^#{2}\\s+(.*)$"), QRegularExpression::MultilineOption),
                   QStringLiteral("<b style='font-size: 110%;'>\\1</b>"));
    rawDesc.replace(QRegularExpression(QStringLiteral("^#\\s+(.*)$"), QRegularExpression::MultilineOption),
                   QStringLiteral("<b style='font-size: 120%;'>\\1</b>"));

    // Convert **text** to bold
    rawDesc.replace(QRegularExpression(QStringLiteral("\\*\\*([^\\*]+)\\*\\*")), QStringLiteral("<b>\\1</b>"));

    // Convert `code` to monospace
    rawDesc.replace(QRegularExpression(QStringLiteral("`([^`]+)`")), QStringLiteral("<code>\\1</code>"));

    // Convert line breaks, but keep paragraph spacing
    rawDesc.replace(QStringLiteral("\n\n"), QStringLiteral("<br><br>"));
    rawDesc.replace(QStringLiteral("\n"), QStringLiteral("<br>"));

    project.description = rawDesc;

    project.id = obj.value(QStringLiteral("id")).toInt();
    project.homepage = obj.value(QStringLiteral("homepage")).toString();

    QStringList chrootKeys = obj.value(QStringLiteral("chroot_repos")).toObject().keys();
    for (const QString &chroot : chrootKeys) {
        project.chroots.append(chroot);
    }

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
            QString sourceType = source.value(QStringLiteral("type")).toString();
            QString url = source.value(QStringLiteral("url")).toString();
            if (!url.isEmpty()) {
                package.homepage = url;
            }
        }

        packages.append(package);
    }

    return packages;
}
