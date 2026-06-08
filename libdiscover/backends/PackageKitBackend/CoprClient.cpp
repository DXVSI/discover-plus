#include "CoprClient.h"
#include "libdiscover_backend_packagekit_debug.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

CoprClient::CoprClient(QObject *parent)
    : QObject(parent)
    , m_baseUrl(QStringLiteral("https://copr.fedorainfracloud.org/api_3"))
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
    m_fedoraVersion = getFedoraVersion();
    m_currentChroot = getCurrentChroot();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprClient initialized. Fedora version:" << m_fedoraVersion << "Chroot:" << m_currentChroot;
}

CoprClient::~CoprClient()
{
    cancelAllRequests();
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
    for (const auto &reply : std::as_const(m_activeReplies)) {
        if (reply) {
            reply->disconnect(this);
            reply->abort();
            reply->deleteLater();
        }
    }
    m_activeReplies.clear();
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
            const QList<CoprPackageInfo> packages = parsePackagesResponse(json, parts[1], parts[2]);
            Q_EMIT projectPackagesFound(parts[1], parts[2], packages);
        }
    }
}

void CoprClient::emitEmptyResultForRequest(const QString &requestType)
{
    if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects")
        || requestType == QStringLiteral("getLatestProjects")) {
        Q_EMIT projectsFound(QList<CoprProjectInfo>());
    } else if (requestType.startsWith(QStringLiteral("getProjectPackages:"))) {
        const QStringList parts = requestType.split(QLatin1Char(':'));
        if (parts.size() >= 3) {
            Q_EMIT projectPackagesFound(parts[1], parts[2], QList<CoprPackageInfo>());
        }
    }
}

void CoprClient::processNextRequest()
{
    while (!m_requestQueue.isEmpty() && m_activeRequests < MaxConcurrentRequests) {
        const auto requestData = m_requestQueue.dequeue();
        const QUrl url = requestData.first;
        const QString requestType = requestData.second;
        const QString urlString = url.toString();

        QNetworkRequest request(url);
        request.setRawHeader("Accept", "application/json");

        QNetworkReply *reply = m_networkAccessManager->get(request);
        reply->setProperty("requestType", requestType);
        reply->setProperty("urlString", urlString);

        m_activeReplies.append(reply);
        ++m_activeRequests;

        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Processing COPR request via Qt network:" << requestType << "active:" << m_activeRequests
                                                    << "queued:" << m_requestQueue.size();

        const int timeoutMs = requestType == QStringLiteral("searchProjects") ? 25000 : 10000;
        QTimer::singleShot(timeoutMs, reply, [reply]() {
            if (reply->isRunning()) {
                reply->setProperty("timedOut", true);
                reply->abort();
            }
        });

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            m_activeReplies.removeAll(reply);

            const QString requestType = reply->property("requestType").toString();
            const QString urlString = reply->property("urlString").toString();
            const bool timedOut = reply->property("timedOut").toBool();
            const QByteArray data = reply->readAll();

            if (m_activeRequests > 0) {
                --m_activeRequests;
            }

            if (timedOut || reply->error() != QNetworkReply::NoError || data.isEmpty()) {
                const QString errorMsg =
                    timedOut ? QStringLiteral("COPR request timed out") : QStringLiteral("COPR request failed: %1").arg(reply->errorString());
                qWarning() << errorMsg << "for" << requestType;
                Q_EMIT errorOccurred(errorMsg);
                emitEmptyResultForRequest(requestType);
                reply->deleteLater();
                processNextRequest();
                return;
            }

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            if (!doc.isObject()) {
                qWarning() << "Invalid JSON from COPR for" << requestType << "- error:" << parseError.errorString() << "- data:" << data.left(200);
                Q_EMIT errorOccurred(QStringLiteral("Invalid response from COPR API"));
                emitEmptyResultForRequest(requestType);
                reply->deleteLater();
                processNextRequest();
                return;
            }

            const QJsonObject json = doc.object();

            // Cache the successful response
            m_cache[urlString] = CacheEntry{json, QDateTime::currentMSecsSinceEpoch()};

            emitResultForRequest(requestType, json);

            reply->deleteLater();
            processNextRequest();
        });
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

static QStringList jsonStringArray(const QJsonArray &array)
{
    QStringList values;
    values.reserve(array.size());

    for (const QJsonValue &value : array) {
        const QString text = value.toString();
        if (!text.isEmpty()) {
            values.append(text);
        }
    }

    return values;
}

static QString jsonValueToDisplayString(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toInt());
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    return {};
}

static QDateTime unixTimestampToDateTime(const QJsonValue &value)
{
    const qint64 timestamp = value.toVariant().toLongLong();
    if (timestamp <= 0) {
        return {};
    }
    return QDateTime::fromSecsSinceEpoch(timestamp);
}

