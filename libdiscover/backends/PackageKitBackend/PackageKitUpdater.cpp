/*
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "PackageKitUpdater.h"
#include "PackageKitMessages.h"
#include <AppStreamQt/release.h>
#include <appstream/AppStreamIntegration.h>

#include <PackageKit/Daemon>
#include <PackageKit/Offline>
#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>
#include <QIcon>
#include <QSet>

#include <KConfigGroup>
#include <KFormat>
#include <KIO/FileSystemFreeSpaceJob>
#include <KLocalizedString>
#include <KSharedConfig>

#include <algorithm>
#include <optional>

#if !QPK_CHECK_VERSION(1, 1, 4)
#include "pk-offline-private.h"
#endif
#include "libdiscover_backend_packagekit_debug.h"
#include "utils.h"

using namespace Qt::StringLiterals;

int percentageWithStatus(PackageKit::Transaction::Status status, uint percentage)
{
    const auto was = percentage;
    if (status != PackageKit::Transaction::StatusUnknown) {
        static const QMap<PackageKit::Transaction::Status, int> statuses = {
            {PackageKit::Transaction::Status::StatusDownload, 0},
            {PackageKit::Transaction::Status::StatusInstall, 1},
            {PackageKit::Transaction::Status::StatusRemove, 1},
            {PackageKit::Transaction::Status::StatusLoadingCache, 1},
            {PackageKit::Transaction::Status::StatusUpdate, 1},
        };
        const auto idx = statuses.value(status, -1);
        if (idx < 0) {
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Status not present" << status << "among" << statuses.keys() << percentage;
            return -1;
        }
        percentage = (idx * 100 + percentage) / 2 /*the maximum in statuses*/;
    }
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "reporting progress with status:" << status << percentage << was;
    return percentage;
}

static void kRemoveDuplicates(QJsonArray &input, std::function<QString(const QJsonValueRef &)> fetchKey)
{
    QSet<QString> ret;
    for (auto it = input.begin(); it != input.end();) {
        const auto key = fetchKey(*it);
        if (!ret.contains(key)) {
            ret << key;
            ++it;
        } else {
            it = input.erase(it);
        }
    }
}

class SystemUpgrade : public AbstractResource
{
    Q_OBJECT
public:
    explicit SystemUpgrade(PackageKitBackend *backend)
        : AbstractResource(backend)
        , m_backend(backend)
        , m_updateSizeTimer(new QTimer(this))
    {
        connect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, [this](AbstractResource *resource) {
            m_resources.remove(resource);
        });

