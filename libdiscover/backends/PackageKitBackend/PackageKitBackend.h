/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "PackageKitResource.h"

#include <PackageKit/Offline>
#include <PackageKit/Transaction>
#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <QVariantList>

#include <appstream/AppStreamConcurrentPool.h>
#include <resources/AbstractResourcesBackend.h>

class AppPackageKitResource;
class PackageKitUpdater;
class PackageKitSourcesBackend;
class OdrsReviewsBackend;
class PKResultsStream;
class PKResolveTransaction;
class CoprClient;
class CoprResource;
struct CoprProjectInfo;
struct CoprPackageInfo;

/** This is either a package name or an appstream id */
struct PackageOrAppId {
    QString id;
    bool isPackageName;
};
PackageOrAppId makePackageId(const QString &id);
PackageOrAppId makeAppId(const QString &id);

class Delay : public QObject
{
    Q_OBJECT
public:
    Delay();
    void add(const QString &pkgid)
    {
        if (!m_delay.isActive()) {
            m_delay.start();
        }

        m_pkgids << pkgid;
    }
    void add(const QSet<QString> &pkgids)
    {
        if (!m_delay.isActive()) {
            m_delay.start();
        }

        m_pkgids += pkgids;
    }

Q_SIGNALS:
    void perform(const QSet<QString> &pkgids);

private:
    QTimer m_delay;
    QSet<QString> m_pkgids;
};

class DISCOVERCOMMON_EXPORT PackageKitBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit PackageKitBackend(QObject *parent = nullptr);
    ~PackageKitBackend() override;

    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    QSet<AbstractResource *> resourcesByPackageName(const QString &name) const;

    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    PKResultsStream *findResourceByPackageName(const QUrl &search);
    int updatesCount() const override;
    bool hasSecurityUpdates() const override;

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    bool isValid() const override
    {
        return true;
    }
    QSet<AbstractResource *> upgradeablePackages() const;

    bool isPackageNameUpgradeable(const PackageKitResource *res) const;
    QSet<QString> upgradeablePackageId(const PackageKitResource *res) const;
    QVector<AbstractResource *> extendedBy(const QString &id) const;

    PKResolveTransaction *resolvePackages(const QStringList &packageNames);
    void fetchDetails(const QString &pkgid);
    void fetchDetails(const QSet<QString> &pkgid);

    void checkForUpdates() override;
    QString displayName() const override;

    bool hasApplications() const override
    {
        return true;
    }
    static QString locateService(const QString &filename);

    AppStream::ComponentBox componentsById(const QString &id) const;
    void fetchUpdates();
    int fetchingUpdatesProgress() const override;
    uint fetchingUpdatesProgressWeight() const override;

    InlineMessage *explainDysfunction() const override;

    void addPackageArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
    void addPackageNotArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
    void clear()
    {
        m_updatesPackageId.clear();
    }
    Delay &updateDetails()
    {
        return m_updateDetails;
    }
    template<typename T, typename W>
    T resourcesByPackageNames(const W &names) const;

    QStringList globalHints()
    {
        return m_globalHints;
    }
    void aboutTo(AboutToAction action) override;
    bool needsRebootForPowerOffAction() const override
    {
        return true;
    }
    QPointer<PackageKit::Transaction> refresher() const
    {
        return m_refresher;
    }

    CoprClient *coprClient() const
    {
        return m_coprClient;
    }
    void searchCoprPackages(const QString &query);
    void loadPopularCoprProjects();
    void loadMoreCoprProjects();
    void requestCoprInstalledStateCheck(CoprResource *resource);
    void setCoprInstalledStateCache(const QString &owner, const QString &packageName, bool installed);
    void refreshSources();

public Q_SLOTS:
    void reloadPackageList();
    void transactionError(PackageKit::Transaction::Error, const QString &message);

private Q_SLOTS:
    void getPackagesFinished();
    void addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary, bool arch);
    void packageDetails(const PackageKit::Details &details);
    void addPackageToUpdate(PackageKit::Transaction::Info, const QString &pkgid, const QString &summary);
    void getUpdatesFinished(PackageKit::Transaction::Exit, uint);
    void loadAllPackages();
    void loadAllPackagesHybrid();
    void onCoprProjectsFound(const QList<CoprProjectInfo> &projects);
    void onCoprProjectFound(const CoprProjectInfo &project);
    void onCoprProjectNotFound(const QString &owner, const QString &project);
    void onCoprProjectPackagesFound(const QString &owner, const QString &project, const QList<CoprPackageInfo> &packages, bool complete);
    void onCoprProjectMonitorFound(const QString &owner, const QString &project, const QList<CoprPackageInfo> &packages, bool complete);

Q_SIGNALS:
    void loadedAppStream();
    void available();

