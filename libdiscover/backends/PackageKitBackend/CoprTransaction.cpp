/*
 *   SPDX-FileCopyrightText: 2025-2026 DXVSI <https://github.com/DXVSI>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "CoprTransaction.h"
#include "CoprClient.h"
#include "CoprInstalledPackages.h"
#include "CoprResource.h"
#include "PackageKitBackend.h"
#include "libdiscover_backend_packagekit_debug.h"

#include <KLocalizedString>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

CoprTransaction::CoprTransaction(CoprResource *resource, Transaction::Role role, PackageKitBackend *backend)
    : Transaction(backend, resource, role, {})
    , m_resource(resource)
    , m_backend(backend)
    , m_process(new QProcess(this))
    , m_role(role)
    , m_state(EnableRepo)
{
    setCancellable(true);
    setStatus(SetupStatus);

    // The messages of pkexec and dnf are matched as text; pkexec hands LC_ALL over to what it runs
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_process->setProcessEnvironment(environment);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CoprTransaction::processFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &CoprTransaction::processError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &CoprTransaction::processOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &CoprTransaction::processOutput);

    // Start the transaction immediately
    QTimer::singleShot(0, this, &CoprTransaction::proceed);
}

CoprTransaction::~CoprTransaction()
{
}

void CoprTransaction::cancel()
{
    m_cancelled = true;
    setCancellable(false);

    // Never waited for: processFinished() ends the transaction. While pkexec still asks for
    // the password it goes away at once; dnf that already runs as root does not take a
    // signal from this user and finishes what it is doing.
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        QTimer::singleShot(5000, m_process, &QProcess::kill);
        return;
    }
    if (m_state == CheckInstalledPackages) {
        return;
    }

    setStatus(CancelledStatus);
}

void CoprTransaction::proceed()
{
    // Cancelled before it started
    if (m_cancelled) {
        return;
    }
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprTransaction::proceed() - role:" << m_role;
    setStatus(DownloadingStatus);

    if (!m_resource) {
        setStatus(DoneWithErrorStatus);
        return;
    }
    m_packageName = m_resource->packageName();
    m_owner = m_resource->coprOwner();
    m_project = m_resource->coprProject();

    if (m_role == InstallRole) {
        // Before the repository is enabled, not after
        if (m_packageName.isEmpty()) {
            Q_EMIT passiveMessage(i18n("No installable package is known for this COPR project yet"));
            setStatus(DoneWithErrorStatus);
            return;
        }
        if (!hasValidNames() || !canInstallForCurrentChroot()) {
            setStatus(DoneWithErrorStatus);
            return;
        }
        enableCoprRepo();
    } else if (m_role == RemoveRole) {
        if (m_packageName.isEmpty()) {
            Q_EMIT passiveMessage(i18n("No installed package is known for this COPR resource"));
            setStatus(DoneWithErrorStatus);
            return;
        }
        if (!hasValidNames()) {
            setStatus(DoneWithErrorStatus);
            return;
        }
        m_packageCameFromRepository = m_backend->coprInstalledPackages()->packagesFromRepository(m_owner, m_project).contains(m_packageName);
        removePackage();
    }
}

bool CoprTransaction::hasValidNames()
{
    // These names came from the API: only what passes a strict allow-list reaches a privileged command line
    QString invalidName;
    if (!CoprClient::isValidOwnerName(m_owner)) {
        invalidName = m_owner;
    } else if (!CoprClient::isValidProjectName(m_project)) {
        invalidName = m_project;
    } else if (!CoprClient::isValidPackageName(m_packageName)) {
        invalidName = m_packageName;
    } else {
        return true;
    }

    Q_EMIT passiveMessage(i18n("Nothing was done: \"%1\" is not a valid name of a COPR owner, project or package", invalidName));
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Refusing to run a privileged command with the name" << invalidName << "of" << m_owner << "/" << m_project
                                                  << "package:" << m_packageName;
    return false;
}

bool CoprTransaction::canInstallForCurrentChroot()
{
    if (!m_resource) {
        return false;
    }

    // Only a known negative blocks: the chroot of this system is known and either the
    // project does not enable it or the monitor has no build of the package for it.
    // A last build that failed is a warning, an older one may still be published.
    if (!m_resource->isInstallBlocked()) {
        const QString warning = m_resource->coprInstallWarning();
        if (!warning.isEmpty()) {
            Q_EMIT passiveMessage(warning);
        }
        return true;
    }

    Q_EMIT passiveMessage(i18n("This COPR package is not available for your Fedora version."));
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Refusing to install unsupported COPR package:" << m_resource->coprOwner() << "/"
                                                  << m_resource->coprProject() << "package:" << m_packageName
                                                  << "project chroots:" << m_resource->projectChroots() << "package chroots:" << m_resource->availableChroots();
    return false;
}

QString CoprTransaction::processDiagnosticOutput() const
{
    const QString stderrOutput = m_stderrBuffer.trimmed();
    if (!stderrOutput.isEmpty()) {
        return stderrOutput;
    }

    return m_stdoutBuffer.trimmed();
}

void CoprTransaction::startDnf(const QStringList &dnfArguments)
{
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();

    // By its full path: pkexec would look a bare "dnf" up in the PATH of this user, which
    // may have directories that the user can write to, and run what it finds there as root
    const QString dnf = QStandardPaths::findExecutable(QStringLiteral("dnf"), {QStringLiteral("/usr/bin"), QStringLiteral("/usr/sbin")});
    if (dnf.isEmpty()) {
        processError(QProcess::FailedToStart);
        return;
    }

    const QStringList arguments = QStringList{dnf} + dnfArguments;
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Starting process: pkexec" << arguments;
    m_process->start(QStringLiteral("pkexec"), arguments);
}

void CoprTransaction::enableCoprRepo()
{
    m_state = EnableRepo;
    setStatus(CommittingStatus);
    setProgress(10);

    const QString coprRepo = CoprClient::dnfProjectSpec(m_owner, m_project);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Enabling COPR repository:" << coprRepo;

    // Use pkexec to run dnf copr enable with privileges
    QStringList args;
    args << QStringLiteral("copr");
    args << QStringLiteral("enable");
    args << QStringLiteral("-y");  // Auto-accept
    args << QStringLiteral("--");
    args << coprRepo;

    startDnf(args);
}

void CoprTransaction::installPackage()
{
    m_state = InstallPackage;
    setStatus(CommittingStatus);
    setProgress(50);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Installing package:" << m_packageName;

    // Use pkexec to run dnf install with privileges. From the repository of the project
    // only: Fedora may have a package of the same name. Its dependencies come from anywhere.
    QStringList args;
    args << QStringLiteral("install");
    args << QStringLiteral("-y");
    args << QStringLiteral("--from-repo=") + CoprClient::repositoryId(m_owner, m_project);
    args << QStringLiteral("--");
    args << m_packageName;

    startDnf(args);
}

void CoprTransaction::removePackage()
{
    m_state = RemovePackage;
    setStatus(CommittingStatus);

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Removing package:" << m_packageName;
    setProgress(50);

    // Use pkexec to run dnf remove with privileges
    QStringList args;
    args << QStringLiteral("remove");
    args << QStringLiteral("-y");
    if (m_packageCameFromRepository) {
        // Not a package of the same name that replaced it meanwhile. ":ml" has the multilib packages.
        const QString repositoryId = CoprClient::repositoryId(m_owner, m_project);
        args << QStringLiteral("--installed-from-repo=%1,%1:ml").arg(repositoryId);
    }
    args << QStringLiteral("--");
    args << m_packageName;

    startDnf(args);
}

void CoprTransaction::removeCoprRepo()
{
    m_state = RemoveRepo;
    setStatus(CommittingStatus);
    setProgress(80);

    const QString coprRepo = CoprClient::dnfProjectSpec(m_owner, m_project);
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Removing COPR repository:" << coprRepo;

    QStringList args;
    args << QStringLiteral("copr");
    args << QStringLiteral("remove");
    args << QStringLiteral("-y");
    args << QStringLiteral("--");
    args << coprRepo;

    startDnf(args);
}

void CoprTransaction::checkInstalledPackages()
{
    // What dnf did is asked from the system, for every COPR resource: installedPackagesRefreshed() follows
    m_state = CheckInstalledPackages;
    setProgress(m_role == InstallRole ? 95 : 70);
    connect(m_backend->coprInstalledPackages(), &CoprInstalledPackages::refreshed, this, &CoprTransaction::installedPackagesRefreshed, Qt::UniqueConnection);
    m_backend->coprInstalledPackages()->refresh();
}

void CoprTransaction::installedPackagesRefreshed()
{
    if (m_state != CheckInstalledPackages) {
        return;
    }
    m_state = Done;

    if (m_cancelled) {
        setStatus(CancelledStatus);
    } else if (m_role == RemoveRole) {
        finishRemoval();
    } else {
        const CoprInstalledPackages *installed = m_backend->coprInstalledPackages();
        if (installed->isKnown() && installed->installedVersion(m_owner, m_project, m_packageName).isEmpty()) {
            // dnf had nothing to do: the same or a newer version is installed from elsewhere
            Q_EMIT passiveMessage(i18n("dnf finished, but %1 is not installed from this COPR repository", m_packageName));
        }
        setProgress(100);
        setStatus(DoneStatus);
    }
}

void CoprTransaction::finishRemoval()
{
    const CoprInstalledPackages *installed = m_backend->coprInstalledPackages();
    if (installed->isKnown() && !installed->installedVersion(m_owner, m_project, m_packageName).isEmpty()) {
        // dnf had nothing to do: what is installed is no longer what was asked to go
        Q_EMIT passiveMessage(i18n("dnf finished, but %1 is still installed", m_packageName));
        setStatus(DoneWithErrorStatus);
        return;
    }

    const QStringList otherPackages = installed->packagesFromRepository(m_owner, m_project);
    const QString repositoryId = CoprClient::repositoryId(m_owner, m_project);

    // The repository goes only when the removed package came from it and nothing else does.
    // A package that was recognised by its vendor says nothing about this project.
    if (!installed->isKnown() || !m_packageCameFromRepository || !otherPackages.isEmpty()) {
        // Not told to the user: every message of a transaction is shown as an issue
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Keeping COPR repository" << repositoryId << "- installed from it:" << otherPackages;
    } else if (QFile::exists(QStringLiteral("/etc/yum.repos.d/_%1.repo").arg(repositoryId))) {
        removeCoprRepo();
        return;
    }

    setProgress(100);
    setStatus(DoneStatus);
}

void CoprTransaction::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process finished with exit code:" << exitCode << "status:" << exitStatus;
    processOutput();

    // What pkexec itself answers with, dnf was not run. 126: the password dialog was dismissed.
    // 127: not authorised, but also everything else that kept pkexec from running dnf.
    if (exitStatus == QProcess::NormalExit && (exitCode == 126 || exitCode == 127)) {
        const QString error = processDiagnosticOutput();
        if (exitCode == 127 && !error.contains(QLatin1String("Not authorized"))) {
            qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "pkexec failed:" << error;
            Q_EMIT passiveMessage(i18n("Operation failed: %1", error.isEmpty() ? i18n("No error output was returned") : error));
            setStatus(DoneWithErrorStatus);
            return;
        }
        // Not an error and nothing to explain
        setStatus(CancelledStatus);
        return;
    }
    if (m_cancelled) {
        // Whatever dnf got done before it went away is asked from the system
        if (m_state == EnableRepo || m_state == RemoveRepo) {
            m_backend->coprRepositoriesChanged();
        }
        checkInstalledPackages();
        return;
    }

    if (exitStatus == QProcess::CrashExit) {
        setStatus(DoneWithErrorStatus);
        return;
    }

    // The package is gone, and so is the repository: someone else removed it
    const bool repositoryWasGone = m_state == RemoveRepo && processDiagnosticOutput().contains(QLatin1String("not found on this system"));

    if (exitCode != 0 && !repositoryWasGone) {
        const QString error = processDiagnosticOutput();
        const QString errorMessage = error.isEmpty() ? i18n("No error output was returned") : error;
        qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process failed:" << errorMessage;
        Q_EMIT passiveMessage(i18n("Operation failed: %1", errorMessage));

        setStatus(DoneWithErrorStatus);
        return;
    }

    // Move to next state
    switch (m_state) {
    case EnableRepo:
        m_backend->coprRepositoriesChanged();
        installPackage();
        break;
    case InstallPackage:
    case RemovePackage:
        checkInstalledPackages();
        break;
    case RemoveRepo:
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << (repositoryWasGone ? "COPR repository was not there any more" : "COPR repository removed successfully");
        m_backend->coprRepositoriesChanged();
        setProgress(100);
        setStatus(DoneStatus);
        break;
    default:
        break;
    }
}

void CoprTransaction::processError(QProcess::ProcessError error)
{
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process error:" << error;

    // Killed by cancel(): processFinished() follows
    if (m_cancelled && error != QProcess::FailedToStart) {
        return;
    }

    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = i18n("Failed to start the installation process. Make sure 'pkexec' and 'dnf' are installed.");
        break;
    case QProcess::Crashed:
        errorMessage = i18n("The installation process crashed unexpectedly.");
        break;
    case QProcess::Timedout:
        errorMessage = i18n("The installation process timed out.");
        break;
    default:
        errorMessage = i18n("An unknown error occurred during installation.");
        break;
    }

    Q_EMIT passiveMessage(errorMessage);
    setStatus(DoneWithErrorStatus);
}

void CoprTransaction::processOutput()
{
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    if (!output.isEmpty()) {
        m_stdoutBuffer += output;
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process output:" << output;
    }

    QString error = QString::fromUtf8(m_process->readAllStandardError());
    if (!error.isEmpty()) {
        m_stderrBuffer += error;
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process stderr:" << error;
    }
}
