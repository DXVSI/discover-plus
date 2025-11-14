/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "PackageKitResource.h"

#include <PackageKit/Offline>
#include <PackageKit/Transaction>
#include <QFile>
#include <QPointer>
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
        return !QFile::exists(QStringLiteral("/run/ostree-booted"));
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

    CoprClient *coprClient() const { return m_coprClient; }
    void searchCoprPackages(const QString &query);
    void loadPopularCoprProjects();
    void loadMoreCoprProjects();

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
    void onCoprProjectsFound(const QList<CoprProjectInfo> &projects);
    void onCoprPackagesFound(const QList<CoprPackageInfo> &packages);

Q_SIGNALS:
    void loadedAppStream();
    void available();
    void coprSearchResults(const QList<CoprResource*> &resources);

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

    QScopedPointer<AppStream::ConcurrentPool> m_appdata;
    bool m_appdataLoaded = false;
    PackageKitUpdater *m_updater;
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
    QHash<QString, CoprResource*> m_coprResources;
    QPointer<PKResultsStream> m_currentSearchStream;
    int m_coprOffset = 0;
    QString m_lastCoprSearchQuery;
};
