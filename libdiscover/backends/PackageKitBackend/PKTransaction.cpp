/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PKTransaction.h"
#include "LocalFilePKResource.h"
#include "PackageKitBackend.h"
#include "PackageKitMessages.h"
#include "PackageKitResource.h"
#include "PackageKitUpdater.h"
#include "libdiscover_backend_packagekit_debug.h"
#include "utils.h"
#include <KLocalizedString>
#include <PackageKit/Daemon>
#include <QDebug>
#include <QTimer>
#include <functional>
#include <resources/AbstractResource.h>

PKTransaction::PKTransaction(const QVector<AbstractResource *> &resources, Transaction::Role role)
    : Transaction(resources.first()->backend(), resources.first(), role)
    , m_apps(resources)
{
    Q_ASSERT(!resources.contains(nullptr));
    for (auto resource : resources) {
        auto pkResource = qobject_cast<PackageKitResource *>(resource);
        m_pkgnames.unite(kToSet(pkResource->allPackageNames()));
    }

    QTimer::singleShot(0, this, &PKTransaction::start);
}

static QStringList packageIds(const QVector<AbstractResource *> &resources, std::function<QString(PackageKitResource *)> func)
{
    QStringList ret;
    for (auto resource : resources) {
        ret += func(qobject_cast<PackageKitResource *>(resource));
    }
    ret.removeDuplicates();
    return ret;
}

bool PKTransaction::isLocal() const
{
    return m_apps.size() == 1 && qobject_cast<LocalFilePKResource *>(m_apps.at(0));
}

void PKTransaction::start()
{
    trigger(PackageKit::Transaction::TransactionFlagSimulate);
}

void PKTransaction::trigger(PackageKit::Transaction::TransactionFlags flags)
{
    if (m_trans) {
        m_trans->deleteLater();
    }
    m_newPackageStates.clear();

    if (isLocal() && role() == Transaction::InstallRole) {
        auto resource = qobject_cast<LocalFilePKResource *>(m_apps.at(0));
        m_trans = PackageKit::Daemon::installFile(QUrl(resource->packageName()).toLocalFile(), flags);
    } else
        switch (role()) {
        case Transaction::ChangeAddonsRole:
        case Transaction::InstallRole: {
            const auto ids = packageIds(m_apps, [](PackageKitResource *resource) {
                return resource->availablePackageId();
            });
            if (ids.isEmpty()) {
                // FIXME this state shouldn't exist
                qWarning() << "Installing no packages found!";
                for (auto resource : std::as_const(m_apps)) {
                    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "app" << resource << resource->state();
                }

                setStatus(Transaction::DoneWithErrorStatus);
                return;
            }
            m_trans = PackageKit::Daemon::installPackages(ids, flags);
            break;
        }
        case Transaction::RemoveRole:
            // see bug #315063
#ifdef PACKAGEKIT_AUTOREMOVE
            constexpr bool autoRemove = true;
#else
            constexpr bool autoRemove = false;
#endif
            m_trans = PackageKit::Daemon::removePackages(packageIds(m_apps,
                                                                    [](PackageKitResource *resource) {
                                                                        return resource->installedPackageId();
                                                                    }),
                                                         true /*allowDeps*/,
                                                         autoRemove,
                                                         flags);
            break;
        };
    Q_ASSERT(m_trans);

    if (false) {
        connect(m_trans.data(), &PackageKit::Transaction::statusChanged, this, [this]() {
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "state..." << m_trans->status();
        });
    }

    connect(m_trans.data(), &PackageKit::Transaction::package, this, &PKTransaction::packageResolved);
    connect(m_trans.data(), &PackageKit::Transaction::finished, this, &PKTransaction::cleanup);
    connect(m_trans.data(), &PackageKit::Transaction::errorCode, this, &PKTransaction::errorFound);
    connect(m_trans.data(), &PackageKit::Transaction::mediaChangeRequired, this, &PKTransaction::mediaChange);
    connect(m_trans.data(), &PackageKit::Transaction::requireRestart, this, &PKTransaction::requireRestart);
    connect(m_trans.data(), &PackageKit::Transaction::repoSignatureRequired, this, &PKTransaction::repoSignatureRequired);
    connect(m_trans.data(), &PackageKit::Transaction::percentageChanged, this, &PKTransaction::progressChanged);
    connect(m_trans.data(), &PackageKit::Transaction::statusChanged, this, &PKTransaction::statusChanged);
    connect(m_trans.data(), &PackageKit::Transaction::eulaRequired, this, &PKTransaction::eulaRequired);
    connect(m_trans.data(), &PackageKit::Transaction::allowCancelChanged, this, &PKTransaction::cancellableChanged);
    connect(m_trans.data(), &PackageKit::Transaction::remainingTimeChanged, this, [this]() {
        setRemainingTime(m_trans->remainingTime());
    });
    connect(m_trans.data(), &PackageKit::Transaction::speedChanged, this, [this]() {
        setDownloadSpeed(m_trans->speed());
    });

    setCancellable(m_trans->allowCancel());
}

