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

static bool isEmptyPackageKitTransactionFailure(const QString &error)
{
    const QString trimmed = error.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }

    if (trimmed == QStringLiteral("Transaction failed")) {
        return true;
    }

    if (trimmed.startsWith(QStringLiteral("Transaction failed:"))) {
        return trimmed.mid(QStringLiteral("Transaction failed:").size()).trimmed().isEmpty();
    }

    return false;
}

static bool isValidDnfPackageName(const QString &packageName)
{
    if (packageName.isEmpty() || packageName.startsWith(QLatin1Char('-'))) {
        return false;
    }

    for (const QChar &ch : packageName) {
        if (!ch.isLetterOrNumber() && ch != QLatin1Char('_') && ch != QLatin1Char('+') && ch != QLatin1Char('.') && ch != QLatin1Char('-')) {
            return false;
        }
    }

    return true;
}

static QStringList dnfFallbackPackagesForPackageIds(const QStringList &packageIds)
{
    QStringList packages;
    for (const QString &packageId : packageIds) {
        const QString packageName = PackageKit::Daemon::packageName(packageId);
        if (!isValidDnfPackageName(packageName)) {
            qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Refusing DNF fallback package name from PackageKit package id" << packageId << packageName;
            continue;
        }
        packages << packageName;
    }
    packages.removeDuplicates();
    return packages;
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
    m_packageKitRemoveFailedWithEmptyDetail = false;

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
            {
                auto ids = packageIds(m_apps, [](PackageKitResource *resource) {
                    return resource->installedPackageId();
                });
                ids.removeAll(QString());
                m_dnfFallbackPackages = dnfFallbackPackagesForPackageIds(ids);

                if (ids.isEmpty()) {
                    for (auto resource : std::as_const(m_apps)) {
                        auto pkResource = qobject_cast<PackageKitResource *>(resource);
                        qWarning() << "Cannot remove PackageKit resource without installed package id"
                                   << "resourceName=" << resource->name() << "packageName=" << (pkResource ? pkResource->packageName() : QString())
                                   << "allPackageNames=" << (pkResource ? pkResource->allPackageNames() : QStringList()) << "state=" << resource->state();
                    }
                    Q_EMIT passiveMessage(
                        i18n("Cannot determine the installed package for '%1'. Refresh the application list and try again.", resource()->name()));
                    setStatus(Transaction::DoneWithErrorStatus);
                    return;
                }

                qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Removing PackageKit packages" << ids;
                m_trans = PackageKit::Daemon::removePackages(ids, true /*allowDeps*/, autoRemove, flags);
            }
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
    m_waitingForDnfFallbackConfirmation = false;

    if (m_dnfFallbackProcess && m_dnfFallbackProcess->state() != QProcess::NotRunning) {
        m_dnfFallbackCancelling = true;
        m_dnfFallbackProcess->terminate();
        if (!m_dnfFallbackProcess->waitForFinished(5000)) {
            m_dnfFallbackProcess->kill();
        }
        if (status() < Transaction::DoneStatus) {
            setStatus(CancelledStatus);
        }
        return;
    }

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
                "This action cannot be completed as it would remove the following software which is critical to the system’s operation:<nl/>"
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

    if (failed && !simulate && m_packageKitRemoveFailedWithEmptyDetail && !m_dnfFallbackAttempted && !m_dnfFallbackPackages.isEmpty()) {
        requestDnfRemoveFallback();
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
    if (m_waitingForDnfFallbackConfirmation) {
        startDnfRemoveFallback();
        return;
    }

    if (!m_proceedFunctions.isEmpty()) {
        processProceedFunction();
    } else {
        if (isLocal() || role() == Transaction::RemoveRole) {
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

void PKTransaction::requestDnfRemoveFallback()
{
    m_waitingForDnfFallbackConfirmation = true;
    setCancellable(true);
    setStatus(Transaction::CommittingStatus);
    setProgress(50);

    const QString packageList = QStringLiteral("<ul><li>") + m_dnfFallbackPackages.join(QLatin1String("</li><li>")) + QStringLiteral("</li></ul>");
    Q_EMIT proceedRequest(i18n("Retry removal with DNF"),
                          i18np("PackageKit could not remove this package and did not provide a detailed error. Discover can retry the removal with DNF for "
                                "the following RPM package:<nl/>%2<nl/>You will be asked for administrator privileges.",
                                "PackageKit could not remove this package and did not provide a detailed error. Discover can retry the removal with DNF for "
                                "the following RPM packages:<nl/>%2<nl/>You will be asked for administrator privileges.",
                                m_dnfFallbackPackages.count(),
                                packageList));
}

void PKTransaction::startDnfRemoveFallback()
{
    m_waitingForDnfFallbackConfirmation = false;
    m_dnfFallbackAttempted = true;
    m_dnfFallbackCancelling = false;

    if (m_dnfFallbackPackages.isEmpty()) {
        Q_EMIT passiveMessage(i18n("Cannot retry the removal with DNF because no RPM package name is known."));
        setStatus(Transaction::DoneWithErrorStatus);
        return;
    }

    QStringList arguments;
    arguments << QStringLiteral("dnf");
    arguments << QStringLiteral("remove");
    arguments << QStringLiteral("-y");
    arguments << m_dnfFallbackPackages;

    m_dnfFallbackStdoutBuffer.clear();
    m_dnfFallbackStderrBuffer.clear();

    if (m_dnfFallbackProcess) {
        m_dnfFallbackProcess->deleteLater();
    }
    m_dnfFallbackProcess = new QProcess(this);
    connect(m_dnfFallbackProcess.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PKTransaction::dnfFallbackFinished);
    connect(m_dnfFallbackProcess.data(), &QProcess::errorOccurred, this, &PKTransaction::dnfFallbackError);
    connect(m_dnfFallbackProcess.data(), &QProcess::readyReadStandardOutput, this, &PKTransaction::dnfFallbackOutput);
    connect(m_dnfFallbackProcess.data(), &QProcess::readyReadStandardError, this, &PKTransaction::dnfFallbackOutput);

    setCancellable(true);
    setStatus(Transaction::CommittingStatus);
    setProgress(50);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Starting DNF remove fallback: pkexec" << arguments;
    m_dnfFallbackProcess->start(QStringLiteral("pkexec"), arguments);
}

void PKTransaction::dnfFallbackFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    dnfFallbackOutput();

    if (m_dnfFallbackProcess) {
        m_dnfFallbackProcess->deleteLater();
        m_dnfFallbackProcess = nullptr;
    }

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "DNF remove fallback finished with exit code:" << exitCode << "status:" << exitStatus;

    if (m_dnfFallbackCancelling) {
        m_dnfFallbackCancelling = false;
        if (status() < Transaction::DoneStatus) {
            setStatus(Transaction::CancelledStatus);
        }
        return;
    }

    if (status() >= Transaction::DoneStatus) {
        return;
    }

    if (exitStatus == QProcess::CrashExit) {
        Q_EMIT passiveMessage(i18n("DNF removal crashed unexpectedly."));
        setStatus(Transaction::DoneWithErrorStatus);
        return;
    }

    if (exitCode != 0) {
        const QString output = dnfFallbackDiagnosticOutput();
        const QString errorMessage = output.isEmpty() ? i18n("No error output was returned") : output;
        qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "DNF remove fallback failed:" << errorMessage;
        Q_EMIT passiveMessage(i18n("DNF could not remove the package: %1", errorMessage));
        setStatus(Transaction::DoneWithErrorStatus);
        return;
    }

    refreshDnfFallbackPackageState();
    setProgress(100);
    setStatus(Transaction::DoneStatus);
}

void PKTransaction::dnfFallbackError(QProcess::ProcessError error)
{
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "DNF remove fallback process error:" << error;

    if (m_dnfFallbackCancelling) {
        return;
    }

    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = i18n("Failed to start DNF removal. Make sure 'pkexec' and 'dnf' are installed.");
        break;
    case QProcess::Crashed:
        errorMessage = i18n("DNF removal crashed unexpectedly.");
        break;
    case QProcess::Timedout:
        errorMessage = i18n("DNF removal timed out.");
        break;
    default:
        errorMessage = i18n("An unknown error occurred during DNF removal.");
        break;
    }

    Q_EMIT passiveMessage(errorMessage);
    setStatus(Transaction::DoneWithErrorStatus);
}

void PKTransaction::dnfFallbackOutput()
{
    if (!m_dnfFallbackProcess) {
        return;
    }

    const QString output = QString::fromUtf8(m_dnfFallbackProcess->readAllStandardOutput());
    if (!output.isEmpty()) {
        m_dnfFallbackStdoutBuffer += output;
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "DNF remove fallback output:" << output;
    }

    const QString error = QString::fromUtf8(m_dnfFallbackProcess->readAllStandardError());
    if (!error.isEmpty()) {
        m_dnfFallbackStderrBuffer += error;
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "DNF remove fallback stderr:" << error;
    }
}

QString PKTransaction::dnfFallbackDiagnosticOutput() const
{
    QString output = m_dnfFallbackStderrBuffer.trimmed();
    if (output.isEmpty()) {
        output = m_dnfFallbackStdoutBuffer.trimmed();
    }

    constexpr qsizetype maxOutputLength = 800;
    if (output.size() > maxOutputLength) {
        output = output.left(maxOutputLength) + QStringLiteral("...");
    }

    return output;
}

void PKTransaction::refreshDnfFallbackPackageState()
{
    const auto backend = qobject_cast<PackageKitBackend *>(resource()->backend());
    if (!backend) {
        return;
    }

    QStringList needResolving = m_dnfFallbackPackages;
    for (auto resource : std::as_const(m_apps)) {
        auto pkResource = qobject_cast<PackageKitResource *>(resource);
        if (!pkResource) {
            continue;
        }
        pkResource->clearPackageIds();
        Q_EMIT pkResource->stateChanged();
        needResolving << pkResource->allPackageNames();
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

    const bool simulate = m_trans && (m_trans->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate);
    if (role() == Transaction::RemoveRole && !simulate && err == PackageKit::Transaction::ErrorTransactionError && isEmptyPackageKitTransactionFailure(error)
        && !m_dnfFallbackAttempted && !m_dnfFallbackPackages.isEmpty()) {
        m_packageKitRemoveFailedWithEmptyDetail = true;
        qWarning() << "PackageKit remove failed with empty transaction detail, DNF fallback will be offered for packages:" << m_dnfFallbackPackages;
        return;
    }

    qWarning() << "PackageKit error:" << err << PackageKitMessages::errorMessage(err, error) << error;
    Q_EMIT passiveMessage(PackageKitMessages::errorMessage(err, error));
}

void PKTransaction::mediaChange(PackageKit::Transaction::MediaType media, const QString &type, const QString &text)
{
    Q_UNUSED(media)
    Q_EMIT passiveMessage(i18n("Media Change of type “%1” is requested.\n%2", type, text));
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
