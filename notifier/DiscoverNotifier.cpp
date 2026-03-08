/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DiscoverNotifier.h"
#include "BackendNotifierFactory.h"
#include "UnattendedUpdates.h"
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KPluginFactory>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDebug>
#include <QNetworkInformation>
#include <QProcess>

#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>

#include "../libdiscover/utils.h"
#include "../libdiscover/UpdateModel/RefreshNotifierDBus.h"
#include "Login1ManagerInterface.h"
#include "updatessettings.h"
#include <chrono>

#include "debug.h"

using namespace std::chrono_literals;
using namespace Qt::Literals;

namespace
{
bool isOnline()
{
    return !QNetworkInformation::instance() || QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online;
}
} // namespace

DiscoverNotifier::DiscoverNotifier(const std::chrono::seconds &checkDelay, QObject *parent)
    : QObject(parent)
    , m_stateConfig(u"discovernotifierstaterc"_s, KConfig::SimpleConfig, QStandardPaths::GenericStateLocation)
{
    m_settings = std::make_unique<UpdatesSettings>();
    m_settingsWatcher = KConfigWatcher::create(m_settings->sharedConfig());

    KConfigGroup stateGroup = m_stateConfig.group(u"Global"_s);
    settings()->config()->group(u"Global"_s).moveValuesTo({"LastNotificationTime"}, stateGroup);
    settings()->save();

    QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability | QNetworkInformation::Feature::TransportMedium);
    if (auto info = QNetworkInformation::instance()) {
        connect(info, &QNetworkInformation::reachabilityChanged, this, &DiscoverNotifier::stateChanged);
        connect(info, &QNetworkInformation::transportMediumChanged, this, &DiscoverNotifier::stateChanged);
        connect(info, &QNetworkInformation::isBehindCaptivePortalChanged, this, &DiscoverNotifier::stateChanged);
    } else {
        qWarning() << "QNetworkInformation has no backend. Is NetworkManager.service running?";
    }

    refreshUnattended();
    connect(m_settingsWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.config()->name() != m_settings->config()->name() || group.name() != QLatin1String("Global")) {
            return;
        }
        if (names.contains("UseUnattendedUpdates") || names.contains("RequiredNotificationInterval")) {
            refreshUnattended();
            Q_EMIT stateChanged();
        }
    });

    m_backends = BackendNotifierFactory().allBackends();
    for (BackendNotifierModule *module : std::as_const(m_backends)) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &DiscoverNotifier::updateStatusNotifier);
        connect(module, &BackendNotifierModule::needsRebootChanged, this, [this]() {
            // If we are using offline updates, there is no need to badger the user to
            // reboot since it is safe to continue using the system in its current state
            if (!m_needsReboot && !m_settings->useUnattendedUpdates()) {
                m_needsReboot = true;
                showRebootNotification();
                Q_EMIT stateChanged();
                Q_EMIT needsRebootChanged(true);
            }
        });

        connect(module, &BackendNotifierModule::foundUpgradeAction, this, &DiscoverNotifier::foundUpgradeAction);
    }
    connect(&m_timer, &QTimer::timeout, this, &DiscoverNotifier::showUpdatesNotification);
    m_timer.setSingleShot(true);
    m_timer.setInterval(1s);
    updateStatusNotifier();

    // Only fetch updates after the system is comfortably booted
    QTimer::singleShot(checkDelay, this, &DiscoverNotifier::recheckSystemUpdateNeeded);

    auto login1 = new OrgFreedesktopLogin1ManagerInterface(QStringLiteral("org.freedesktop.login1"),
                                                           QStringLiteral("/org/freedesktop/login1"),
                                                           QDBusConnection::systemBus(),
                                                           this);
    connect(login1, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, this, [this, checkDelay](bool sleeping) {
        // if we just woke up and have not checked in the notification interval
        if (!sleeping && m_lastUpdate.secsTo(QDateTime::currentDateTimeUtc()) > m_settings->requiredNotificationInterval()) {
            QTimer::singleShot(checkDelay, this, &DiscoverNotifier::recheckSystemUpdateNeeded);
        }
    });

    // Listen to broadcasts from discover about notification changes.
    QDBusConnection::sessionBus().connect(QString(),
                                          RefreshNotifierDBus::path,
                                          RefreshNotifierDBus::interface,
                                          RefreshNotifierDBus::notifyNotifier,
                                          this,
                                          SLOT(recheckSystemUpdateNeeded()));
}

