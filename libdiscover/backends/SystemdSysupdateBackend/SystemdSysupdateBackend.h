/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "SystemdSysupdateResource.h"
#include "sysupdate1.h"

#include <QCoro/QCoroTask>
#include <QNetworkAccessManager>
#include <resources/AbstractResourcesBackend.h>
#include <resources/StandardBackendUpdater.h>

class SystemdSysupdateBackend : public AbstractResourcesBackend
{
    Q_OBJECT

public:
    explicit SystemdSysupdateBackend(QObject *parent = nullptr);

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    bool isValid() const override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    int fetchingUpdatesProgress() const override
    {
        return m_fetchOperationCount > 0 ? 42 : 100;
    }

    uint fetchingUpdatesProgressWeight() const override
    {
        return 55;
    }

    QNetworkAccessManager *networkAccess() const
    {
        return m_nam;
    }

    void checkForUpdates() override;
    QString displayName() const override;

    static QDBusConnection OUR_BUS();

private:
    void beginFetch();
    void endFetch();
    QCoro::Task<> checkForUpdatesAsync();

    int m_fetchOperationCount = 0;
    StandardBackendUpdater *m_updater;

    QList<QPointer<SystemdSysupdateResource>> m_resources;
    QPointer<org::freedesktop::sysupdate1::Manager> m_manager;

    QNetworkAccessManager *m_nam;

Q_SIGNALS:
    void transactionRemoved(qulonglong id, const QDBusObjectPath &path, int status);
};