        m_updateSizeTimer->setInterval(100);
        m_updateSizeTimer->setSingleShot(true);
        connect(m_updateSizeTimer, &QTimer::timeout, this, &SystemUpgrade::refreshResource);
    }

    QString packageName() const override
    {
        return QStringLiteral("discover-offline-upgrade");
    }
    QString name() const override
    {
        if (isDistroUpgrade()) {
            return i18nc("distro upgrade: name version", "%1 %2", AppStreamIntegration::global()->osRelease()->name(), m_distroUpgrade->version());
        }
        return i18n("System upgrade");
    }
    QString comment() override
    {
        return upgradeText();
    }
    QVariant icon() const override
    {
        return QStringLiteral("system-upgrade");
    }
    bool canExecute() const override
    {
        return false;
    }
    void invokeApplication() const override
    {
    }
    State state() override
    {
        return Upgradeable;
    }
    bool hasCategory(const QString & /*category*/) const override
    {
        return {};
    }
    AbstractResource::Type type() const override
    {
        return System;
    }
    bool isRemovable() const override
    {
        return false;
    }

    QVector<PackageKitResource *> withoutDuplicates() const
    {
        QVector<PackageKitResource *> ret;
        QSet<QString> donePkgs;
        for (auto resource : std::as_const(m_resources)) {
            const auto pkResource = qobject_cast<PackageKitResource *>(resource);
            const auto pkgName = pkResource->packageName();
            if (!donePkgs.contains(pkgName)) {
                donePkgs.insert(pkgName);
                ret += pkResource;
            }
        }
        return ret;
    }

    quint64 size() override
    {
        quint64 ret = 0;
        if (isDistroUpgrade()) {
            return ret;
        }

        const auto resources = withoutDuplicates();
        for (auto pkResource : resources) {
            ret += pkResource->size();
        }
        return ret;
    }
    QJsonArray licenses() override
    {
        QJsonArray ret;
        for (auto res : std::as_const(m_resources)) {
            ret += res->licenses();
        }
        kRemoveDuplicates(ret, [](const QJsonValueRef &val) -> QString {
            return val.toObject().value(QLatin1String("name")).toString();
        });
        return ret;
    }
    QString section() override
    {
        return {};
    }
    QString origin() const override
    {
        return {};
    }
    QString author() const override
    {
        return {};
    }
    QList<PackageState> addonsInformation() override
    {
        return {};
    }
    QString upgradeText() const override
    {
        return i18np("1 package will be upgraded", "%1 packages will be upgraded", withoutDuplicates().count());
    }
    QString longDescription() override
    {
        QStringList changes;

        auto resources = withoutDuplicates();
        std::ranges::sort(resources, std::less{}, &PackageKitResource::nameSortKey);

        for (auto resource : std::as_const(resources)) {
            const auto changelog = resource->changelog();
            if (changelog.isEmpty()) {
                changes += i18n("<h3>%1</h3>Upgrade to new version %2<br/>No release notes provided", resource->packageName(), resource->availableVersion());
            } else {
                changes += i18n("<h3>%1</h3>Upgrade to new version %2<br/>Release notes:<blockquote>%3</blockquote>",
                                resource->packageName(),
                                resource->availableVersion(),
                                changelog);
            }
        }

        if (isDistroUpgrade()) {
            changes.prepend(m_distroUpgrade->description());
        }
        return changes.join(QString());
    }
    void fetchChangelog() override
    {
        if (isDistroUpgrade()) {
            return;
        }

        for (auto resource : std::as_const(m_resources)) {
            resource->fetchUpdateDetails();
        }
        Q_EMIT changelogFetched({});
    }

    QString installedVersion() const override
    {
        return i18n("Present");
    }
    QString availableVersion() const override
    {
        return i18n("Future");
    }
    QString sourceIcon() const override
    {
        return QStringLiteral("package-x-generic");
    }
    QDate releaseDate() const override
    {
        return {};
    }

    QSet<AbstractResource *> resources() const
    {
        return m_resources;
    }

    QSet<QString> allPackageNames() const
    {
        QSet<QString> ret;
        for (auto resource : std::as_const(m_resources)) {
            ret += kToSet(qobject_cast<PackageKitResource *>(resource)->allPackageNames());
        }
        return ret;
    }

    void refreshResource()
    {
        Q_EMIT m_backend->resourcesChanged(this, {"size", "license"});
        Q_EMIT updateSizeChanged();
    }

    void setCandidates(const QSet<AbstractResource *> &candidates)
    {
        const auto toDisconnect = (m_resources - candidates);
        for (auto resource : toDisconnect) {
            disconnect(resource, &AbstractResource::sizeChanged, this, &SystemUpgrade::startIfStopped);
            disconnect(resource, &AbstractResource::changelogFetched, this, &SystemUpgrade::startIfStopped);
        }

        const auto newCandidates = (candidates - m_resources);
        m_resources = candidates;

        for (auto resource : newCandidates) {
            connect(resource, &AbstractResource::sizeChanged, this, &SystemUpgrade::startIfStopped);
            connect(resource, &AbstractResource::changelogFetched, this, &SystemUpgrade::startIfStopped);
        }
    }

    void startIfStopped()
    {
        if (!m_updateSizeTimer->isActive()) {
            m_updateSizeTimer->start();
        }
    }

    void setDistroUpgrade(const AppStream::Release &release)
    {
        m_distroUpgrade = release;
    }

    void clearDistroUpgrade()
    {
        m_distroUpgrade = std::nullopt;
        Q_EMIT m_backend->inlineMessageChanged({});
    }

    bool isDistroUpgrade() const
    {
        return m_distroUpgrade.has_value();
    }

    const AppStream::Release &getDistroUpgrade()
    {
        return m_distroUpgrade.value();
    }

Q_SIGNALS:
    void updateSizeChanged();

