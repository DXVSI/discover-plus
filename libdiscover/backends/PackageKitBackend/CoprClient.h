/*
 *   SPDX-FileCopyrightText: 2025-2026 DXVSI <https://github.com/DXVSI>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

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

// What the monitor says about one package in the chroot of this system
enum class CoprAvailability {
    Unknown, // not asked, or the chroot of this system is unknown
    Available,
    NotConfirmed, // the last build for this chroot did not succeed, an older one may still be published
    NotAvailable, // never built for this chroot
};

struct CoprPackageInfo {
    QString name;
    QString description;
    QString owner;
    QString projectName;
    QString projectFullName;
    QString version;
    // Chroots with an installable build of this package (from the monitor)
    QStringList availableChroots;
    // Chroots enabled in the project (chroot_repos), never the chroots of a single build
    QStringList projectChroots;
    CoprAvailability availability = CoprAvailability::Unknown;
    // Monitor state of the last build for the chroot of this system
    QString currentChrootState;
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
    // One project by its exact name; group owners are written "@group". Answers with
    // projectFound() or projectNotFound(), never with errorOccurred()
    void getProject(const QString &owner, const QString &project);
    // Newest first, or by name
    void getLatestProjects(int limit, int offset, bool byName = false);
    // The package list of an application page: details, versions and dates. The server
    // walks the builds of every package for it, so it is never used for many projects.
    void getProjectPackages(const QString &owner, const QString &project);
    // Names, versions and availability of all packages, about 150 bytes per package.
    // A request for an open application page survives cancelStreamRequests().
    void getProjectMonitor(const QString &owner, const QString &project, bool forOpenPage = false);
    // The same for a list item that merely became visible. Such requests are served
    // after everything else, the newest first, and never use the last free slot.
    // Returns false when nothing was queued because lazy requests are paused.
    bool getProjectMonitorLazily(const QString &owner, const QString &project);
    // The list item went away: takes its lazy request back while it is still queued
    void dropLazyProjectMonitor(const QString &owner, const QString &project);
    // A new list or search replaces the previous one: cancels the list and search
    // requests and the monitor requests that were made for list items
    void cancelStreamRequests();
    void cancelAllRequests();

    static QString projectPackagesRequestType()
    {
        return QStringLiteral("getProjectPackages");
    }
    static QString projectMonitorRequestType()
    {
        return QStringLiteral("getProjectMonitor");
    }

Q_SIGNALS:
    void projectsFound(const QList<CoprProjectInfo> &projects);
    void projectFound(const CoprProjectInfo &project);
    // Also when the request failed: the search that follows reports what is wrong
    void projectNotFound(const QString &owner, const QString &project);
    // complete is false when the project has more packages than the client keeps
    void projectPackagesFound(const QString &owner, const QString &project, const QList<CoprPackageInfo> &packages, bool complete);
    void projectMonitorFound(const QString &owner, const QString &project, const QList<CoprPackageInfo> &packages, bool complete);
    // requestType is projectPackagesRequestType() or projectMonitorRequestType()
    void projectRequestFailed(const QString &requestType, const QString &owner, const QString &project, const QString &errorMessage);
    void projectRequestCancelled(const QString &requestType, const QString &owner, const QString &project);
    void errorOccurred(const QString &requestType, const QString &errorMessage);

private:
    QString detectCurrentChroot() const;
    QList<CoprProjectInfo> parseProjectsResponse(const QJsonObject &json);
    CoprProjectInfo parseProjectObject(const QJsonObject &json);
    QList<CoprPackageInfo> parsePackagesResponse(const QJsonObject &json, const QString &owner, const QString &project);
    QList<CoprPackageInfo> parseMonitorResponse(const QJsonObject &json, const QString &owner, const QString &project, bool *complete);
    QString convertMarkdownToHtml(const QString &markdown) const;

    struct Request {
        QUrl url;
        QString requestType;
        // Made for an open application page: not cancelled together with the list
        bool forOpenPage = false;
        // Made for a visible list item: see getProjectMonitorLazily()
        bool lazy = false;
    };
    QUrl projectMonitorUrl(const QString &owner, const QString &project) const;
    bool isLazyPaused() const;
    void noteLazyResult(bool failed);
    void cancelLazyQueue();
    void requestProjectPackagesPage(const QString &owner, const QString &project, int offset);
    void emitResultForRequest(const Request &request, const QJsonObject &json);
    void failRequest(const Request &request, const QString &errorMessage);
    void cancelRequest(const Request &request);
    void noteRetryAfter(const QNetworkReply *reply);
    // Empty unless the server asked us to back off
    QString backOffMessage() const;
    void storeInCache(const Request &request, const QJsonObject &json, qint64 size);

    void processNextRequest();
    void queueRequest(const Request &request);

    QString m_baseUrl;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QString m_currentChroot;

    // Set after HTTP 429/503: no new network request is sent before this moment
    qint64 m_retryNotBeforeMs = 0;
    static constexpr int DefaultRetryAfterSecs = 60;
    static constexpr int MaxRetryAfterSecs = 3600;

    // Request queue with limited concurrency
    QQueue<Request> m_requestQueue;
    int m_activeRequests = 0;
    static constexpr int MaxConcurrentRequests = 3;

    // Lazy requests: a stack, so that the rows on screen win over those that were
    // scrolled past. One slot always stays free for lists, searches and open pages.
    QList<Request> m_lazyQueue;
    int m_activeLazyRequests = 0;
    static constexpr int MaxConcurrentLazyRequests = MaxConcurrentRequests - 1;
    static constexpr int MaxLazyQueueLength = 24;
    // Circuit breaker: lazy requests are refused until this moment. A back-off that
    // the server asked for (m_retryNotBeforeMs) pauses them as well.
    qint64 m_lazyPausedUntilMs = 0;
    int m_lazyFailuresInARow = 0;
    static constexpr int MaxLazyFailuresInARow = 3;
    static constexpr int LazyPauseSecs = 60;

    // Active network replies (for cancellation)
    QList<QPointer<QNetworkReply>> m_activeReplies;
    // Bumped by every cancellation so that deferred answers of cancelled requests are dropped
    quint64 m_requestGeneration = 0;

    // Pages of a package list that is still being collected, by "owner/project"
    QHash<QString, QList<CoprPackageInfo>> m_packagePages;
    static constexpr int PackagesPageSize = 100;
    // What is kept per project from the package list and from the monitor. The monitor
    // is not paginated: iucar/cran answers with 12.5 MB for 24500 packages.
    static constexpr int MaxPackagesPerProject = 500;
    static constexpr qint64 MaxMonitorBytes = 2 * 1024 * 1024;

    // Response cache with TTL
    struct CacheEntry {
        QJsonObject data;
        qint64 timestamp;
        qint64 size; // of the raw response
        int ttlMs;
    };
    QHash<QString, CacheEntry> m_cache;
    static constexpr int CacheTtlMs = 300000; // 5 minutes
    // A search costs the server 6-8 s and its result changes only when projects are
    // created or deleted. What can be installed is never taken from it.
    static constexpr int SearchCacheTtlMs = 1800000; // 30 minutes
    static constexpr qint64 MaxCacheBytes = 4 * 1024 * 1024; // about 8 project list pages
};

#endif // COPRCLIENT_H
