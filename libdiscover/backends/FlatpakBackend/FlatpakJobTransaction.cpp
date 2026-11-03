/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakJobTransaction.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "FlatpakTransactionThread.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <QDebug>
#include <QScopeGuard>
#include <QTimer>
namespace
{
    struct InstallationContext {
        FlatpakJobTransaction::Role role;
        FlatpakInstallation *installation;

        bool operator==(const InstallationContext &) const = default;
    };

    uint qHash(const InstallationContext &context, uint seed)
    {
        return qHash(context.role, seed) ^ qHash(context.installation, seed);
    }
}

class FlatpakTransactionsMerger : public QObject
{
    Q_OBJECT
public:
    [[nodiscard]] static FlatpakTransactionsMerger *instance()
    {
        static FlatpakTransactionsMerger merger;
        return &merger;
    }

    ~FlatpakTransactionsMerger() override
    {
        for (const auto &thread : std::as_const(m_activeThreads)) {
            thread->cancel();
            if (FlatpakThreadPool::instance()->tryTake(thread)) { // immediately delete if the runnable hasn't started yet
                delete thread;
            } else { // otherwise defer cleanup to the pool
                thread->setAutoDelete(true);
            }
        }
    }

    Q_DISABLE_COPY_MOVE(FlatpakTransactionsMerger)

    void schedule(FlatpakJobTransaction *transaction)
    {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Scheduling transaction" << transaction;
        m_pendingJobTransactions.push_back(transaction);
        connect(&m_timer, &QTimer::timeout, this, &FlatpakTransactionsMerger::dispatch, Qt::UniqueConnection);
        // We use a 0 timer because we merge all transactions that are being scheduled in one eventloop run
        m_timer.setSingleShot(true);
        m_timer.start(0);
    }

    void discard(FlatpakJobTransaction *transaction)
    {
        m_pendingJobTransactions.removeAll(transaction);
    }

private Q_SLOTS:
    void dispatch()
    {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Dispatching from amount of jobs" << m_pendingJobTransactions.size();
        if (m_pendingJobTransactions.isEmpty()) {
            return;
        }

        QHash<InstallationContext, FlatpakTransactionThread *> threadsByInstallation;

        for (const auto &pendingJobTransaction : std::as_const(m_pendingJobTransactions)) {
            Q_ASSERT(pendingJobTransaction->m_app);
            auto installation = pendingJobTransaction->m_app->installation();
            Q_ASSERT(installation);
            auto role = pendingJobTransaction->role();

            InstallationContext installationContext{.role = role, .installation = installation};
            if (!threadsByInstallation.contains(installationContext)) {
                auto thread = new FlatpakTransactionThread(installationContext.role, installationContext.installation);
                connect(thread, &QObject::destroyed, this, [this, thread] {
                    m_activeThreads.removeAll(thread);
                });
                m_activeThreads.append(thread);
                threadsByInstallation.insert(installationContext, thread);
            }

            auto &thread = threadsByInstallation[installationContext];
            thread->setAutoDelete(false);
            thread->addJobTransaction(pendingJobTransaction);
            pendingJobTransaction->m_thread = thread;
        }
        m_pendingJobTransactions.clear();

        for (auto &thread : std::as_const(threadsByInstallation)) {
            FlatpakThreadPool::instance()->start(thread);
        }
    }

private:
    using QObject::QObject;

    // Purely exists for cleanup. Do not call into these! Do not recycle these! They have been dispatched to the pool.
    QList<FlatpakTransactionThread *> m_activeThreads;
    QList<FlatpakJobTransaction *> m_pendingJobTransactions;
    QTimer m_timer;
};

FlatpakJobTransaction::FlatpakJobTransaction(FlatpakResource *app, Role role)
    : Transaction(app, role, {})
    , m_app(app)
{
    setCancellable(true);

    setStatus(QueuedStatus);
    FlatpakTransactionsMerger::instance()->schedule(this);
}

FlatpakJobTransaction::~FlatpakJobTransaction()
{
    FlatpakTransactionsMerger::instance()->discard(this);
}

void FlatpakJobTransaction::cancel()
{
    if (m_thread) {
        m_thread->cancel();
    }
}

void FlatpakJobTransaction::finishTransaction(bool cancelled, const QString &errorMessage, const FlatpakTransactionThread::Repositories &addedRepositories, bool success)
{
    if (static_cast<FlatpakBackend *>(m_app->backend())->getInstalledRefForApp(m_app)) {
        m_app->setState(AbstractResource::Installed);
    } else {
        m_app->setState(AbstractResource::None);
    }

    if (addedRepositories.isEmpty()) {
        Q_EMIT repositoriesAdded(addedRepositories);
    }

    if (!cancelled && !errorMessage.isEmpty()) {
        Q_EMIT passiveMessage(errorMessage);
    }

    if (success) {
        setStatus(DoneStatus);
    } else {
        setStatus(cancelled ? CancelledStatus : DoneWithErrorStatus);
    }
}

void FlatpakJobTransaction::proceed()
{
    if (m_thread) {
        m_thread->proceed();
    }
}

#include "FlatpakJobTransaction.moc"
#include "moc_FlatpakJobTransaction.cpp"