private:
    QSet<AbstractResource *> m_resources;
    PackageKitBackend *const m_backend;
    QTimer *m_updateSizeTimer;
    std::optional<AppStream::Release> m_distroUpgrade;
};

PackageKitUpdater::PackageKitUpdater(PackageKitBackend *parent)
    : AbstractBackendUpdater(parent)
    , m_transaction(nullptr)
    , m_backend(parent)
    , m_isCancelable(false)
    , m_isProgressing(false)
    , m_percentage(0)
    , m_lastUpdate()
    , m_upgrade(new SystemUpgrade(m_backend))
{
    fetchLastUpdateTime();
}

PackageKitUpdater::~PackageKitUpdater()
{
}

void PackageKitUpdater::prepare()
{
    auto offline = PackageKit::Daemon::global()->offline();
    if (offline->updateTriggered() || offline->upgradeTriggered()) {
        m_toUpgrade.clear();
        m_allUpgradeable.clear();
        setNeedsReboot(true);
        return;
    }

#if QPK_CHECK_VERSION(1, 1, 4)
    auto results = offline->getResults();
    results.waitForFinished();
    if (results.isError() || !results.success()) {
#else
    if (QFile::exists(QStringLiteral(PK_OFFLINE_RESULTS_FILENAME))) {
#endif
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Removed offline results file";
        offline->clearResults();
    }

    Q_ASSERT(!m_transaction);
    const auto candidates = m_backend->upgradeablePackages();
    if (useOfflineUpdates() && !candidates.isEmpty()) {
        // Separate packages with theme icons from system packages
        QSet<AbstractResource *> appsWithIcons;
        QSet<AbstractResource *> systemPackages;

        for (auto resource : candidates) {
            const QString pkgName = resource->packageName();
            // Check if package has a theme icon
            bool hasIcon = QIcon::hasThemeIcon(pkgName);
            if (!hasIcon) {
                // Try org.package.name format
                QString orgName = QStringLiteral("org.") + QString(pkgName).replace(QLatin1Char('-'), QLatin1Char('.'));
                hasIcon = QIcon::hasThemeIcon(orgName);
            }

            if (hasIcon) {
                appsWithIcons.insert(resource);
            } else {
                systemPackages.insert(resource);
            }
        }

        // Group only packages without icons
        if (!systemPackages.isEmpty()) {
            m_upgrade->setCandidates(systemPackages);
            m_toUpgrade = appsWithIcons;
            m_toUpgrade.insert(m_upgrade);
        } else {
            m_toUpgrade = candidates;
        }
        connect(m_upgrade, &SystemUpgrade::updateSizeChanged, this, &PackageKitUpdater::checkFreeSpace);
    } else {
        m_toUpgrade = candidates;
    }

    checkFreeSpace();
    m_allUpgradeable = m_toUpgrade;
}

void PackageKitUpdater::checkFreeSpace()
{
    auto job = KIO::fileSystemFreeSpace(QUrl::fromLocalFile(QStringLiteral("/usr")));
    connect(job, &KJob::result, this, [this, job]() {
        if (job->availableSize() < updateSize()) {
            setErrorMessage(i18nc("@info:status %1 is a formatted disk space string e.g. '240 MiB'",
                                  "Not enough space to perform the update; only %1 of space are available.",
                                  KFormat().formatByteSize(job->availableSize())));
        }
    });
}

void PackageKitUpdater::setupTransaction(PackageKit::Transaction::TransactionFlags flags)
{
    m_packagesModified.clear();
    if (m_toUpgrade.contains(m_upgrade) && m_upgrade->isDistroUpgrade()) {
        const QString &upgradeVersion = m_upgrade->getDistroUpgrade().version();
        m_transaction = PackageKit::Daemon::upgradeSystem(upgradeVersion, PackageKit::Transaction::UpgradeKind::UpgradeKindComplete, flags);
        m_transaction->setHints(m_backend->globalHints() << QStringLiteral("cache-age=86400" /* 24*60*60 */));
    } else {
        auto pkgs = involvedPackages(m_toUpgrade).values();
        pkgs.sort();
        m_transaction = PackageKit::Daemon::updatePackages(pkgs, flags);
    }
    m_isCancelable = m_transaction->allowCancel();
    cancellableChanged();

    connect(m_transaction.data(), &PackageKit::Transaction::finished, this, &PackageKitUpdater::finished);
    connect(m_transaction.data(), &PackageKit::Transaction::package, this, &PackageKitUpdater::packageResolved);
    connect(m_transaction.data(), &PackageKit::Transaction::errorCode, this, &PackageKitUpdater::errorFound);
    connect(m_transaction.data(), &PackageKit::Transaction::mediaChangeRequired, this, &PackageKitUpdater::mediaChange);
    connect(m_transaction.data(), &PackageKit::Transaction::eulaRequired, this, &PackageKitUpdater::eulaRequired);
    connect(m_transaction.data(), &PackageKit::Transaction::repoSignatureRequired, this, &PackageKitUpdater::repoSignatureRequired);
    connect(m_transaction.data(), &PackageKit::Transaction::allowCancelChanged, this, &PackageKitUpdater::cancellableChanged);
    connect(m_transaction.data(), &PackageKit::Transaction::itemProgress, this, &PackageKitUpdater::itemProgress);
    connect(m_transaction.data(), &PackageKit::Transaction::speedChanged, this, [this] {
        Q_EMIT downloadSpeedChanged(downloadSpeed());
    });
    if (!(flags & PackageKit::Transaction::TransactionFlagSimulate)) {
        connect(m_transaction.data(), &PackageKit::Transaction::percentageChanged, this, &PackageKitUpdater::percentageChanged);
        if (m_toUpgrade.contains(m_upgrade)) {
            connect(m_transaction, &PackageKit::Transaction::percentageChanged, this, [this] {
                if (m_transaction->status() == PackageKit::Transaction::StatusDownload) {
                    Q_EMIT resourceProgressed(m_upgrade, m_transaction->percentage(), Downloading);
                }
            });
        }
    }
}

QSet<AbstractResource *> PackageKitUpdater::packagesForPackageId(const QSet<QString> &pkgids) const
{
    const auto packages = kTransform<QSet<QString>>(pkgids, [](const QString &pkgid) {
        return PackageKit::Daemon::packageName(pkgid);
    });

    QSet<AbstractResource *> ret;
    for (auto resource : std::as_const(m_allUpgradeable)) {
        QSet<QString> allPackageNames;

        if (auto upgrade = qobject_cast<SystemUpgrade *>(resource)) {
            allPackageNames = upgrade->allPackageNames();
        } else if (auto pkResource = qobject_cast<PackageKitResource *>(resource)) {
            allPackageNames = kToSet(pkResource->allPackageNames());
        }

        if (!allPackageNames.isEmpty() && packages.contains(allPackageNames)) {
            ret.insert(resource);
        }
    }

    return ret;
}

QSet<QString> PackageKitUpdater::involvedPackages(const QSet<AbstractResource *> &packages) const
{
    QSet<QString> packageIds;
    packageIds.reserve(packages.size());
    for (auto resource : packages) {
        if (auto upgrade = qobject_cast<SystemUpgrade *>(resource)) {
            packageIds = involvedPackages(upgrade->resources());
            continue;
        }

        auto pkResource = qobject_cast<PackageKitResource *>(resource);
        const QSet<QString> ids = m_backend->upgradeablePackageId(pkResource);
        if (ids.isEmpty()) {
            qWarning() << "no upgradeablePackageId for" << pkResource;
            continue;
        }

        packageIds.unite(ids);
    }
    return packageIds;
}

void PackageKitUpdater::processProceedFunction()
{
    auto t = m_proceedFunctions.takeFirst()();
    connect(t, &PackageKit::Transaction::finished, this, [this](PackageKit::Transaction::Exit status) {
        if (status != PackageKit::Transaction::Exit::ExitSuccess) {
            qWarning() << "transaction failed" << sender() << status;
            cancel();
            return;
        }

        if (!m_proceedFunctions.isEmpty()) {
            processProceedFunction();
        } else {
            start();
        }
    });
}

void PackageKitUpdater::proceed()
{
    if (!m_proceedFunctions.isEmpty()) {
        processProceedFunction();
    } else if (useOfflineUpdates()) {
        setupTransaction(PackageKit::Transaction::TransactionFlagOnlyTrusted | PackageKit::Transaction::TransactionFlagOnlyDownload);
    } else {
        setupTransaction(PackageKit::Transaction::TransactionFlagOnlyTrusted);
    }
}

bool PackageKitUpdater::useOfflineUpdates() const
{
    return m_useOfflineUpdates || m_upgrade->isDistroUpgrade() || qEnvironmentVariableIntValue("PK_OFFLINE_UPDATE");
}

void PackageKitUpdater::setOfflineUpdates(bool use)
{
    m_useOfflineUpdates = use;
}

void PackageKitUpdater::setDistroUpgrade(const AppStream::Release &release)
{
    m_upgrade->setDistroUpgrade(release);
}

void PackageKitUpdater::clearDistroUpgrade()
{
    m_upgrade->clearDistroUpgrade();
}

bool PackageKitUpdater::isDistroUpgrade() const
{
    return m_upgrade->isDistroUpgrade();
}

void PackageKitUpdater::start()
{
    Q_ASSERT(!isProgressing());

    setupTransaction(PackageKit::Transaction::TransactionFlagSimulate);
    setProgressing(true);

    if (useOfflineUpdates()) {
        setNeedsReboot(true);
    }
}

void PackageKitUpdater::finished(PackageKit::Transaction::Exit exit, uint /*time*/)
{
    // qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "update finished!" << exit << time;
    if (!m_proceedFunctions.isEmpty()) {
        return;
    }
    const bool cancel = exit == PackageKit::Transaction::ExitCancelled;
    const bool simulate = m_transaction->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;

    disconnect(m_transaction, nullptr, this, nullptr);
    m_transaction = nullptr;

    if (!cancel && simulate) {
        auto toremoveOrig = m_packagesModified.value(PackageKit::Transaction::InfoRemoving);
#ifdef QPK_CHECK_VERSION
#if QPK_CHECK_VERSION(1, 1, 2)
        toremoveOrig += m_packagesModified.value(PackageKit::Transaction::InfoRemove);
#endif
#endif
        auto toremove = toremoveOrig;
        auto toinstall = QStringList() << m_packagesModified.value(PackageKit::Transaction::InfoInstalling)
                                       << m_packagesModified.value(PackageKit::Transaction::InfoUpdating);
#ifdef QPK_CHECK_VERSION
#if QPK_CHECK_VERSION(1, 1, 2)
        toinstall << m_packagesModified.value(PackageKit::Transaction::InfoInstall);
#endif
#endif

        // some backends will treat upgrades as removal + install, which makes for terrible error messages.
        for (auto it = toremove.begin(), itEnd = toremove.end(); it != itEnd;) {
            const QString name = PackageKit::Transaction::packageName(*it);
            auto itInstall = std::find_if(toinstall.begin(), toinstall.end(), [&](const QString &pkgid) {
                return name == PackageKit::Transaction::packageName(pkgid);
            });
            if (itInstall != toinstall.end()) {
                toinstall.erase(itInstall);
                it = toremove.erase(it);
            } else {
                ++it;
            }
        };

        if (PackageKit::Daemon::backendName() == QLatin1String("dnf")) {
            // Fedora has some packages that it uninstalls then eventually creates on its own. No need to
            // notify about these.
            toremove = kFilter<QStringList>(toremove, [](const QString &pkgid) {
                return !PackageKit::Transaction::packageName(pkgid).startsWith(QLatin1String("kmod"));
            });
        }

        if (!toremove.isEmpty()) {
            QStringList criticals;
            for (const auto &pkgid : std::as_const(toremove)) {
                auto resources = kFilter<QVector<AbstractResource *>>(m_backend->resourcesByPackageName(pkgid), [](AbstractResource *resource) {
                    return static_cast<PackageKitResource *>(resource)->isCritical();
                });
                criticals << kTransform<QStringList>(resources, [](AbstractResource *resource) {
                    return resource->name();
                });
                if (!criticals.isEmpty()) {
                    break;
                }
            }

            if (!criticals.isEmpty()) {
                const QString msg = i18n(
                    "This update cannot be completed as it would remove the following software which is critical to the system's operation:<nl/>"
                    "<ul><li>%1</li></ul><nl/>"
                    "If you believe this is an error, please report it as a bug to the packagers of your distribution.",
                    criticals.constFirst());
                Q_EMIT distroErrorMessage(msg);
            } else {
                Q_EMIT proceedRequest(
                    i18n("Confirm Changes"),
                    i18n("The following packages will be removed by the update:<ul><li>%1</li></ul><br/>in order to install:<ul><li>%2</li></ul>",
                         PackageKitResource::joinPackages(toremove, QStringLiteral("</li><li>"), {}),
                         PackageKitResource::joinPackages(toinstall, QStringLiteral("</li><li>"), {})));
            }
        } else {
            proceed();
        }
        return;
    }

    if (!m_toUpgrade.contains(m_upgrade) || !m_upgrade->isDistroUpgrade()) {
        // If the distro-upgrade was prepared successfully, fetching updates now would invalidate it.
        // If the distro-upgrade failed, fetching updates now would find nothing and we would propose to reboot.
        setProgressing(false);
        m_backend->fetchUpdates();
        fetchLastUpdateTime();
    } else if (exit != PackageKit::Transaction::ExitSuccess) {
        Q_EMIT resourceProgressed(m_upgrade, 0, None);
        setNeedsReboot(false);
        setProgressing(false);
    }

    if (useOfflineUpdates() && exit == PackageKit::Transaction::ExitSuccess) {
        if (m_upgrade->isDistroUpgrade()) {
            QDBusPendingReply<void> reply = PackageKit::Daemon::global()->offline()->triggerUpgrade(PackageKit::Offline::ActionReboot);
            // Call may fail because of authorization
            reply.waitForFinished();
            if (reply.isError()) {
                Q_EMIT resourceProgressed(m_upgrade, 0, None);
                setNeedsReboot(false);
                setProgressing(false);
                Q_EMIT passiveMessage(reply.error().message());
                return;
            }
            m_backend->clear();
            setProgressing(false);
        } else {
            PackageKit::Daemon::global()->offline()->trigger(m_offlineUpdateAction);
        }
        enableReadyToReboot();
    }
}

void PackageKitUpdater::cancellableChanged()
{
    if (m_isCancelable != m_transaction->allowCancel()) {
        m_isCancelable = m_transaction->allowCancel();
        Q_EMIT cancelableChanged(m_isCancelable);
    }
}

void PackageKitUpdater::percentageChanged()
{
    const int percentage = m_transaction->percentage();
    if (percentage > 100) {
        return;
    }
    const auto actualPercentage = useOfflineUpdates() ? percentage : percentageWithStatus(m_transaction->status(), percentage);
    if (actualPercentage >= 0 && m_percentage != actualPercentage) {
        m_percentage = actualPercentage;
        Q_EMIT progressChanged(m_percentage);
    }
}

bool PackageKitUpdater::hasUpdates() const
{
    return m_backend->updatesCount() > 0;
}

qreal PackageKitUpdater::progress() const
{
    return m_percentage;
}

void PackageKitUpdater::removeResources(const QList<AbstractResource *> &resources)
{
    const QSet<QString> pkgs = involvedPackages(kToSet(resources));
    m_toUpgrade.subtract(packagesForPackageId(pkgs));
}

void PackageKitUpdater::addResources(const QList<AbstractResource *> &resources)
{
    const QSet<QString> pkgs = involvedPackages(kToSet(resources));
    m_toUpgrade.unite(packagesForPackageId(pkgs));
}

QList<AbstractResource *> PackageKitUpdater::toUpdate() const
{
    return m_toUpgrade.values();
}

bool PackageKitUpdater::isMarked(AbstractResource *resource) const
{
    return m_toUpgrade.contains(resource);
}

QDateTime PackageKitUpdater::lastUpdate() const
{
    return m_lastUpdate;
}

bool PackageKitUpdater::isCancelable() const
{
    return m_isCancelable;
}

bool PackageKitUpdater::isProgressing() const
{
    return m_isProgressing;
}

void PackageKitUpdater::cancel()
{
    if (m_transaction) {
        m_transaction->cancel();
    } else {
        setProgressing(false);
    }
}

void PackageKitUpdater::errorFound(PackageKit::Transaction::Error err, const QString &error)
{
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement || err == PackageKit::Transaction::ErrorTransactionCancelled
        || err == PackageKit::Transaction::ErrorNotAuthorized) {
        return;
    }
    QString finalMessage = xi18nc("@info", "%1:<nl/><nl/>%2", PackageKitMessages::errorMessage(err, QString()), error);
    Q_EMIT passiveMessage(finalMessage);
    qWarning() << "Error happened" << err << error;
}

