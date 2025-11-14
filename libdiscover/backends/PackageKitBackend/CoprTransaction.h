#ifndef COPRTRANSACTION_H
#define COPRTRANSACTION_H

#include <Transaction/Transaction.h>
#include <QProcess>

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
    void enableCoprRepo();
    void installPackage();
    void removePackage();
    void disableCoprRepo();

    CoprResource *m_resource;
    PackageKitBackend *m_backend;
    QProcess *m_process;
    Transaction::Role m_role;
    enum State {
        EnableRepo,
        InstallPackage,
        DisableRepo,
        Done
    };
    State m_state;
};

#endif // COPRTRANSACTION_H