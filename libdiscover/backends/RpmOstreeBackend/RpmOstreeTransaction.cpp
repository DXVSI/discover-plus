/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeTransaction.h"

#include <KLocalizedString>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>

#include "libdiscover_rpm-ostree_debug.h"

static const QString TransactionConnection = QStringLiteral("discover_transaction");
static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");

RpmOstreeTransaction::RpmOstreeTransaction(QObject *parent,
                                           AbstractResource *resource,
                                           OrgProjectatomicRpmostree1SysrootInterface *interface,
                                           Operation operation,
                                           QString arg)
    : Transaction(parent, resource, Transaction::Role::InstallRole, {})
    , m_timer(nullptr)
    , m_operation(operation)
    , m_resource((RpmOstreeResource *)resource)
    , m_cancelled(false)
    , m_interface(interface)
{
    setStatus(Status::SetupStatus);

    // This should never happen. We need a reference to the DBus interface to be
    // able to cancel a running transaction.
    if (interface == nullptr) {
        qCWarning(RPMOSTREE_LOG) << "Error: No DBus interface provided. Please file a bug.";
        passiveMessage(i18n("rpm-ostree backend: Error: No DBus interface provided. Please file a bug."));
        setStatus(Status::CancelledStatus);
        return;
    }

    // Make sure we are asking for a supported operation and set up arguments
    switch (m_operation) {
    case Operation::CheckForUpdate: {
        qCInfo(RPMOSTREE_LOG) << "Starting transaction to check for updates";
        if (m_resource->isClassic()) {
            m_prog = QStringLiteral("rpm-ostree");
            m_args.append({QStringLiteral("update"), QStringLiteral("--check")});
        } else if (m_resource->isOCI()) {
            m_prog = QStringLiteral("skopeo");
            // This will fail on non-remote transports (oci, oci-archive, containers-storage) but
            // that's OK as we can not check for updates in those cases.
            m_args.append({QStringLiteral("inspect"), QStringLiteral("--no-tags"), m_resource->OCIUrl()});
        } else {
            // Should never happen
            qCWarning(RPMOSTREE_LOG) << "Error: Can not start a transaction for resource with an invalid format. Please file a bug.";
            passiveMessage(i18n("rpm-ostree backend: Error: Can not start a transaction for resource with an invalid format. Please file a bug."));
            setStatus(Status::CancelledStatus);
            return;
        }
        break;
    }
    case Operation::DownloadOnly:
        qCInfo(RPMOSTREE_LOG) << "Starting transaction to only download updates";
        m_prog = QStringLiteral("rpm-ostree");
        m_args.append({QStringLiteral("update"), QStringLiteral("--download-only ")});
        break;
    case Operation::Update:
        qCInfo(RPMOSTREE_LOG) << "Starting transaction to update";
        m_prog = QStringLiteral("rpm-ostree");
        m_args.append({QStringLiteral("update")});
        break;
    case Operation::Rebase:
        // This should never happen
        if (arg.isEmpty()) {
            qCWarning(RPMOSTREE_LOG) << "Error: Can not rebase to an empty ref. Please file a bug.";
            passiveMessage(i18n("rpm-ostree backend: Error: Can not rebase to an empty ref. Please file a bug."));
            setStatus(Status::CancelledStatus);
            return;
        }
        qCInfo(RPMOSTREE_LOG) << "Starting transaction to rebase to:" << arg;
        m_prog = QStringLiteral("rpm-ostree");
        m_args.append({QStringLiteral("rebase"), arg});
        break;
    case Operation::Unknown:
        // This is a transaction started externally to Discover. We'll just
        // display it as best as we can.
        qCInfo(RPMOSTREE_LOG) << "Creating a transaction for an operation not started by Discover";
        setupExternalTransaction();
        return;
        break;
    default:
        // This should never happen
        qCWarning(RPMOSTREE_LOG) << "Error: Unknown operation requested. Please file a bug.";
        passiveMessage(i18n("rpm-ostree backend: Error: Unknown operation requested. Please file a bug."));
        setStatus(Status::CancelledStatus);
        return;
    }

    // Directly run the command via a QProcess
    m_process = new QProcess(this);
    m_process->setProgram(m_prog);
    m_process->setArguments(m_args);

    // Store stderr output for later
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        QByteArray message = m_process->readAllStandardError();
        qCWarning(RPMOSTREE_LOG) << m_prog << QLatin1String("(error):") << message;
        m_stderr += message;
    });

    // Store stdout output for later and process it to fake progress
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray message = m_process->readAllStandardOutput();
        qCDebug(RPMOSTREE_LOG) << (m_prog + QStringLiteral(":")) << message;
        m_stdout += message;
        fakeProgress(message);
    });

    // Process the result of the transaction once rpm-ostree is done
    connect(m_process, &QProcess::finished, this, &RpmOstreeTransaction::processCommand);

    // Wait for the start command to effectively start the transaction so that
    // the caller has the time to setup signal/slots connections.
}