void PackageKitUpdater::mediaChange(PackageKit::Transaction::MediaType media, const QString &type, const QString &text)
{
    Q_UNUSED(media)
    Q_EMIT passiveMessage(i18n("Media Change of type '%1' is requested.\n%2", type, text));
}

EulaHandling handleEula(const QString &eulaID, const QString &licenseAgreement)
{
    KConfigGroup group(KSharedConfig::openConfig(), u"EULA"_s);
    auto licenseGroup = group.group(eulaID);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(licenseAgreement.toUtf8());
    QByteArray hashHex = hash.result().toHex();

    EulaHandling ret;
    ret.request = licenseGroup.readEntry("Hash", QByteArray()) != hashHex;
    if (!ret.request) {
        ret.proceedFunction = [eulaID] {
            return PackageKit::Daemon::acceptEula(eulaID);
        };
    } else {
        ret.proceedFunction = [eulaID, hashHex] {
            KConfigGroup group(KSharedConfig::openConfig(), u"EULA"_s);
            KConfigGroup licenseGroup = group.group(eulaID);
            licenseGroup.writeEntry<QByteArray>("Hash", hashHex);
            return PackageKit::Daemon::acceptEula(eulaID);
        };
    }
    return ret;
}

void PackageKitUpdater::eulaRequired(const QString &eulaID, const QString &packageID, const QString &vendor, const QString &licenseAgreement)
{
    const auto handle = handleEula(eulaID, licenseAgreement);
    m_proceedFunctions << handle.proceedFunction;
    if (handle.request) {
        Q_EMIT proceedRequest(i18n("Accept EULA"),
                              i18n("The package %1 and its vendor %2 require that you accept their license:\n %3",
                                   PackageKit::Daemon::packageName(packageID),
                                   vendor,
                                   licenseAgreement));
    } else {
        proceed();
    }
}

