/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "FlatpakResource.h"

#include <QSharedPointer>
#include <QThreadPool>
#include <QVariantList>
#include <resources/AbstractResourcesBackend.h>

#include <AppStreamQt/component.h>

#include <QCoroTask>

#include "FlatpakRefreshAppstreamMetadataJob.h"
#include "flatpak-helper.h"

class FlatpakSourcesBackend;
class FlatpakSource;
class StandardBackendUpdater;
class OdrsReviewsBackend;

namespace AppStream
{
class Pool;
}

namespace Utils
{
/// Useful for when libflatpak returns strings that need to be freed
QString copyAndFree(char *str);
class ProgressCollector;
}

class FlatpakBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit FlatpakBackend(QObject *parent = nullptr);
    ~FlatpakBackend();

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);
    bool isValid() const override;
    int fetchingUpdatesProgress() const override;
    uint fetchingUpdatesProgressWeight() const override
    {
        return 50; // Same as packagekit
    }

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override
    {
        return true;
    }
    bool extends(const QString &extends) const override;

    void addSourceFromFlatpakRepo(const QUrl &url, ResultsStream *stream);
    void addAppFromFlatpakBundle(const QUrl &url, ResultsStream *stream);
    void addAppFromFlatpakRef(const QUrl &url, ResultsStream *stream);
    FlatpakResource *getAppForInstalledRef(FlatpakInstallation *flatpakInstallation, FlatpakInstalledRef *ref, bool *freshResource = nullptr) const;

    FlatpakSourcesBackend *sources() const
    {
        return m_sources;
    }

    bool updateAppSize(FlatpakResource *resource);
    FlatpakInstalledRef *getInstalledRefForApp(const FlatpakResource *resource) const;
    void loadRemote(FlatpakInstallation *installation, FlatpakRemote *remote);
    void unloadRemote(FlatpakInstallation *installation, FlatpakRemote *remote);

    InlineMessage *explainDysfunction() const override;

    bool isTracked(FlatpakResource *resource) const;
    QThreadPool *threadPool()
    {
        return &m_threadPool;
    }

    GCancellable *cancellable() const
    {
        return m_cancellable;
    }
    bool isFetching() const
    {
        return m_isFetching != 0;
    }

private Q_SLOTS:
    void onFetchMetadataFinished(FlatpakResource *resource, const QByteArray &metadata);
    void onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize);

Q_SIGNALS: // for tests
    void initialized();

private:
    friend class FlatpakSource;

    void metadataRefreshed(FlatpakRemote *remote);
    bool flatpakResourceLessThan(const StreamResult &left, const StreamResult &right) const;
    bool flatpakResourceLessThan(AbstractResource *left, AbstractResource *right) const;
    FlatpakInstallation *preferredInstallation() const
    {
        return m_installations.constFirst();
    }
    QSharedPointer<FlatpakSource> integrateRemote(GLibHolder<FlatpakInstallation> flatpakInstallation, GLibHolder<FlatpakRemote> remote);
    FlatpakResource *getRuntimeForApp(FlatpakResource *resource) const;
    FlatpakResource *resourceForComponent(const AppStream::Component &component, const QSharedPointer<FlatpakSource> &source) const;
    void checkRepositories(const QMap<QString, QStringList> &repositories);

    void loadAppsFromAppstreamData();
    bool loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation);
    void loadLocalUpdates();
    void loadLocalUpdates(FlatpakInstallation *flatpakInstallation);
    bool setupFlatpakInstallations(GError **error);
    void updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource, const QByteArray &data);
    bool updateAppMetadata(FlatpakResource *resource, const QString &path);
    bool updateAppSizeFromRemote(FlatpakResource *resource);
    void updateAppState(FlatpakResource *resource);
    QSharedPointer<FlatpakSource> findSource(FlatpakInstallation *installation, const QString &origin) const;
    QSharedPointer<FlatpakSource> findSourceUrl(FlatpakInstallation *installation, const QString &url) const;

    QVector<StreamResult> resultsByAppstreamName(const QString &name) const;
    void acquireFetching(bool f);
    void checkForRemoteUpdates(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote);
    void createPool(QSharedPointer<FlatpakSource> source);
    FlatpakRemote *installSource(FlatpakResource *resource);

    ResultsStream *deferredResultStream(const QString &streamName, std::function<QCoro::Task<>(ResultsStream *)> callback);
    ResultsStream *deferredResultStreamNoFinish(const QString &streamName, std::function<QCoro::Task<>(ResultsStream *)> callback);
    // Returned Installation and Ref objects are g_object_ref'ed, caller is responsible for calling g_object_unref.
    QCoro::Task<QHash<FlatpakInstallation *, QList<FlatpakInstalledRef *>>> listInstalledRefsForUpdate();

    StandardBackendUpdater *m_updater;
    FlatpakSourcesBackend *m_sources = nullptr;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
    uint m_isFetching = 0;
    QSet<FlatpakRemote *> m_refreshAppstreamMetadataJobs;

    GCancellable *m_cancellable;
    QVector<FlatpakInstallation *> m_installations;
    QThreadPool m_threadPool;
    QVector<QSharedPointer<FlatpakSource>> m_flatpakSources;
    QVector<QSharedPointer<FlatpakSource>> m_flatpakLoadingSources;
    QSharedPointer<FlatpakSource> m_localSource;
    QTimer *const m_checkForUpdatesTimer;

    friend class Utils::ProgressCollector;
    Utils::ProgressCollector *const m_collector;
};
