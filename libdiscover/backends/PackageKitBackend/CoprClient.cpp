#include "CoprClient.h"
#include "libdiscover_backend_packagekit_debug.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QSysInfo>
#include <QDebug>
#include <QRegularExpression>

// Network timeout for COPR API requests (in milliseconds)
static constexpr int COPR_REQUEST_TIMEOUT = 30000; // 30 seconds - increased for slower API responses

CoprClient::CoprClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
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

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Disable HTTP/2 to avoid protocol errors
    request.setTransferTimeout(COPR_REQUEST_TIMEOUT);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprClient: Searching projects, query:" << query << "limit:" << limit << "offset:" << offset;

    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingRequests[reply] = QStringLiteral("searchProjects");
    connect(reply, &QNetworkReply::finished, this, &CoprClient::onNetworkReplyFinished);
}

void CoprClient::getLatestProjects(int limit, int offset)
{
    QString endpoint = QStringLiteral("/project/list");
    QUrl url(m_baseUrl + endpoint);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    urlQuery.addQueryItem(QStringLiteral("offset"), QString::number(offset));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Disable HTTP/2 to avoid protocol errors
    request.setTransferTimeout(COPR_REQUEST_TIMEOUT);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprClient: Getting latest projects, limit:" << limit << "offset:" << offset;

    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingRequests[reply] = QStringLiteral("getLatestProjects");
    connect(reply, &QNetworkReply::finished, this, &CoprClient::onNetworkReplyFinished);
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

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Disable HTTP/2 to avoid protocol errors
    request.setTransferTimeout(COPR_REQUEST_TIMEOUT);

    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingRequests[reply] = QStringLiteral("getProjectInfo");
    connect(reply, &QNetworkReply::finished, this, &CoprClient::onNetworkReplyFinished);
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

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Disable HTTP/2 to avoid protocol errors
    request.setTransferTimeout(COPR_REQUEST_TIMEOUT);

    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingRequests[reply] = QStringLiteral("getProjectPackages:") + owner + QStringLiteral(":") + project;
    connect(reply, &QNetworkReply::finished, this, &CoprClient::onNetworkReplyFinished);
}

void CoprClient::searchPackages(const QString &query, int limit)
{
    searchProjects(query, limit);
}

void CoprClient::cancelAllRequests()
{
    // Cancel all pending network requests
    QList<QNetworkReply*> replies = m_pendingRequests.keys();
    for (QNetworkReply* reply : replies) {
        if (reply) {
            reply->abort();
            reply->deleteLater();
        }
    }
    m_pendingRequests.clear();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Cancelled all COPR pending requests";
}

void CoprClient::onNetworkReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    QString requestType = m_pendingRequests.take(reply);

    if (reply->error() != QNetworkReply::NoError) {
        // Log the error but don't spam if it's HTTP/2 protocol errors
        if (!reply->errorString().contains(QStringLiteral("HTTP/2"))) {
            qWarning() << "COPR API error for" << requestType << ":" << reply->errorString()
                       << "Error code:" << reply->error()
                       << "URL:" << reply->url().toString();
        } else {
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "HTTP/2 error for" << requestType << "- likely too many requests";
        }
        // Don't emit error for individual project failures, just log and continue
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON response from COPR API";
        Q_EMIT errorOccurred(QStringLiteral("Invalid JSON response"));
        reply->deleteLater();
        return;
    }

    QJsonObject json = doc.object();

    if (requestType == QStringLiteral("searchProjects") || requestType == QStringLiteral("getPopularProjects") || requestType == QStringLiteral("getLatestProjects")) {
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

    reply->deleteLater();
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
