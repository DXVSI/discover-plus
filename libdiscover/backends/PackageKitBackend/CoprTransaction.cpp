#include "CoprTransaction.h"
#include "CoprResource.h"
#include "PackageKitBackend.h"
#include "libdiscover_backend_packagekit_debug.h"

#include <KLocalizedString>
#include <QProcess>
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
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(5000)) {
            m_process->kill();
        }
    }

    setStatus(CancelledStatus);
}

void CoprTransaction::proceed()
{
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "CoprTransaction::proceed() - role:" << m_role;
    setStatus(DownloadingStatus);

    if (m_role == InstallRole) {
        enableCoprRepo();
    } else if (m_role == RemoveRole) {
        removePackage();
    }
}

void CoprTransaction::enableCoprRepo()
{
    m_state = EnableRepo;
    setStatus(CommittingStatus);
    setProgress(10);

    QString coprRepo = m_resource->coprOwner() + QStringLiteral("/") + m_resource->coprProject();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Enabling COPR repository:" << coprRepo;

    // Use pkexec to run dnf copr enable with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("copr");
    args << QStringLiteral("enable");
    args << QStringLiteral("-y");  // Auto-accept
    args << coprRepo;

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Starting process: pkexec" << args;
    m_process->start(QStringLiteral("pkexec"), args);

    if (!m_process->waitForStarted(5000)) {
        qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Failed to start process";
        Q_EMIT passiveMessage(i18n("Failed to start installation process"));
        setStatus(DoneWithErrorStatus);
    }
}

void CoprTransaction::installPackage()
{
    m_state = InstallPackage;
    setStatus(CommittingStatus);
    setProgress(50);

    QString packageName = m_resource->packageName();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Installing package:" << packageName;

    // Use pkexec to run dnf install with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("install");
    args << QStringLiteral("-y");
    args << packageName;

    m_process->start(QStringLiteral("pkexec"), args);
}

void CoprTransaction::removePackage()
{
    m_state = InstallPackage;  // Reuse same state for removal
    setStatus(CommittingStatus);

    QString packageName = m_resource->packageName();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Removing package:" << packageName;
    setProgress(50);

    // Use pkexec to run dnf remove with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("remove");
    args << QStringLiteral("-y");
    args << packageName;

    m_process->start(QStringLiteral("pkexec"), args);
}

void CoprTransaction::disableCoprRepo()
{
    m_state = DisableRepo;
    setStatus(CommittingStatus);
    setProgress(90);

    QString coprRepo = m_resource->coprOwner() + QStringLiteral("/") + m_resource->coprProject();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Disabling COPR repository:" << coprRepo;

    // Use pkexec to run dnf copr disable with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("copr");
    args << QStringLiteral("disable");
    args << QStringLiteral("-y");
    args << coprRepo;

    m_process->start(QStringLiteral("pkexec"), args);
}

void CoprTransaction::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process finished with exit code:" << exitCode << "status:" << exitStatus;

    if (exitStatus == QProcess::CrashExit) {
        setStatus(DoneWithErrorStatus);
        return;
    }

    if (exitCode != 0) {
        QString error = QString::fromUtf8(m_process->readAllStandardError());
        qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process failed:" << error;
        Q_EMIT passiveMessage(i18n("Operation failed: %1", error));

        setStatus(DoneWithErrorStatus);
        return;
    }

    // Move to next state
    switch (m_state) {
    case EnableRepo:
        if (m_role == InstallRole) {
            installPackage();
        }
        break;
    case InstallPackage:
        if (m_role == RemoveRole) {
            // After removing the package, also disable the COPR repository
            disableCoprRepo();
        } else {
            // Update resource state to installed
            m_resource->setState(AbstractResource::Installed);
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Package installed successfully, state updated to Installed";
            setProgress(100);
            setStatus(DoneStatus);
            Q_EMIT passiveMessage(i18n("Package %1 has been installed", m_resource->name()));
        }
        break;
    case DisableRepo:
        // Update resource state to not installed
        m_resource->setState(AbstractResource::None);
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Package and repository removed successfully";
        setProgress(100);
        setStatus(DoneStatus);
        Q_EMIT passiveMessage(i18n("Package %1 and its COPR repository have been removed", m_resource->name()));
        break;
    default:
        break;
    }
}

void CoprTransaction::processError(QProcess::ProcessError error)
{
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process error:" << error;

    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = i18n("Failed to start the installation process. Make sure 'dnf' is installed.");
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
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process output:" << output;
    }

    QString error = QString::fromUtf8(m_process->readAllStandardError());
    if (!error.isEmpty()) {
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process stderr:" << error;
    }
}