DiscoverNotifier::~DiscoverNotifier() = default;

void DiscoverNotifier::showDiscover(const QString &xdgActivationToken)
{
    auto *job = new KIO::ApplicationLauncherJob(KService::serviceByDesktopName(QStringLiteral("org.kde.discover")));
    job->setStartupId(xdgActivationToken.toUtf8());
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();

    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }
}

void DiscoverNotifier::showDiscoverUpdates(const QString &xdgActivationToken)
{
    auto *job = new KIO::CommandLauncherJob(QStringLiteral("plasma-discover"), {QStringLiteral("--mode"), QStringLiteral("update")});
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->setDesktopName(QStringLiteral("org.kde.discover"));
    job->setStartupId(xdgActivationToken.toUtf8());
    job->start();

    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }
}

bool DiscoverNotifier::checkTriggerTimes(const QDateTime &lastTriggerTime) const
{
    if (state() != NormalUpdates && state() != SecurityUpdates) {
        // it's not very helpful to notify that everything is in order
        qCDebug(NOTIFIER) << "Not triggering, state is" << state();
        return false;
    }

    if (m_settings->requiredNotificationInterval() < 0) {
        qCDebug(NOTIFIER) << "Not triggering, requiredNotificationInterval is" << m_settings->requiredNotificationInterval();
        return false;
    }

    // To configure to a random value, execute:
    // kwriteconfig5 --file PlasmaDiscoverUpdates --group Global --key RequiredNotificationInterval 3600
    const QDateTime earliestNextTriggerTime = lastTriggerTime.addSecs(m_settings->requiredNotificationInterval());
    if (earliestNextTriggerTime.isValid() && earliestNextTriggerTime > QDateTime::currentDateTimeUtc()) {
        qCDebug(NOTIFIER) << "Not triggering, earliestNextTriggerTime is" << earliestNextTriggerTime;
        return false;
    }

    return true;
}

QDateTime DiscoverNotifier::lastNotificationTime() const
{
    return m_stateConfig.group(u"Global"_s).readEntry("LastNotificationTime", QDateTime());
}

bool DiscoverNotifier::notifyAboutUpdates()
{
    if (!checkTriggerTimes(lastNotificationTime())) {
        return false;
    }

    m_stateConfig.group(u"Global"_s).writeEntry("LastNotificationTime", QDateTime::currentDateTimeUtc());
    m_stateConfig.sync();

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        qCDebug(NOTIFIER) << "Not notifying about updates, discover is running";
        return false;
    }
    return true;
}

bool DiscoverNotifier::proceedUnattended() const
{
    if (!checkTriggerTimes(m_settings->lastUnattendedTrigger())) {
        return false;
    }

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        qCDebug(NOTIFIER) << "Not proceeding with unattended update, discover is running";
        return false;
    }
    return true;
}

void DiscoverNotifier::showUpdatesNotification()
{
    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }

    if (!notifyAboutUpdates()) {
        qCDebug(NOTIFIER) << "showUpdatesNotification: not notifying about updates";
        return;
    }
    qCDebug(NOTIFIER) << "showUpdatesNotification: notifying about updates";

    m_updatesAvailableNotification =
        KNotification::event(QStringLiteral("Update"), message(), {}, iconName(), KNotification::CloseOnTimeout, QStringLiteral("discoverabstractnotifier"));
    m_updatesAvailableNotification->setHint(QStringLiteral("resident"), true);
    const QString name = i18n("View Updates");

    auto showUpdates = [this] {
        showDiscoverUpdates(m_updatesAvailableNotification->xdgActivationToken());
    };

    auto defaultAction = m_updatesAvailableNotification->addDefaultAction(name);
    connect(defaultAction, &KNotificationAction::activated, this, showUpdates);

    auto showUpdatesAction = m_updatesAvailableNotification->addAction(name);
    connect(showUpdatesAction, &KNotificationAction::activated, this, showUpdates);
}