void PackageKitUpdater::setProgressing(bool progressing)
{
    if (m_isProgressing != progressing) {
        m_isProgressing = progressing;
        Q_EMIT progressingChanged(m_isProgressing);
    }
}

void PackageKitUpdater::fetchLastUpdateTime()
{
    QDBusPendingReply<uint> transaction = PackageKit::Daemon::global()->getTimeSinceAction(PackageKit::Transaction::RoleGetUpdates);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(transaction, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PackageKitUpdater::lastUpdateTimeReceived);
}

void PackageKitUpdater::lastUpdateTimeReceived(QDBusPendingCallWatcher *w)
{
    QDBusPendingReply<uint> reply = w->reply();
    if (reply.isError()) {
        qWarning() << "Error when fetching the last update time" << reply.error();
    } else {
        m_lastUpdate = QDateTime::currentDateTime().addSecs(-int(reply.value()));
    }
    w->deleteLater();
}

AbstractBackendUpdater::State toUpdateState(PackageKit::Transaction::Status t)
{
    switch (t) {
    case PackageKit::Transaction::StatusUnknown:
    case PackageKit::Transaction::StatusDownload:
        return AbstractBackendUpdater::Downloading;
    case PackageKit::Transaction::StatusDepResolve:
    case PackageKit::Transaction::StatusSigCheck:
    case PackageKit::Transaction::StatusTestCommit:
    case PackageKit::Transaction::StatusInstall:
    case PackageKit::Transaction::StatusCommit:
        return AbstractBackendUpdater::Installing;
    case PackageKit::Transaction::StatusFinished:
    case PackageKit::Transaction::StatusCancel:
        return AbstractBackendUpdater::Done;
    default:
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "unknown packagekit status" << t;
        return AbstractBackendUpdater::None;
    }
    Q_UNREACHABLE();
}

