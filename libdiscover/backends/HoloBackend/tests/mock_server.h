/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#ifndef MOCK_SERVER_H
#define MOCK_SERVER_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

#include "dbushelpers.h"

#define TEST_CURRENT_BRANCH QLatin1String("main")
#define TEST_CURRENT_VERSION QLatin1String("2.7")
#define TEST_CURRENT_BUILDID QLatin1String("20250326.0")

#define TEST_UPDATE_VERSION QLatin1String("2.8")
#define TEST_UPDATE_BUILDID QLatin1String("20250403.0")

/// NOTE: Keep this in sync with values in atomupd-daemon:/utils.h
/**
 * AuUpdateStatus:
 * @AU_UPDATE_STATUS_IDLE: The update has not been launched yet
 * @AU_UPDATE_STATUS_IN_PROGRESS: The update is currently being applied
 * @AU_UPDATE_STATUS_PAUSED: The update has been paused
 * @AU_UPDATE_STATUS_SUCCESSFUL: The update process successfully completed
 * @AU_UPDATE_STATUS_FAILED: An Error occurred during the update
 * @AU_UPDATE_STATUS_CANCELLED: A special case of FAILED where the update attempt
 *  has been cancelled
 */
typedef enum {
    AU_UPDATE_STATUS_IDLE = 0,
    AU_UPDATE_STATUS_IN_PROGRESS = 1,
    AU_UPDATE_STATUS_PAUSED = 2,
    AU_UPDATE_STATUS_SUCCESSFUL = 3,
    AU_UPDATE_STATUS_FAILED = 4,
    AU_UPDATE_STATUS_CANCELLED = 5,
} AuUpdateStatus;

class MockServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString CurrentVersion READ currentVersion)
    Q_PROPERTY(QString CurrentBuildID READ currentBuildID)
    Q_PROPERTY(QString UpdateVersion READ updateVersion)
    Q_PROPERTY(QString UpdateBuildID READ updateBuildID)
    Q_PROPERTY(QString Branch READ branch)
    Q_PROPERTY(int UpdateStatus READ updateStatus WRITE setUpdateStatus NOTIFY updateStatusChanged)
public:
    MockServer();
    virtual ~MockServer();

    QString currentVersion() const;
    QString currentBuildID() const;
    QString updateVersion() const;
    QString updateBuildID() const;
    QString branch() const;
    int updateStatus() const;
    void setUpdateStatus(int status);

    VariantMapMap CheckForUpdates(const QVariantMap &options, VariantMapMap &updates_available_later);

    // Set whether to report updates are available or not
    void setUpdatesAvailable(bool available);

Q_SIGNALS:
    void updateStatusChanged();

private:
    AuUpdateStatus m_updateStatus;
    bool m_updatesAvailable;
};

#endif
