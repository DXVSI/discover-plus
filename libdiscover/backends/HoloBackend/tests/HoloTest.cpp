/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#include <ApplicationAddonsModel.h>
#include <Category/CategoryModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/DiscoverAction.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/SourcesModel.h>

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "libdiscover_holo_debug.h"

#include "atomupd1_adaptor.h"
#include "mock_server.h"

#include "HoloTest.h"

AbstractResourcesBackend *HoloTest::backendByName(ResourcesModel *m, const QString &name)
{
    const QVector<AbstractResourcesBackend *> backends = m->backends();
    for (AbstractResourcesBackend *backend : backends) {
        if (QLatin1String(backend->metaObject()->className()) == name) {
            return backend;
        }
    }
    return nullptr;
}

HoloTest::HoloTest(QObject *parent)
    : QObject(parent)
{
    QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-holo-test")).removeRecursively();

    m_server = new MockServer();
    m_adaptor = new Atomupd1Adaptor(m_server);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QLatin1String("/com/steampowered/Atomupd1"), m_server);
    dbus.registerService(QLatin1String("com.steampowered.Atomupd1"));
    QStandardPaths::setTestModeEnabled(true);
    qputenv("HOLO_TEST_MODE", "ON");
    m_model = new ResourcesModel(QStringLiteral("holo-backend"), this);
    m_appBackend = backendByName(m_model, QStringLiteral("HoloBackend"));
}

void HoloTest::initTestCase()
{
    QVERIFY(m_appBackend);
    if (!m_appBackend || !m_appBackend->isValid()) {
        qWarning() << "couldn't run the test";
        exit(0);
    }

    // Check both MockServer and dbus interface to Version are working
    Q_ASSERT(m_server->currentVersion() == TEST_CURRENT_VERSION);
    Q_ASSERT(m_adaptor->currentVersion() == TEST_CURRENT_VERSION);

    // Check buildid also
    Q_ASSERT(m_server->currentBuildID() == TEST_CURRENT_BUILDID);
    Q_ASSERT(m_adaptor->currentBuildID() == TEST_CURRENT_BUILDID);

    // Check branch next
    Q_ASSERT(m_server->branch() == TEST_CURRENT_BRANCH);
    Q_ASSERT(m_adaptor->branch() == TEST_CURRENT_BRANCH);

    // Next test default status
    Q_ASSERT(m_server->updateStatus() == AU_UPDATE_STATUS_IDLE);
    Q_ASSERT(m_adaptor->updateStatus() == AU_UPDATE_STATUS_IDLE);

    // Now wait until it's done checking for updates
    while (m_appBackend->fetchingUpdatesProgress() < 100) {
        QCoreApplication::processEvents();
    }
}

void HoloTest::testUpdateInProgress()
{
    // Then change it and check change stuck.
    m_server->setUpdateStatus(AU_UPDATE_STATUS_IN_PROGRESS);
    Q_ASSERT(m_server->updateStatus() == AU_UPDATE_STATUS_IN_PROGRESS);
    Q_ASSERT(m_adaptor->updateStatus() == AU_UPDATE_STATUS_IN_PROGRESS);

    // Now check the holo backend's reaction matches
    QSignalSpy spyNewTransaction(TransactionModel::global(), &TransactionModel::transactionAdded);
    m_server->setUpdatesAvailable(true);
    m_appBackend->checkForUpdates();

    spyNewTransaction.wait(std::chrono::seconds(1));

    QCOMPARE(spyNewTransaction.count(), 1);
}
QTEST_GUILESS_MAIN(HoloTest)