RpmOstreeTransaction::~RpmOstreeTransaction()
{
    delete m_timer;
}

void RpmOstreeTransaction::start()
{
    // Calling this function only makes sense if we have a QProcess for the
    // current transaction.
    if (m_process != nullptr) {
        m_process->start();
        setStatus(Status::DownloadingStatus);
        setProgress(5);
        setDownloadSpeed(0);
    }
}

void RpmOstreeTransaction::processCommand(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_process->deleteLater();
    m_process = nullptr;
    if (exitStatus != QProcess::NormalExit) {
        if (m_cancelled) {
            // If the user requested the transaction to be cancelled then we
            // don't need to show any error
            qCWarning(RPMOSTREE_LOG) << "Transaction cancelled: rpm-ostree " << m_args;
        } else {
            // The transaction was cancelled unexpectedly so let's display the
            // error to the user
            qCWarning(RPMOSTREE_LOG) << "Error while calling: rpm-ostree " << m_args;
            passiveMessage(i18n("rpm-ostree transaction failed with:\n%1", QString::fromUtf8(m_stderr)));
        }
        setStatus(Status::CancelledStatus);
        return;
    }
    if (exitCode != 0) {
        if ((m_operation == Operation::CheckForUpdate) && (exitCode == 77)) {
            // rpm-ostree will exit with status 77 when no updates are available
            qCInfo(RPMOSTREE_LOG) << "No updates available";
            // Tell the backend to look for a new major version
            Q_EMIT lookForNextMajorVersion();
            setStatus(Status::DoneStatus);
            return;
        } else if (m_cancelled) {
            // If the user requested the transaction to be cancelled then we
            // don't need to show any error
            qCInfo(RPMOSTREE_LOG) << "Transaction cancelled: rpm-ostree " << m_args;
            setStatus(Status::DoneWithErrorStatus);
            return;
        } else {
            // The transaction failed unexpectedly so let's display the error to
            // the user
            qCWarning(RPMOSTREE_LOG) << "rpm-ostree" << m_args << "returned with an error code:" << exitCode;
            passiveMessage(i18n("rpm-ostree transaction failed with:\n%1", QString::fromUtf8(m_stderr)));
            setStatus(Status::DoneWithErrorStatus);
            return;
        }
    }

    // The transaction was successful. Let's process the result.
    switch (m_operation) {
    case Operation::CheckForUpdate: {
        if (m_resource->isClassic()) {
            // Look for new version in rpm-ostree stdout
            QString newVersion, line;
            QString output = QString::fromUtf8(m_stdout);
            QTextStream stream(&output);
            while (stream.readLineInto(&line)) {
                if (line.contains(QLatin1String("Version: "))) {
                    newVersion = line;
                    break;
                }
            }
            // If we found a new version then offer it as an update
            if (!newVersion.isEmpty()) {
                newVersion = newVersion.trimmed();
                newVersion.remove(0, QStringLiteral("Version: ").length());
                newVersion.remove(newVersion.size() - QStringLiteral(" (XXXX-XX-XXTXX:XX:XXZ)").length(), newVersion.size() - 1);
                qCInfo(RPMOSTREE_LOG) << "Found new version:" << newVersion;
                Q_EMIT newVersionFound(newVersion);
            }
        } else if (m_resource->isOCI()) {
            // Parse stdout as JSON and look at the container image labels for the version
            const QJsonDocument jsonDocument = QJsonDocument::fromJson(m_stdout);
            if (!jsonDocument.isObject()) {
                qCWarning(RPMOSTREE_LOG) << "Could not parse output as JSON:" << m_prog << m_args;
                setStatus(Status::DoneWithErrorStatus);
                return;
            }

            // Get the version stored in 'org.opencontainers.image.version' label
            QString newVersion =
                jsonDocument.object().value(QLatin1String("Labels")).toObject().value(QLatin1String("org.opencontainers.image.version")).toString();
            if (newVersion.isEmpty()) {
                // Check the 'version' label for compatibility with older rpm-ostree releases (< v2024.03)
                newVersion = jsonDocument.object().value(QLatin1String("Labels")).toObject().value(QLatin1String("version")).toString();
                if (newVersion.isEmpty()) {
                    qCWarning(RPMOSTREE_LOG) << "Could not get the version from the container labels";
                    setStatus(Status::DoneWithErrorStatus);
                    return;
                }
            }

            QVersionNumber newVersionNumber = QVersionNumber::fromString(newVersion);
            QVersionNumber currentVersionNumber = QVersionNumber::fromString(m_resource->version());
            if (newVersionNumber <= currentVersionNumber) {
                qCInfo(RPMOSTREE_LOG) << "No new version found";
            } else {
                qCInfo(RPMOSTREE_LOG) << "Found new version:" << newVersion;
                Q_EMIT newVersionFound(newVersion);
            }
        } else {
            // Should never happen
            qCWarning(RPMOSTREE_LOG) << "Error: Unknown resource format. Please file a bug.";
            passiveMessage(i18n("rpm-ostree backend: Error: Unknown resource format. Please file a bug."));
            setStatus(Status::DoneWithErrorStatus);
            return;
        }

        // Always tell the backend to look for a new major version
        Q_EMIT lookForNextMajorVersion();

        break;
    }
    case Operation::DownloadOnly:
        // Nothing to do here after downloading pending updates.
        break;
    case Operation::Update:
        // Refresh ressources (deployments) and update state
        Q_EMIT deploymentsUpdated();
        break;
    case Operation::Rebase:
        // Refresh ressources (deployments) and update state
        Q_EMIT deploymentsUpdated();
        // Tell the backend to refresh the new major version message now that
        // we've reabsed to the new version
        Q_EMIT lookForNextMajorVersion();
        break;
    case Operation::Unknown:
    default:
        // This should never happen
        qCWarning(RPMOSTREE_LOG) << "Error: Unknown operation requested. Please file a bug.";
        setStatus(Status::DoneStatus);
    }
    setStatus(Status::DoneStatus);
}

