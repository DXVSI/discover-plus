/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "HoloBackend.h"
#include "HoloResource.h"
#include "HoloTransaction.h"

#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KOSRelease>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QTimer>

#include "atomupd1.h"
#include "libdiscover_holo_debug.h"

DISCOVER_BACKEND_PLUGIN(HoloBackend)

// We expect 2 results, updates and later updates
#define CHECK_UPDATES_RETURN_COUNT 2
#define ATOMUPD_SERVICE_PATH "/usr/lib/systemd/system/atomupd.service"

QString HoloBackend::service()
{
    return QStringLiteral("com.steampowered.Atomupd1");
}

QString HoloBackend::path()
{
    return QStringLiteral("/com/steampowered/Atomupd1");
}

HoloBackend::HoloBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_updateVersion()
    , m_updateSize(0)
    , m_resource(nullptr)
    , m_transaction(nullptr)
    , m_testing(false)
{
    qDBusRegisterMetaType<VariantMapMap>();
    const KOSRelease os;
    QString label = os.prettyName();

    // NOTE: Some translations have localised renditions of the core OS
    // names (Holo and SteamOS).
    // Arabic and Punjabi are current examples of this:
    // NOTE: we explicitly check for these two names instead of just passing
    // label so that the i18n framework knows they can be translated:
    if (label == QStringLiteral("SteamOS"))
        m_label = i18n("SteamOS");
    else if (label == QStringLiteral("Holo"))
        m_label = i18n("Holo");
    else
        m_label = label;

    // Honour the old test mode variable so we don't trigger real updates
    // on old test setups
    if (qEnvironmentVariableIsSet("HOLO_TEST_MODE") || qEnvironmentVariableIsSet("STEAMOS_TEST_MODE")) {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-holo-test");
        qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "running holo backend on test mode" << path;
        m_testing = true;
    }

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &HoloBackend::updatesCountChanged);

    // Use the session bus when testing.
    if (m_testing) {
        m_interface = new ComSteampoweredAtomupd1Interface(service(), path(), QDBusConnection::sessionBus(), this);
    } else {
        m_interface = new ComSteampoweredAtomupd1Interface(service(), path(), QDBusConnection::systemBus(), this);
    }

    // First try to get the current version, only check valid after that since
    // this could wake up the service
    m_currentVersion = m_interface->currentVersion();
    m_currentBuildID = m_interface->currentBuildID();
    qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: Current version from dbus api: " << m_currentVersion << " and build ID: " << m_currentBuildID;

    // If we got a version property, assume the service is responding and check for updates
    if (!m_currentVersion.isEmpty() && !m_currentBuildID.isEmpty()) {
        if (fetchExistingTransaction()) {
            return;
        }

        checkForUpdates();
    } else {
        qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: Unable to query atomupd for Holo Updates...";
        // Should never happen, since trying to open the interface above starts
        // it, but if this plugin is on non holo devices we should show something
        Q_EMIT passiveMessage(i18nc("%1 is the pretty name of the distro e.g. Holo or SteamOS", "Holo: Unable to query atomupd for %1 Updates...", m_label));
    }
}

void HoloBackend::hasUpdateChanged(bool hasUpdate)
{
    if (hasUpdate) {
        // Create or update resource from m_updateVersion, m_updateBuildID
        if (!m_resource) {
            qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: Creating new HoloResource with build id: " << m_updateBuildID;
            m_resource = new HoloResource(m_updateVersion,
                                          m_label,
                                          m_updateBuildID,
                                          m_updateSize,
                                          QStringLiteral("%1 - %2").arg(m_currentVersion).arg(m_currentBuildID),
                                          this);
        } else {
            qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: Updating HoloResource with new version: " << m_updateVersion
                                                  << " and new build id: " << m_updateBuildID;
            m_resource->setVersion(m_updateVersion);
            m_resource->setBuild(m_updateBuildID);
            Q_EMIT m_resource->versionsChanged();
        }
    } else {
        // Clear or remove any previously created resource
    }

    qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: Updates count is now " << updatesCount();
}

void HoloBackend::needRebootChanged()
{
    // Tell gui we need to reboot
    m_updater->setNeedsReboot(true);
}

void HoloBackend::acquireFetching(bool f)
{
    if (f) {
        m_fetching++;
    } else {
        m_fetching--;
    }

    if (!f && m_fetching == 0) {
        Q_EMIT contentsChanged();
    } else if (f && m_fetching == 1) {
        Q_EMIT fetchingUpdatesProgressChanged();
    }
}