void PackageKitUpdater::itemProgress(const QString &itemID, PackageKit::Transaction::Status status, uint percentage)
{
    const auto resources = packagesForPackageId({itemID});

    for (auto resource : resources) {
        Q_EMIT resourceProgressed(resource, percentage, toUpdateState(status));
    }
}

void PackageKitUpdater::fetchChangelog() const
{
    QStringList pkgids;
    for (auto resource : std::as_const(m_allUpgradeable)) {
        if (auto upgrade = qobject_cast<SystemUpgrade *>(resource)) {
            upgrade->fetchChangelog();
        } else {
            pkgids += static_cast<PackageKitResource *>(resource)->availablePackageId();
        }
    }
    Q_ASSERT(!pkgids.isEmpty());

    PackageKit::Transaction *transaction = PackageKit::Daemon::getUpdatesDetails(pkgids);
    connect(transaction, &PackageKit::Transaction::updateDetail, this, &PackageKitUpdater::updateDetail);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitUpdater::errorFound);
}

void PackageKitUpdater::updateDetail(const QString &packageID,
                                     const QStringList &updates,
                                     const QStringList &obsoletes,
                                     const QStringList &vendorUrls,
                                     const QStringList &bugzillaUrls,
                                     const QStringList &cveUrls,
                                     PackageKit::Transaction::Restart restart,
                                     const QString &updateText,
                                     const QString &changelog,
                                     PackageKit::Transaction::UpdateState state,
                                     const QDateTime &issued,
                                     const QDateTime &updated)
{
    const auto resources = packagesForPackageId({packageID});
    for (auto resource : resources) {
        auto pkResource = static_cast<PackageKitResource *>(resource);
        pkResource->updateDetail(packageID, updates, obsoletes, vendorUrls, bugzillaUrls, cveUrls, restart, updateText, changelog, state, issued, updated);
    }
}

