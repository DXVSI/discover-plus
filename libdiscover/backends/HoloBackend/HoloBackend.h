/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef HOLOBACKEND_H
#define HOLOBACKEND_H

#include <QDBusMessage>
#include <QPointer>
#include <QVariantList>
#include <resources/AbstractResourcesBackend.h>

class ComSteampoweredAtomupd1Interface;
class QDBusPendingCallWatcher;
class StandardBackendUpdater;
class HoloResource;
class HoloTransaction;

class HoloBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit HoloBackend(QObject *parent = nullptr);

    // Status is one of these from the xml definition:
    //    0 = IDLE, the update has not been launched yet
    //    1 = IN_PROGRESS, the update is currently being applied
    //    2 = PAUSED, the update has been paused
    //    3 = SUCCESSFUL, the update process successfully completed
    //    4 = FAILED, an error occurred during the update
    //    5 = CANCELLED, a special case of FAILED where the update attempt has been cancelled
    enum Status {
        Idle = 0,
        InProgress,
        Paused,
        Successful,
        Failed,
        Cancelled,
    };

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    QHash<QString, HoloResource *> resources() const;
    bool isValid() const override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;

    void checkForUpdates() override;
    QString displayName() const override;
    int fetchingUpdatesProgress() const override
    {
        return m_fetching > 0 ? 42 : 100;
    }

    static QString service();
    static QString path();

public Q_SLOTS:
    void checkForUpdatesFinished(QDBusPendingCallWatcher *call);

private Q_SLOTS:
    void needRebootChanged();

private:
    void hasUpdateChanged(bool hasUpdate);

    void acquireFetching(bool f);

    /* Check if a transaction exists already */
    bool fetchExistingTransaction();

    /* Helper to setup a Transaction and connect all signals/slots */
    void setupTransaction(HoloResource *app);

    QHash<QString, HoloResource *> m_resources;
    StandardBackendUpdater *m_updater;
    uint m_fetching = 0;

    QString m_updateVersion; // Next update version, can use once we get from dbus.
    QString m_updateBuildID; // Next build version.
    quint64 m_updateSize; // Estimated size of next update

    QPointer<HoloResource> m_resource; // Since we only ever have one, cache it.

    /* The current transaction in progress, if any */
    HoloTransaction *m_transaction;

    QPointer<ComSteampoweredAtomupd1Interface> m_interface; // Interface to atomupd dbus api
    QString m_currentVersion;
    QString m_currentBuildID;
    QString m_label;
    bool m_testing;
};

#endif // HOLOBACKEND_H