void PKTransaction::statusChanged()
{
    setStatus(m_trans->status() == PackageKit::Transaction::StatusDownload ? Transaction::DownloadingStatus : Transaction::CommittingStatus);
    progressChanged();
}

void PKTransaction::progressChanged()
{
    auto percent = m_trans->percentage();
    if (percent == 101) {
        qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "percentage cannot be calculated";
        percent = 50;
    }

    const auto processedPercentage = percentageWithStatus(m_trans->status(), qBound<int>(0, percent, 100));
    if (processedPercentage >= 0) {
        setProgress(processedPercentage);
    }
}

void PKTransaction::cancellableChanged()
{
    setCancellable(m_trans->allowCancel());
}

void PKTransaction::cancel()
{
    if (!m_trans) {
        setStatus(CancelledStatus);
    } else if (m_trans->allowCancel()) {
        m_trans->cancel();
    } else {
        qWarning() << "trying to cancel a non-cancellable transaction: " << resource()->name();
    }
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    Q_UNUSED(runtime)
    const bool cancel = !m_proceedFunctions.isEmpty() || exit == PackageKit::Transaction::ExitCancelled;
    const bool failed = exit == PackageKit::Transaction::ExitFailed || exit == PackageKit::Transaction::ExitUnknown;
    const bool simulate = m_trans->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;

    disconnect(m_trans, nullptr, this, nullptr);
    m_trans = nullptr;

    const auto backend = qobject_cast<PackageKitBackend *>(resource()->backend());

    if (!cancel && !failed && simulate) {
        auto packagesToRemove = m_newPackageStates.value(PackageKit::Transaction::InfoRemoving);
        QMutableListIterator<QString> i(packagesToRemove);
        QSet<AbstractResource *> removedResources;
        while (i.hasNext()) {
            const auto pkgname = PackageKit::Daemon::packageName(i.next());
            removedResources.unite(backend->resourcesByPackageName(pkgname));

            if (m_pkgnames.contains(pkgname)) {
                i.remove();
            }
        }
        removedResources.subtract(kToSet(m_apps));

        auto isCritical = [](AbstractResource *resource) {
            return static_cast<PackageKitResource *>(resource)->isCritical();
        };
        auto criticals = kFilter<QSet<AbstractResource *>>(removedResources, isCritical);
        criticals.unite(kFilter<QSet<AbstractResource *>>(m_apps, isCritical));
        auto resourceName = [](AbstractResource *a) {
            return a->name();
        };
        if (!criticals.isEmpty()) {
            const QString msg = i18n(
                "This action cannot be completed as it would remove the following software which is critical to the system's operation:<nl/>"
                "<ul><li>%1</li></ul><nl/>"
                "If you believe this is an error, please report it as a bug to the packagers of your distribution.",
                resourceName(*criticals.begin()));
            Q_EMIT distroErrorMessage(msg);
            setStatus(Transaction::DoneWithErrorStatus);
        } else if (!packagesToRemove.isEmpty() || !removedResources.isEmpty()) {
            QString msg;
            const QStringList removedResourcesStr = kTransform<QStringList>(removedResources, resourceName);
            msg += QLatin1String("<ul><li>") + PackageKitResource::joinPackages(packagesToRemove, QLatin1String("</li><li>"), {}) + QLatin1Char('\n');
            msg += removedResourcesStr.join(QLatin1String("</li><li>"));
            msg += QStringLiteral("</li></ul>");

            Q_EMIT proceedRequest(i18n("Confirm package removal"),
                                  i18np("This action will also remove the following package:\n%2",
                                        "This action will also remove the following packages:\n%2",
                                        packagesToRemove.count(),
                                        msg));
        } else {
            proceed();
        }
        return;
    }

    this->submitResolve();
    if (isLocal()) {
        qobject_cast<LocalFilePKResource *>(m_apps.at(0))->resolve({});
    }
    if (failed) {
        setStatus(Transaction::DoneWithErrorStatus);
    } else if (cancel) {
        setStatus(Transaction::CancelledStatus);
    } else {
        setStatus(Transaction::DoneStatus);
    }
}