private:
    friend class PackageKitResource;

    template<typename T, typename W>
    T resourcesByAppNames(const W &names) const;

    template<typename T>
    T resourcesByComponents(const AppStream::ComponentBox &names) const;

    QVector<StreamResult> resultsByComponents(const AppStream::ComponentBox &names) const;

    PKResultsStream *deferredResultStream(const QString &streamName, std::function<void(PKResultsStream *)> callback);

    void checkDaemonRunning();
    void acquireFetching(bool f);
    void includePackagesToAdd();
    void performDetailsFetch(const QSet<QString> &pkgids);
    AppPackageKitResource *addComponent(const AppStream::Component &component) const;
    void updateProxy();
    void foundNewMajorVersion(const AppStream::Release &release);
    void setRefresher(PackageKit::Transaction *refresh);
    void processNextCoprInstalledStateCheck();
    void requestNextCoprBrowsePage();
    void requestNextCoprSearchPage();
    void resetCoprStreamState();
    CoprResource *coprProjectResource(const CoprProjectInfo &project);
    void showCoprMessageOnce(const QString &kind, const QString &message);
    QList<CoprResource *> coprResourcesOfProject(const QString &owner, const QString &project) const;

    QScopedPointer<AppStream::ConcurrentPool> m_appdata;
    bool m_appdataLoaded = false;
    PackageKitUpdater *m_updater;
    PackageKitSourcesBackend *m_sourcesBackend = nullptr;
    QPointer<PackageKit::Transaction> m_refresher;
    int m_isFetching;
    QSet<QString> m_updatesPackageId;
    bool m_hasSecurityUpdates = false;
    mutable QHash<PackageOrAppId, PackageKitResource *> m_packagesToAdd;
    QSet<PackageKitResource *> m_packagesToDelete;
    bool m_appstreamInitialized = false;

    mutable struct {
        QHash<PackageOrAppId, AbstractResource *> packages;
        QHash<QString, QStringList> packageToApp;
    } m_packages;

    Delay m_details;
    Delay m_updateDetails;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
    QThreadPool m_threadPool;
    QPointer<PKResolveTransaction> m_resolveTransaction;
    QStringList m_globalHints;
    bool m_allPackagesLoaded = false;
    CoprClient *m_coprClient = nullptr;
    QHash<QString, CoprResource *> m_coprResources;
    QPointer<PKResultsStream> m_currentSearchStream;
    int m_coprOffset = 0;
    QString m_lastCoprSearchQuery;
    // Search mode. The server is asked for the query itself, or for "owner/project" when
    // the query names one project: a full name, a link to its page or the command that
    // enables it. That project is looked up first and m_coprSearchOwner is set then.
    // m_coprSearchName is what the names of the results are compared with.
    QString m_coprSearchServerQuery;
    QString m_coprSearchOwner;
    QString m_coprSearchName;
    bool m_coprSearchPagePending = false;
    bool m_coprSearchExhausted = false;
    int m_coprSearchRequests = 0;
    QSet<QString> m_coprSearchSeenKeys;
    // A shorter query costs the server a full search of about 8 s and returns junk
    static constexpr int CoprSearchMinimumLength = 3;
    // The cost of a search does not depend on the limit (about 7 s, 2 KB per result), and
    // the server sorts by creation date: what a page cuts off are the oldest projects,
    // often the established ones. A full page is therefore followed by one more at once,
    // further ones wait for a fetchMore.
    static constexpr int CoprSearchPageSize = 100;
    static constexpr int CoprSearchAutomaticPages = 2;
    struct CoprInstalledStateRequest {
        QPointer<CoprResource> resource;
        CoprResource *resourceKey = nullptr;
        QString key;
        QString packageName;
        QString owner;
    };
    QQueue<CoprInstalledStateRequest> m_coprInstalledStateQueue;
    QHash<CoprResource *, QString> m_coprInstalledStatePendingKeys;
    QHash<QString, bool> m_coprInstalledStateCache;
    int m_activeCoprInstalledStateChecks = 0;
    static constexpr int MaxConcurrentCoprInstalledStateChecks = 2;
    // Every automatically selected package of a list item asks for an rpm process
    static constexpr int MaxQueuedCoprInstalledStateChecks = 100;

    // Browse mode. About 90% of the newest projects are hidden from the COPR
    // homepage (CI scratch projects) or lack the current chroot, so one user
    // action (opening the page, a fetchMore) requests large pages one after
    // another until about a screenful passed the filters, up to a hard cap.
    bool m_coprBrowsePagePending = false;
    // The order the list is browsed in: by name instead of newest first
    bool m_coprBrowseByName = false;
    bool m_coprBrowseExhausted = false;
    int m_coprBrowseRequests = 0;
    int m_coprBrowseAccepted = 0;
    QSet<QString> m_coprBrowseSeenKeys;
    // What the browse stream was given so far, to hand over to a stream that replaces it
    QVector<StreamResult> m_coprBrowseResults;
    static constexpr int CoprBrowsePageSize = 300;
    static constexpr int CoprBrowseTargetCount = 30;
    static constexpr int CoprBrowseMaxRequestsPerAction = 4;

    QString m_lastCoprMessageKind;
    QElapsedTimer m_lastCoprMessageTimer;
};