void RpmOstreeTransaction::setupExternalTransaction()
{
    // Create a timer to periodically look for updates on the transaction
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(2000);

    // Update transaction status
    connect(m_timer, &QTimer::timeout, this, [this]() {
        // Is the transaction finished?
        qCDebug(RPMOSTREE_LOG) << "External transaction update timer triggered";
        QString transaction = m_interface->activeTransactionPath();
        if (transaction.isEmpty()) {
            qCInfo(RPMOSTREE_LOG) << "External transaction finished";
            Q_EMIT deploymentsUpdated();
            setStatus(Status::DoneStatus);
            return;
        }

        // Read status and fake progress
        QStringList transactionInfo = m_interface->activeTransaction();
        if (transactionInfo.length() != 3) {
            qCInfo(RPMOSTREE_LOG) << "External transaction:" << transactionInfo;
        } else {
            qCInfo(RPMOSTREE_LOG) << "External transaction '" << transactionInfo.at(0) << "' requested by '" << transactionInfo.at(1);
        }
        fakeProgress({});

        // Restart the timer
        m_timer->start();
    });

    // Setup status, fake progress and start the timer
    setStatus(Status::DownloadingStatus);
    setProgress(5);
    setDownloadSpeed(0);
    m_timer->start();
}

void RpmOstreeTransaction::fakeProgress(const QByteArray &msg)
{
    QString message = QString::fromUtf8(msg);
    int progress = this->progress();
    if (message.contains(QLatin1String("Receiving metadata objects"))) {
        progress += 10;
    } else if (message.contains(QLatin1String("Checking out tree"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Enabled rpm-md repositories:"))) {
        progress += 1;
    } else if (message.contains(QLatin1String("Updating metadata for"))) {
        progress += 1;
    } else if (message.contains(QLatin1String("rpm-md repo"))) {
        progress += 1;
    } else if (message.contains(QLatin1String("Resolving dependencies"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Applying")) && (message.contains(QLatin1String("overrides")) || message.contains(QLatin1String("overlays")))) {
        progress += 5;
        setStatus(Status::CommittingStatus);
    } else if (message.contains(QLatin1String("Processing packages"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Running pre scripts"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Running post scripts"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Running posttrans scripts"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Writing rpmdb"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Generating initramfs"))) {
        progress += 10;
    } else if (message.contains(QLatin1String("Writing OSTree commit"))) {
        progress += 10;
        setCancellable(false);
    } else if (message.contains(QLatin1String("Staging deployment"))) {
        progress += 5;
    } else if (message.contains(QLatin1String("Freed"))) {
        progress += 1;
    } else if (message.contains(QLatin1String("Upgraded"))) {
        progress = 99;
    } else {
        progress += 1;
    }
    // As we're faking progress, let's make sure that it stays in expected bounds
    setProgress(qBound(1, progress, 99));
}

void RpmOstreeTransaction::cancel()
{
    qCInfo(RPMOSTREE_LOG) << "Cancelling current transaction";
    passiveMessage(i18n("Cancelling rpm-ostree transaction. This may take some time. Please wait."));

    // Cancel directly using the DBus interface to work in all cases whether we
    // started the transaction or if it's an externally started one.
    QString transaction = m_interface->activeTransactionPath();
    QDBusConnection peerConnection = QDBusConnection::connectToPeer(transaction, TransactionConnection);
    OrgProjectatomicRpmostree1TransactionInterface transactionInterface(DBusServiceName, QStringLiteral("/"), peerConnection, this);
    auto reply = transactionInterface.Cancel();

    // Cancelled marker that is used to avoid displaying an error message to the
    // user when they asked to cancel a transaction.
    m_cancelled = true;

    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(reply, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this, [callWatcher]() {
        callWatcher->deleteLater();
        QDBusConnection::disconnectFromPeer(TransactionConnection);
    });
}

void RpmOstreeTransaction::proceed()
{
    qCInfo(RPMOSTREE_LOG) << "proceed";
}

QString RpmOstreeTransaction::name() const
{
    switch (m_operation) {
    case Operation::CheckForUpdate:
        return i18n("Checking for a system update");
        break;
    case Operation::DownloadOnly:
        return i18n("Downloading system update. Please be patient as progress is not reported.");
        break;
    case Operation::Update:
        return i18n("Updating the system. Please be patient as progress is not reported.");
        break;
    case Operation::Rebase:
        return i18n("Updating to the next major version. Please be patient as progress is not reported.");
        break;
    case Operation::Unknown:
        return i18n("Operation in progress (started outside of Discover)");
        break;
    default:
        break;
    }
    // This should never happen
    return i18n("Unknown transaction type");
}

QVariant RpmOstreeTransaction::icon() const
{
    return QStringLiteral("folder-rpm-symbolic");
}

#include "moc_RpmOstreeTransaction.cpp"