static QString sourceUrlFromSourceDict(const QJsonObject &source)
{
    const QString url = source.value(QStringLiteral("url")).toString();
    if (!url.isEmpty()) {
        return url;
    }

    const QString cloneUrl = source.value(QStringLiteral("clone_url")).toString();
    if (!cloneUrl.isEmpty()) {
        return cloneUrl;
    }

    return {};
}

CoprProjectInfo CoprClient::parseProjectObject(const QJsonObject &obj)
{
    CoprProjectInfo project;
    project.owner = obj.value(QStringLiteral("ownername")).toString();
    project.name = obj.value(QStringLiteral("name")).toString();
    project.fullName = obj.value(QStringLiteral("full_name")).toString();
    project.description = convertMarkdownToHtml(obj.value(QStringLiteral("description")).toString());
    project.instructions = convertMarkdownToHtml(obj.value(QStringLiteral("instructions")).toString());
    project.contact = obj.value(QStringLiteral("contact")).toString();
    project.id = obj.value(QStringLiteral("id")).toInt();
    project.homepage = obj.value(QStringLiteral("homepage")).toString();
    project.additionalRepos = jsonStringArray(obj.value(QStringLiteral("additional_repos")).toArray());
    project.repoPriority = jsonValueToDisplayString(obj.value(QStringLiteral("repo_priority")));
    project.appstream = obj.value(QStringLiteral("appstream")).toBool();
    project.develMode = obj.value(QStringLiteral("devel_mode")).toBool();
    project.enableNet = obj.value(QStringLiteral("enable_net")).toBool();
    project.followFedoraBranching = obj.value(QStringLiteral("follow_fedora_branching")).toBool();
    project.autoPrune = obj.value(QStringLiteral("auto_prune")).toBool();
    project.moduleHotfixes = obj.value(QStringLiteral("module_hotfixes")).toBool();

    const QJsonObject chrootRepos = obj.value(QStringLiteral("chroot_repos")).toObject();
    if (!chrootRepos.isEmpty()) {
        project.chroots = chrootRepos.keys();
    } else {
        project.chroots = jsonStringArray(obj.value(QStringLiteral("chroots")).toArray());
    }

    return project;
}

QList<CoprProjectInfo> CoprClient::parseProjectsResponse(const QJsonObject &json)
{
    QList<CoprProjectInfo> projects;

    QJsonArray items = json.value(QStringLiteral("items")).toArray();

    for (const QJsonValue &value : items) {
        projects.append(parseProjectObject(value.toObject()));
    }

    return projects;
}

CoprProjectInfo CoprClient::parseProjectResponse(const QJsonObject &json)
{
    CoprProjectInfo project;

    QJsonObject obj = json.value(QStringLiteral("project")).toObject();
    if (obj.isEmpty()) {
        obj = json;
    }

    project = parseProjectObject(obj);

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
        package.sourceType = obj.value(QStringLiteral("source_type")).toString();

        // Extract info from latest build if available
        QJsonObject builds = obj.value(QStringLiteral("builds")).toObject();
        QJsonObject latestBuild = builds.value(QStringLiteral("latest_succeeded")).toObject();
        if (latestBuild.isEmpty()) {
            latestBuild = builds.value(QStringLiteral("latest")).toObject();
        }

        if (!latestBuild.isEmpty()) {
            // Get build chroots to determine availability
            package.availableChroots = jsonStringArray(latestBuild.value(QStringLiteral("chroots")).toArray());

            // Check if available for current Fedora
            package.isAvailableForCurrentFedora = package.availableChroots.contains(m_currentChroot);
            package.latestBuildState = latestBuild.value(QStringLiteral("state")).toString();
            package.latestBuildRepoUrl = latestBuild.value(QStringLiteral("repo_url")).toString();
            package.latestBuildSubmitter = latestBuild.value(QStringLiteral("submitter")).toString();
            package.latestBuildSubmittedOn = unixTimestampToDateTime(latestBuild.value(QStringLiteral("submitted_on")));
            package.latestBuildStartedOn = unixTimestampToDateTime(latestBuild.value(QStringLiteral("started_on")));
            package.latestBuildEndedOn = unixTimestampToDateTime(latestBuild.value(QStringLiteral("ended_on")));

            const QJsonObject sourcePackage = latestBuild.value(QStringLiteral("source_package")).toObject();
            package.version = sourcePackage.value(QStringLiteral("version")).toString();
        }

        // Get source info if available
        QJsonObject source = obj.value(QStringLiteral("source_dict")).toObject();
        if (!source.isEmpty()) {
            package.sourceUrl = sourceUrlFromSourceDict(source);
            package.sourceSpec = source.value(QStringLiteral("spec")).toString();
            package.sourceSubdirectory = source.value(QStringLiteral("subdirectory")).toString();
            if (package.homepage.isEmpty()) {
                package.homepage = package.sourceUrl;
            }
        }

        packages.append(package);
    }

    return packages;
}
