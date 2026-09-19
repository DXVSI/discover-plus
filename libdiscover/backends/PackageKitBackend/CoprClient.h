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
    bool unlistedOnHomepage = false;
};

class CoprClient : public QObject
{
    Q_OBJECT

public:
    explicit CoprClient(QObject *parent = nullptr);
    ~CoprClient() override;

    // Empty when the chroot of the running system could not be detected.
    // Availability is unknown then and nothing may be blocked because of it.
    QString getCurrentChroot() const
    {
        return m_currentChroot;
    }
    bool isCurrentChrootKnown() const
    {
        return !m_currentChroot.isEmpty();
    }

    static QUrl projectWebUrl(const QString &owner, const QString &project);

    void searchProjects(const QString &query, int limit = 50, int offset = 0);
    void getLatestProjects(int limit, int offset);
    void getProjectPackages(const QString &owner, const QString &project);
    void cancelAllRequests();

Q_SIGNALS:
    void projectsFound(const QList<CoprProjectInfo> &projects);
    void projectPackagesFound(const QString &owner, const QString &project, const QList<CoprPackageInfo> &packages);
    void errorOccurred(const QString &requestType, const QString &errorMessage);

private:
    QString detectCurrentChroot() const;
    QList<CoprProjectInfo> parseProjectsResponse(const QJsonObject &json);
    CoprProjectInfo parseProjectObject(const QJsonObject &json);
    QList<CoprPackageInfo> parsePackagesResponse(const QJsonObject &json, const QString &owner, const QString &project);
    QString convertMarkdownToHtml(const QString &markdown) const;
    void emitResultForRequest(const QString &requestType, const QJsonObject &json);
    void emitEmptyResultForRequest(const QString &requestType);

    void failRequest(const QString &requestType, const QString &errorMessage);
    void noteRetryAfter(const QNetworkReply *reply);
    // Empty unless the server asked us to back off
    QString backOffMessage() const;
    void storeInCache(const QString &urlString, const QJsonObject &json, qint64 size);

    void processNextRequest();
    void queueRequest(const QUrl &url, const QString &requestType);

    QString m_baseUrl;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QString m_currentChroot;

    // Set after HTTP 429/503: no new network request is sent before this moment
    qint64 m_retryNotBeforeMs = 0;
    static constexpr int DefaultRetryAfterSecs = 60;
    static constexpr int MaxRetryAfterSecs = 3600;

    // Request queue with limited concurrency
    QQueue<QPair<QUrl, QString>> m_requestQueue;
    int m_activeRequests = 0;
    static constexpr int MaxConcurrentRequests = 3;

    // Active network replies (for cancellation)
    QList<QPointer<QNetworkReply>> m_activeReplies;
    // Bumped by cancelAllRequests() so that deferred answers of cancelled requests are dropped
    quint64 m_requestGeneration = 0;

    // Response cache with TTL
    struct CacheEntry {
        QJsonObject data;
        qint64 timestamp;
        qint64 size; // of the raw response
    };
    QHash<QString, CacheEntry> m_cache;
    static constexpr int CacheTtlMs = 300000; // 5 minutes
    static constexpr qint64 MaxCacheBytes = 4 * 1024 * 1024; // about 8 project list pages
};

#endif // COPRCLIENT_H
