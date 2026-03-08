/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#include "mock_server.h"

#define TEST_VARIANT QLatin1String("steamdeck")
#define VARIANT_KEY QLatin1String("variant")
#define VERSION_KEY QLatin1String("version")
#define SIZE_KEY QLatin1String("estimated_size")
#define TEST_SIZE 1865596133

MockServer::MockServer()
    : m_updateStatus(AU_UPDATE_STATUS_IDLE)
    , m_updatesAvailable(false)
{
}

MockServer::~MockServer()
{
}

QString MockServer::currentVersion() const
{
    return TEST_CURRENT_VERSION;
}

QString MockServer::currentBuildID() const
{
    return TEST_CURRENT_BUILDID;
}

QString MockServer::updateVersion() const
{
    return TEST_UPDATE_VERSION;
}

QString MockServer::updateBuildID() const
{
    return TEST_UPDATE_BUILDID;
}

QString MockServer::branch() const
{
    return TEST_CURRENT_BRANCH;
}

int MockServer::updateStatus() const
{
    return m_updateStatus;
}

void MockServer::setUpdateStatus(int status)
{
    if (status < 0 || status > AU_UPDATE_STATUS_CANCELLED) {
        // Out of range, so do nothing
        return;
    }
    m_updateStatus = static_cast<AuUpdateStatus>(status);
    Q_EMIT updateStatusChanged();
}

void MockServer::setUpdatesAvailable(bool available)
{
    m_updatesAvailable = available;
}

VariantMapMap MockServer::CheckForUpdates(const QVariantMap & /* options */, VariantMapMap & /* updates_available_later */)
{
    VariantMapMap result;
    qDebug() << "CheckForUpdates called, m_updatesAvailable: " << m_updatesAvailable;
    if (m_updatesAvailable) {
        QVariantMap map;
        map.insert(VERSION_KEY, TEST_UPDATE_VERSION);
        map.insert(VARIANT_KEY, TEST_VARIANT);
        map.insert(SIZE_KEY, TEST_SIZE);
        result.insert(TEST_UPDATE_BUILDID, map);
    }
    return result;
}
