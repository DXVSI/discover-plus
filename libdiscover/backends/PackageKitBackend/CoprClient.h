#ifndef COPRCLIENT_H
#define COPRCLIENT_H

#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QUrl>

struct CoprPackageInfo {
    QString name;
    QString description;
    QString owner;
    QString projectName;
    QString projectFullName;
    QString version;
    QStringList availableChroots;
    bool isAvailableForCurrentFedora = false;
    int projectId = 0;
    QString homepage;
    QString instructions;
    QString contact;
    QStringList additionalRepos;
    QString repoPriority;
    bool appstream = false;
    bool develMode = false;
    bool enableNet = false;
    bool followFedoraBranching = false;
    bool autoPrune = false;
    bool moduleHotfixes = false;
    bool isProjectResource = false;
    QString sourceType;
    QString sourceUrl;
    QString sourceSpec;
    QString sourceSubdirectory;
    QString latestBuildState;
    QString latestBuildRepoUrl;
    QString latestBuildSubmitter;
    QDateTime latestBuildSubmittedOn;
    QDateTime latestBuildStartedOn;
    QDateTime latestBuildEndedOn;
};

struct CoprProjectInfo {
    QString owner;
    QString name;
    QString fullName;
    QString description;
    QStringList chroots;
    QString homepage;
    int id = 0;
    QString instructions;
    QString contact;
    QStringList additionalRepos;
    QString repoPriority;
    bool appstream = false;
    bool develMode = false;
    bool enableNet = false;
    bool followFedoraBranching = false;
    bool autoPrune = false;
    bool moduleHotfixes = false;
};

class CoprClient : public QObject
{
    Q_OBJECT

public:
    explicit CoprClient(QObject *parent = nullptr);
    ~CoprClient() override;

    QString getFedoraVersion() const;
    QString getCurrentChroot() const;

    void searchProjects(const QString &query, int limit = 50, int offset = 0);
    void getPopularProjects(int limit = 20, int offset = 0);
    void getLatestProjects(int limit = 20, int offset = 0);
    void getProjectInfo(const QString &owner, const QString &project);
    void getProjectPackages(const QString &owner, const QString &project);
    void searchPackages(const QString &query, int limit = 50);
    void cancelAllRequests();
    void clearCache();

Q_SIGNALS:
    void projectsFound(const QList<CoprProjectInfo> &projects);
    void projectInfoReceived(const CoprProjectInfo &project);
    void packagesFound(const QList<CoprPackageInfo> &packages);
    void projectPackagesFound(const QString &owner, const QString &project, const QList<CoprPackageInfo> &packages);
    void errorOccurred(const QString &errorMessage);

private:
    QList<CoprProjectInfo> parseProjectsResponse(const QJsonObject &json);
    CoprProjectInfo parseProjectObject(const QJsonObject &json);
    CoprProjectInfo parseProjectResponse(const QJsonObject &json);
    QList<CoprPackageInfo> parsePackagesResponse(const QJsonObject &json, const QString &owner, const QString &project);
    QString convertMarkdownToHtml(const QString &markdown) const;
    void emitResultForRequest(const QString &requestType, const QJsonObject &json);
    void emitEmptyResultForRequest(const QString &requestType);

    void processNextRequest();
    void queueRequest(const QUrl &url, const QString &requestType);

    QString m_baseUrl;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QString m_fedoraVersion;
    QString m_currentChroot;

    // Request queue with limited concurrency
    QQueue<QPair<QUrl, QString>> m_requestQueue;
    int m_activeRequests = 0;
    static constexpr int MaxConcurrentRequests = 3;

    // Active network replies (for cancellation)
    QList<QPointer<QNetworkReply>> m_activeReplies;

    // Response cache with TTL
    struct CacheEntry {
        QJsonObject data;
        qint64 timestamp;
    };
    QHash<QString, CacheEntry> m_cache;
    static constexpr int CacheTtlMs = 300000; // 5 minutes
};

#endif // COPRCLIENT_H