void DiscoverNotifier::updateStatusNotifier()
{
    const bool hasSecurityUpdates = kContains(m_backends, [](BackendNotifierModule *module) {
        return module->hasSecurityUpdates();
    });
    const bool hasUpdates = hasSecurityUpdates || kContains(m_backends, [](BackendNotifierModule *module) {
                                return module->hasUpdates();
                            });

    qCDebug(NOTIFIER) << "updateStatusNotifier: hasUpdates" << hasUpdates << "hasSecurityUpdates" << hasSecurityUpdates;

    if (m_hasUpdates == hasUpdates && m_hasSecurityUpdates == hasSecurityUpdates)
        return;

    m_hasSecurityUpdates = hasSecurityUpdates;
    m_hasUpdates = hasUpdates;

    if (state() != NoUpdates) {
        m_timer.start();
    }

    Q_EMIT stateChanged();
}

// we only want to do unattended updates when on an ethernet or wlan network
static bool isConnectionAdequate()
{
    const auto info = QNetworkInformation::instance();
    if (!info) {
        return false; // no backend available (e.g. NetworkManager not running), assume not adequate
    }
    if (info->supports(QNetworkInformation::Feature::CaptivePortal) && info->isBehindCaptivePortal()) {
        return false;
    }
    if (info->supports(QNetworkInformation::Feature::Metered)) {
        return !info->isMetered();
    } else {
        const auto transport = info->transportMedium();
        return transport == QNetworkInformation::TransportMedium::Ethernet || transport == QNetworkInformation::TransportMedium::WiFi;
    }
}

bool DiscoverNotifier::isSystemUpdateable() const
{
    const bool updateable = !m_isBusy && isOnline() && (m_hasSecurityUpdates || m_hasUpdates);
    qCDebug(NOTIFIER) << "isSystemUpdateable:" << updateable << "isBusy:" << m_isBusy << "isOnline:" << isOnline() << "securityUpdates:" << m_hasSecurityUpdates
                      << "otherUpdates:" << m_hasUpdates;
    return updateable;
}

void DiscoverNotifier::startUnattendedUpdates()
{
    auto process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "Error running plasma-discover" << error;
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Finished running plasma-discover" << exitCode << exitStatus;
        process->deleteLater();
        if (exitCode == 0) {
            settings()->setLastUnattendedTrigger(QDateTime::currentDateTimeUtc());
        }
        settings()->save();
        setBusy(false);
    });

    setBusy(true);
    process->start(QStringLiteral("plasma-discover"), {QStringLiteral("--headless-update")});
    qInfo() << "started unattended update" << QDateTime::currentDateTimeUtc();
}

void DiscoverNotifier::refreshUnattended()
{
    m_settings->read();

    if (!proceedUnattended()) {
        qCDebug(NOTIFIER) << "refreshUnattended: not notifying about updates";
        return;
    }

    qCDebug(NOTIFIER) << "refreshUnattended: useUnattendedUpdates" << m_settings->useUnattendedUpdates() << "isOnline" << isOnline() << "isConnectionAdequate"
                      << isConnectionAdequate();
    const auto enabled = m_settings->useUnattendedUpdates() && isOnline() && isConnectionAdequate();
    if (bool(m_unattended) == enabled)
        return;

    if (enabled) {
        qCDebug(NOTIFIER) << "Enabling unattended updates";
        m_unattended = std::make_unique<UnattendedUpdates>(this);
    } else {
        qCDebug(NOTIFIER) << "Disabling unattended updates";
        m_unattended.reset();
    }
}

DiscoverNotifier::State DiscoverNotifier::state() const
{
    if (m_needsReboot)
        return RebootRequired;
    else if (m_isBusy)
        return Busy;
    else if (!isOnline())
        return Offline;
    else if (m_hasSecurityUpdates)
        return SecurityUpdates;
    else if (m_hasUpdates)
        return NormalUpdates;
    else
        return NoUpdates;
}