void PKTransaction::processProceedFunction()
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

void PKTransaction::proceed()
{
    if (!m_proceedFunctions.isEmpty()) {
        processProceedFunction();
    } else {
        if (isLocal()) {
            trigger(PackageKit::Transaction::TransactionFlagNone);
        } else {
            trigger(PackageKit::Transaction::TransactionFlagOnlyTrusted);
        }
    }
}

void PKTransaction::packageResolved(PackageKit::Transaction::Info info, const QString &packageId)
{
    m_newPackageStates[info].append(packageId);
}

void PKTransaction::submitResolve()
{
    const auto backend = qobject_cast<PackageKitBackend *>(resource()->backend());
    QStringList needResolving;
    for (auto it = m_newPackageStates.constBegin(), itEnd = m_newPackageStates.constEnd(); it != itEnd; ++it) {
        const auto &itValue = it.value();
        for (const auto &pkgid : itValue) {
            const auto resources = backend->resourcesByPackageName(PackageKit::Daemon::packageName(pkgid));
            for (auto resource : resources) {
                auto pkResource = qobject_cast<PackageKitResource *>(resource);
                pkResource->clearPackageIds();
                Q_EMIT pkResource->stateChanged();
                needResolving << pkResource->allPackageNames();
            }
        }
    }
    needResolving.removeDuplicates();
    backend->resolvePackages(needResolving);
}

PackageKit::Transaction *PKTransaction::transaction()
{
    return m_trans;
}

void PKTransaction::eulaRequired(const QString &eulaID, const QString &packageID, const QString &vendor, const QString &licenseAgreement)
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

void PKTransaction::errorFound(PackageKit::Transaction::Error err, const QString &error)
{
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement || err == PackageKit::Transaction::ErrorTransactionCancelled
        || err == PackageKit::Transaction::ErrorNotAuthorized) {
        return;
    }
    qWarning() << "PackageKit error:" << err << PackageKitMessages::errorMessage(err, error) << error;
    Q_EMIT passiveMessage(PackageKitMessages::errorMessage(err, error));
}

void PKTransaction::mediaChange(PackageKit::Transaction::MediaType media, const QString &type, const QString &text)
{
    Q_UNUSED(media)
    Q_EMIT passiveMessage(i18n("Media Change of type '%1' is requested.\n%2", type, text));
}

void PKTransaction::requireRestart(PackageKit::Transaction::Restart restart, const QString &pkgid)
{
    Q_EMIT passiveMessage(PackageKitMessages::restartMessage(restart, pkgid));
}

void PKTransaction::repoSignatureRequired(const QString &packageID,
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

#include "moc_PKTransaction.cpp"