void PackageKitUpdater::packageResolved(PackageKit::Transaction::Info info, const QString &packageId)
{
    m_packagesModified[info] << packageId;
}

void PackageKitUpdater::repoSignatureRequired(const QString &packageID,
                                              const QString &repoName,
                                              const QString &keyUrl,
                                              const QString &keyUserid,
                                              const QString &keyId,
                                              const QString &keyFingerprint,
                                              const QString &keyTimestamp,
                                              PackageKit::Transaction::SigType type)
{
    Q_EMIT proceedRequest(i18n("Missing signature for %1 in %2", packageID, repoName),
                          i18n("Do you trust the following key?\n\nUrl: %1\nUser: %2\nKey: %3\nFingerprint: %4\nTimestamp: %4\n",
                               keyUrl,
                               keyUserid,
                               keyFingerprint,
                               keyTimestamp));

    m_proceedFunctions << [type, keyId, packageID]() {
        return PackageKit::Daemon::installSignature(type, keyId, packageID);
    };
}

double PackageKitUpdater::updateSize() const
{
    double ret = 0.;
    QSet<QString> donePkgs;
    for (auto resource : std::as_const(m_toUpgrade)) {
        if (auto upgrade = qobject_cast<SystemUpgrade *>(resource)) {
            ret += upgrade->size();
        } else if (auto pkResource = qobject_cast<PackageKitResource *>(resource)) {
            QString pkgname = pkResource->packageName();
            if (!donePkgs.contains(pkgname)) {
                donePkgs.insert(pkgname);
                ret += pkResource->size();
            }
        }
    }
    return ret;
}

quint64 PackageKitUpdater::downloadSpeed() const
{
    return m_transaction ? m_transaction->speed() : 0;
}

void PackageKitUpdater::setOfflineUpdateAction(PackageKit::Offline::Action action)
{
    m_offlineUpdateAction = action;
    if (PackageKit::Daemon::global()->offline()->updateTriggered()) {
        PackageKit::Daemon::global()->offline()->trigger(action);
    }
}

bool PackageKitUpdater::isFetchingUpdates() const
{
    return m_backend->refresher() != nullptr;
}

#include "PackageKitUpdater.moc"
#include "moc_PackageKitUpdater.cpp"