QString DiscoverNotifier::iconName() const
{
    switch (state()) {
    case SecurityUpdates:
        return QStringLiteral("update-high");
    case NormalUpdates:
        return QStringLiteral("update-low");
    case NoUpdates:
        return QStringLiteral("update-none");
    case RebootRequired:
        return QStringLiteral("system-reboot-update");
    case Offline:
        return QStringLiteral("offline");
    case Busy:
        return QStringLiteral("update-busy");
    }
    return QString();
}

QString DiscoverNotifier::message() const
{
    switch (state()) {
    case SecurityUpdates:
        return i18n("Security updates available");
    case NormalUpdates:
        return i18n("Updates available");
    case NoUpdates:
        return i18n("System up to date");
    case RebootRequired:
        return i18n("Computer needs to restart");
    case Offline:
        return i18n("Offline");
    case Busy:
        return i18n("Applying unattended updates…");
    }
    return QString();
}

void DiscoverNotifier::recheckSystemUpdateNeeded()
{
    m_lastUpdate = QDateTime::currentDateTimeUtc();
    for (BackendNotifierModule *module : std::as_const(m_backends))
        module->recheckSystemUpdateNeeded();

    QTimer::singleShot(20000, this, &DiscoverNotifier::refreshUnattended);
}

QStringList DiscoverNotifier::loadedModules() const
{
    QStringList ret;
    for (BackendNotifierModule *module : m_backends)
        ret += QString::fromLatin1(module->metaObject()->className());
    return ret;
}

void DiscoverNotifier::showRebootNotification()
{
    KNotification *notification = KNotification::event(QStringLiteral("UpdateRestart"),
                                                       i18n("Restart is required"),
                                                       i18n("The system needs to be restarted for the updates to take effect."),
                                                       QStringLiteral("system-software-update"),
                                                       KNotification::Persistent | KNotification::DefaultEvent,
                                                       QStringLiteral("discoverabstractnotifier"));

    auto restartAction = notification->addAction(i18nc("@action:button", "Update and Restart"));
    auto shutdownAction = notification->addAction(i18nc("@action:button", "Update and Shut Down"));
    connect(restartAction, &KNotificationAction::activated, this, &DiscoverNotifier::rebootPrompt);
    connect(shutdownAction, &KNotificationAction::activated, this, &DiscoverNotifier::shutdownPrompt);

    notification->sendEvent();
}

void DiscoverNotifier::rebootPrompt()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::shutdownPrompt()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptShutDown"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::promptAll()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptAll"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::foundUpgradeAction(UpgradeAction *action)
{
    updateStatusNotifier();

    if (!notifyAboutUpdates()) {
        return;
    }

    KNotification *notification = new KNotification(QStringLiteral("DistUpgrade"), KNotification::Persistent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setTitle(i18n("Upgrade available"));
    notification->setText(i18nc("A new distro release (name and version) is available for upgrade", "%1 is now available.", action->description()));
    notification->setComponentName(QStringLiteral("discoverabstractnotifier"));

    auto upgradeAction = notification->addAction(i18nc("@action:button", "Upgrade"));
    connect(upgradeAction, &KNotificationAction::activated, this, [action] {
        action->trigger();
    });

    connect(action, &UpgradeAction::showDiscoverUpdates, this, [this, notification]() {
        showDiscoverUpdates(notification->xdgActivationToken());
    });

    notification->sendEvent();
}

void DiscoverNotifier::setBusy(bool isBusy)
{
    if (isBusy == m_isBusy)
        return;

    m_isBusy = isBusy;
    Q_EMIT busyChanged();
    Q_EMIT stateChanged();
}

void DiscoverNotifier::recheckSystemUpdateNeededAndNotifyApp()
{
    recheckSystemUpdateNeeded();
    RefreshNotifierDBus::notify(RefreshNotifierDBus::notifyApp);
}

#include "moc_DiscoverNotifier.cpp"
