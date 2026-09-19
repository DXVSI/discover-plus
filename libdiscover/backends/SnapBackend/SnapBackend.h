/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QPointer>
#include <QThreadPool>
#include <QVariantList>
#include <QVector>
#include <Snapd/Client>
#include <functional>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>

class OdrsReviewsBackend;
class StandardBackendUpdater;
class SnapResource;
class SnapBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit SnapBackend(QObject *parent = nullptr);
    ~SnapBackend() override;

    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);

    QString displayName() const override;
    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    bool isValid() const override
    {
        return m_valid;
    }

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    void checkForUpdates() override;
    bool hasApplications() const override
    {
        return true;
    }
    QSnapdClient *client()
    {
        return &m_client;
    }
    void refreshStates();
    int fetchingUpdatesProgress() const override
    {
        return m_updatesFetcher ? 42 : 100;
    }

Q_SIGNALS:
    void shuttingDown();

private:
    template<class T>
    ResultsStream *populateWithFilter(T *snaps, std::function<bool(const QSharedPointer<QSnapdSnap> &)> &filter);

    template<class T>
    ResultsStream *populateJobsWithFilter(const QVector<T *> &snaps, std::function<bool(const QSharedPointer<QSnapdSnap> &)> &filter);

    template<class T>
    ResultsStream *populate(T *snaps);

    template<class T>
    ResultsStream *populate(const QVector<T *> &snaps);

    QHash<QString, SnapResource *> m_resources;
    StandardBackendUpdater *m_updater;
    QSharedPointer<OdrsReviewsBackend> m_reviews;

    bool m_valid = true;
    QSnapdClient m_client;
    QThreadPool m_threadPool;
    QPointer<AggregatedResultsStream> m_updatesFetcher;
};
