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
        if (!canInstallForCurrentChroot()) {
            setStatus(DoneWithErrorStatus);
            return;
        }
        enableCoprRepo();
    } else if (m_role == RemoveRole) {
        removePackage();
    }
}

bool CoprTransaction::canInstallForCurrentChroot()
{
    if (!m_resource) {
        return false;
    }

    if (m_resource->availableChroots().isEmpty() || m_resource->isAvailableForCurrentFedora()) {
        return true;
    }

    Q_EMIT passiveMessage(i18n("This COPR package is not available for your Fedora version."));
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Refusing to install unsupported COPR package:" << m_resource->coprOwner() << "/"
                                                  << m_resource->coprProject() << "package:" << m_resource->packageName()
                                                  << "available chroots:" << m_resource->availableChroots();
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

void CoprTransaction::startPkexec(const QStringList &arguments)
{
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Starting process: pkexec" << arguments;
    m_process->start(QStringLiteral("pkexec"), arguments);
}

void CoprTransaction::enableCoprRepo()
{
    m_state = EnableRepo;
    setStatus(CommittingStatus);
    setProgress(10);

    if (!m_resource) {
        setStatus(DoneWithErrorStatus);
        return;
    }
    QString coprRepo = m_resource->coprOwner() + QStringLiteral("/") + m_resource->coprProject();

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Enabling COPR repository:" << coprRepo;

    // Use pkexec to run dnf copr enable with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("copr");
    args << QStringLiteral("enable");
    args << QStringLiteral("-y");  // Auto-accept
    args << coprRepo;

    startPkexec(args);
}

void CoprTransaction::installPackage()
{
    m_state = InstallPackage;
    setStatus(CommittingStatus);
    setProgress(50);

    if (!m_resource) {
        setStatus(DoneWithErrorStatus);
        return;
    }
    QString packageName = m_resource->packageName();
    if (packageName.isEmpty()) {
        Q_EMIT passiveMessage(i18n("No installable package is known for this COPR project yet"));
        setStatus(DoneWithErrorStatus);
        return;
    }

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Installing package:" << packageName;

    // Use pkexec to run dnf install with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("install");
    args << QStringLiteral("-y");
    args << packageName;

    startPkexec(args);
}

void CoprTransaction::removePackage()
{
    m_state = InstallPackage;  // Reuse same state for removal
    setStatus(CommittingStatus);

    if (!m_resource) {
        setStatus(DoneWithErrorStatus);
        return;
    }
    QString packageName = m_resource->packageName();
    if (packageName.isEmpty()) {
        Q_EMIT passiveMessage(i18n("No installed package is known for this COPR resource"));
        setStatus(DoneWithErrorStatus);
        return;
    }

    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Removing package:" << packageName;
    setProgress(50);

    // Use pkexec to run dnf remove with privileges
    QStringList args;
    args << QStringLiteral("dnf");
    args << QStringLiteral("remove");
    args << QStringLiteral("-y");
    args << packageName;

    startPkexec(args);
}

void CoprTransaction::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Process finished with exit code:" << exitCode << "status:" << exitStatus;
    processOutput();

    if (exitStatus == QProcess::CrashExit) {
        setStatus(DoneWithErrorStatus);
        return;
    }

    if (exitCode != 0) {
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
        if (m_role == InstallRole) {
            installPackage();
        }
        break;
    case InstallPackage:
        if (m_role == RemoveRole) {
            if (m_resource) {
                m_resource->setState(AbstractResource::None);
                qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Package removed successfully, COPR repository left enabled";
            }
            setProgress(100);
            setStatus(DoneStatus);
        } else {
            // Update resource state to installed
            if (m_resource) {
                m_resource->setState(AbstractResource::Installed);
                qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Package installed successfully, state updated to Installed";
            }
            setProgress(100);
            setStatus(DoneStatus);
        }
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