bool HoloBackend::fetchExistingTransaction()
{
    // Do we already know that we have a transaction in progress?
    if (m_transaction) {
        qInfo() << "holo-backend: A transaction is already in progress";
        return true;
    }

    // Is there actually a transaction in progress we don't know about yet?
    uint status = m_interface->updateStatus();
    if (status != Idle) {
        qInfo() << "holo-backend: Found a transaction in progress";
        // We don't check that m_currentlyBootedDeployment is != nullptr here as we expect
        // that the backend is initialized when we're called.
        m_updateVersion = m_interface->updateVersion();
        m_updateBuildID = m_interface->updateBuildID();
        m_currentVersion = m_interface->currentVersion();
        m_currentBuildID = m_interface->currentBuildID();
        m_resource = new HoloResource(m_updateVersion,
                                      m_label,
                                      m_updateBuildID,
                                      m_updateSize,
                                      QStringLiteral("[%1] - %2").arg(m_currentVersion).arg(m_currentBuildID),
                                      this);
        setupTransaction(m_resource);
        TransactionModel::global()->addTransaction(m_transaction);
        return true;
    }

    return false;
}

void HoloBackend::setupTransaction(HoloResource *app)
{
    m_transaction = new HoloTransaction(app, Transaction::InstallRole, m_interface);
    connect(m_transaction, &HoloTransaction::needReboot, this, &HoloBackend::needRebootChanged);
}

void HoloBackend::checkForUpdates()
{
    if (m_fetching) {
        return;
    }

    // If there's no dbus object to talk to, just return
    // TODO: Maybe show a warning/error?
    if (m_currentVersion.isEmpty()) {
        return;
    }

    if (fetchExistingTransaction()) {
        qInfo() << "holo-backend: Not checking for updates while a transaction is in progress";
        return;
    }

    acquireFetching(true);

    qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend-backend::checkForUpdates asking DBus api";

    // We don't send any options for now, dbus api doesn't do anything with them yet anyway
    QDBusPendingReply<VariantMapMap, VariantMapMap> reply = m_interface->CheckForUpdates({});
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &HoloBackend::checkForUpdatesFinished);
}

void HoloBackend::checkForUpdatesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<VariantMapMap, VariantMapMap> reply = *call;
    //            qobject_cast<QDBusPendingReply<VariantMapMap, VariantMapMap>(*call);
    if (call->isError()) {
        Q_EMIT passiveMessage(call->error().message());
        qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: CheckForUpdates error: " << call->error().message();
    } else {
        // Valid response, parse it
        VariantMapMap versions = reply.argumentAt<0>();
        qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend-backend: Versions available: " << versions;
        VariantMapMap laterVersions = reply.argumentAt<1>();

        if (versions.isEmpty()) {
            // No updates
            hasUpdateChanged(false);
        } else {
            m_updateBuildID = versions.keys().at(0);
            QVariantMap data = versions.value(m_updateBuildID);
            m_updateVersion = data.value(QLatin1String("version")).toString();
            m_updateSize = data.value(QLatin1String("estimated_size")).toUInt();
            qCDebug(LIBDISCOVER_BACKEND_HOLO_LOG) << "holo-backend: Data values: " << data.values();
            hasUpdateChanged(true);
        }
    }
    acquireFetching(false);

    call->deleteLater();
}

int HoloBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

// Holo never has any searchable packages or anything, so just always
// give a void stream
ResultsStream *HoloBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    // We only support updates. All targeted searches are not applicable.
    if (!filter.resourceUrl.isEmpty()) {
        return new ResultsStream(QStringLiteral("Holo-empty"), {});
    }

    QVector<StreamResult> res;
    if (m_resource && m_resource->state() >= filter.state) {
        res << StreamResult{m_resource, 0};
    }
    return new ResultsStream(QLatin1String("Holo-stream"), res);
}

QHash<QString, HoloResource *> HoloBackend::resources() const
{
    return m_resources;
}

bool HoloBackend::isValid() const
{
    if (m_testing)
        return true;
    return QFile(QStringLiteral(ATOMUPD_SERVICE_PATH)).exists();
}

AbstractBackendUpdater *HoloBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *HoloBackend::reviewsBackend() const
{
    return nullptr;
}

Transaction *HoloBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);
    return installApplication(app);
}

Transaction *HoloBackend::installApplication(AbstractResource *app)
{
    setupTransaction(static_cast<HoloResource *>(app));
    return m_transaction;
}

Transaction *HoloBackend::removeApplication(AbstractResource *)
{
    qWarning() << "holo-backend: Unsupported operation:" << __PRETTY_FUNCTION__;
    return nullptr;
}

QString HoloBackend::displayName() const
{
    return QStringLiteral("Holo");
}

#include "HoloBackend.moc"
#include "moc_HoloBackend.cpp"
