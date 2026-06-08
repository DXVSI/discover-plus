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

private:
    bool canInstallForCurrentChroot();
    QString processDiagnosticOutput() const;
    void startPkexec(const QStringList &arguments);
    void enableCoprRepo();
    void installPackage();
    void removePackage();
    void removeCoprRepo();

    QPointer<CoprResource> m_resource;
    PackageKitBackend *m_backend;
    QProcess *m_process;
    Transaction::Role m_role;
    QString m_stdoutBuffer;
    QString m_stderrBuffer;
    enum State {
        EnableRepo,
        InstallPackage,
        RemovePackage,
        RemoveRepo,
        Done
    };
    State m_state;
};

#endif // COPRTRANSACTION_H
