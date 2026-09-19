/*
 *   SPDX-FileCopyrightText: 2025-2026 DXVSI <https://github.com/DXVSI>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COPRTRANSACTION_H
#define COPRTRANSACTION_H

#include <QPointer>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <Transaction/Transaction.h>

class CoprResource;
class PackageKitBackend;

class CoprTransaction : public Transaction
{
    Q_OBJECT
public:
    explicit CoprTransaction(CoprResource *resource, Transaction::Role role, PackageKitBackend *backend);
    ~CoprTransaction() override;

    void cancel() override;
    void proceed() override;

private Q_SLOTS:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void processOutput();

    void installedPackagesRefreshed();

private:
    bool hasValidNames();
    bool canInstallForCurrentChroot();
    QString processDiagnosticOutput() const;
    void startDnf(const QStringList &dnfArguments);
    void enableCoprRepo();
    void installPackage();
    void removePackage();
    void removeCoprRepo();
    void checkInstalledPackages();
    void finishRemoval();

    QPointer<CoprResource> m_resource;
    PackageKitBackend *m_backend;
    QProcess *m_process;
    Transaction::Role m_role;
    // Taken once in proceed(): the selection of the resource may change while pkexec runs
    QString m_packageName;
    QString m_owner;
    QString m_project;
    // dnf recorded that the package to remove came from the repository of this project
    bool m_packageCameFromRepository = false;
    bool m_cancelled = false;
    QString m_stdoutBuffer;
    QString m_stderrBuffer;
    enum State {
        EnableRepo,
        InstallPackage,
        RemovePackage,
        CheckInstalledPackages,
        RemoveRepo,
        Done
    };
    State m_state;
};

#endif // COPRTRANSACTION_H
