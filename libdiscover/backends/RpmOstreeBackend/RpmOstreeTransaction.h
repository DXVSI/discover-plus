/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "RpmOstreeDBusInterface.h"
#include "RpmOstreeResource.h"

#include <Transaction/Transaction.h>

#include <QTimer>

/*
 * Internal representation of an actual rpm-ostree transaction in progress.
 */
class RpmOstreeTransaction : public Transaction
{
    Q_OBJECT
public:
    enum Operation {
        /// Only check if an update is available. Note that this is unreliable
        /// but good enough for now:
        /// Note: --check and --preview may be unreliable.
        /// See https://github.com/coreos/rpm-ostree/issues/1579
        CheckForUpdate = 0,
        /// Only download update but do not apply it.
        DownloadOnly,
        /// Download and apply update.
        Update,
        /// Rebase to a new major version. We do not support rebasing to
        /// arbitrary refs.
        Rebase,
        /// Used to represent transactions that were not started by Discover
        Unknown,
    };
    Q_ENUM(Operation)

    /* Note: arg is only used to pass a reference for the rebase operation. it
     * is ignored in all other cases. */
    RpmOstreeTransaction(AbstractResource *resource, OrgProjectatomicRpmostree1SysrootInterface *interface, Operation operation, QString arg = {});
    ~RpmOstreeTransaction();

    Q_SCRIPTABLE void cancel() override;
    Q_SCRIPTABLE void proceed() override;

    QString name() const override;
    QVariant icon() const override;

    /* Effectively start the transaction when calling an rpm-ostree command */
    void start();

Q_SIGNALS:
    /* Emitted when a new version is found and an update can be started */
    void newVersionFound(QString version);

    /* Emitted if no update is found and if the backend should look for a new
     * major version */
    void lookForNextMajorVersion();

    /* Emitted when an operation completed and the backend should refresh the
     * listed deployments */
    void deploymentsUpdated();

public Q_SLOTS:
    /* Process the result of rpm-ostree commands */
    void processCommand(int exitCode, QProcess::ExitStatus exitStatus);

private:
    /* Timer setup for transactions started externally from Discover */
    void setupExternalTransaction();

    /* We don't have clear progress info, so we're faking it */
    void fakeProgress(const QByteArray &message);

    /* Timer wokaround for Transaction updates when the transaction has not been
     * started by Discover */
    QTimer *m_timer;

    /* Operation requested for this transaction */
    Operation m_operation;

    /* The resource (deployment) we're working on */
    RpmOstreeResource *m_resource;

    /* QProcess used to call rpm-ostree */
    QProcess *m_process;

    /* Set when we cancel an in progress transaction */
    bool m_cancelled;

    /* Command line application called to perform the transaction (usually rpm-ostree) */
    QString m_prog;

    /* Arguments passed to the command performing the transaction (usually rpm-ostree) */
    QStringList m_args;

    /* rpm-ostree DBus interface, used to cancel running transactions */
    OrgProjectatomicRpmostree1SysrootInterface *m_interface;

    /* Store standard output from rpm-ostree command line calls */
    QByteArray m_stdout;

    /* Store standard error output from rpm-ostree command line calls */
    QByteArray m_stderr;
};
