/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateTransaction.h"
#include "SystemdSysupdateBackend.h"
#include "libdiscover_systemdsysupdate_debug.h"

#include <QCoroDBusPendingReply>

#define SYSTEMDSYSUPDATE_LOG LIBDISCOVER_BACKEND_SYSTEMDSYSUPDATE_LOG

constexpr QLatin1StringView PROGRESS_PROPERTY_NAME("Progress");
const QLatin1StringView SYSUPDATE_JOB_INTERFACE_NAME = QLatin1String(org::freedesktop::sysupdate1::Job::staticInterfaceName());
// empty means latest and by default doesn't require authorization
const QString TO_VERSION;
// flags - currently unused and expected to be 0
const int FLAGS = 0;

SystemdSysupdateTransaction::SystemdSysupdateTransaction(AbstractResource *resource, org::freedesktop::sysupdate1::Target *target)
    : Transaction(resource, resource, InstallRole)
    , m_resource(resource)
    , m_target(target)
{
    // Can't cancel until we have a job ID
    setCancellable(false);

    // Start downloading the files (acquiring)
    setStatus(DownloadingStatus);
    auto backend = qobject_cast<SystemdSysupdateBackend *>(m_resource->backend());
    Q_ASSERT(backend);
    connect(backend, &SystemdSysupdateBackend::transactionRemoved, this, &SystemdSysupdateTransaction::acquireDone);
    auto acquireCall = m_target->Acquire(TO_VERSION, FLAGS);
    auto watcher = new QDBusPendingCallWatcher(acquireCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const SystemdSysupdateUpdateReply reply = *watcher;

        if (reply.isError()) {
            Q_EMIT passiveMessage(reply.error().message());
            setStatus(DoneWithErrorStatus);
            return;
        }

        const auto path = reply.argumentAt<2>();

        qCInfo(SYSTEMDSYSUPDATE_LOG) << "Created sysupdate1::Job with path " << path << "for Acquire";

        // need to keep a reference to this to be able to cancel
        m_job = new org::freedesktop::sysupdate1::Job(SYSUPDATE1_SERVICE, path.path(), SystemdSysupdateBackend::OUR_BUS(), this);
        m_job->setInteractiveAuthorizationAllowed(true); // cancel may require authorization
        setCancellable(true);

        // Don't need to keep this around because we're just connecting to some signals (and it'll be deleted by Qt with the parent)
        auto *properties = new org::freedesktop::DBus::Properties(SYSUPDATE1_SERVICE, path.path(), SystemdSysupdateBackend::OUR_BUS(), this);
        connect(properties,
                &org::freedesktop::DBus::Properties::PropertiesChanged,
                this,
                [this, properties](const QString &interface, const QVariantMap &changed, const QStringList &invalidated) -> QCoro::Task<> {
                    if (interface != SYSUPDATE_JOB_INTERFACE_NAME) {
                        co_return;
                    }

                    qCDebug(SYSTEMDSYSUPDATE_LOG) << "Properties changed:" << changed << "Invalidated:" << invalidated;

                    if (changed.contains(PROGRESS_PROPERTY_NAME)) {
                        setProgress(changed.value(PROGRESS_PROPERTY_NAME).toUInt());
                    }

                    if (invalidated.contains(PROGRESS_PROPERTY_NAME)) {
                        const auto reply = co_await properties->Get(SYSUPDATE_JOB_INTERFACE_NAME, PROGRESS_PROPERTY_NAME);
                        if (reply.isError()) {
                            qCCritical(SYSTEMDSYSUPDATE_LOG) << "Failed to get progress:" << reply.error().message();
                            co_return;
                        }

                        setProgress(reply.argumentAt(0).toUInt());
                    }
                });
    });
}

void SystemdSysupdateTransaction::acquireDone(qulonglong jobId, const QDBusObjectPath &jobPath, int status)
{
    Q_UNUSED(jobId);
    qCInfo(SYSTEMDSYSUPDATE_LOG) << "Acquire job" << jobPath.path() << "for target" << m_resource->name() << "finished with status" << status;
    auto backend = qobject_cast<SystemdSysupdateBackend *>(m_resource->backend());
    Q_ASSERT(backend);
    disconnect(backend, &SystemdSysupdateBackend::transactionRemoved, this, &SystemdSysupdateTransaction::acquireDone);
    if (status == 0) {
        // We are already installing, do not cancel
        setCancellable(false);
        setStatus(CommittingStatus);
        connect(backend, &SystemdSysupdateBackend::transactionRemoved, this, &SystemdSysupdateTransaction::installDone);
        auto installCall = m_target->Install(TO_VERSION, FLAGS);

        auto watcher = new QDBusPendingCallWatcher(installCall, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            const SystemdSysupdateUpdateReply reply = *watcher;

            if (reply.isError()) {
                Q_EMIT passiveMessage(reply.error().message());
                setStatus(DoneWithErrorStatus);
                return;
            }

            const auto path = reply.argumentAt<2>();

            qCInfo(SYSTEMDSYSUPDATE_LOG) << "Created sysupdate1::Job with path " << path << "for Install";
        });
    } else {
        setStatus(DoneWithErrorStatus);
        deleteLater();
    }
}

void SystemdSysupdateTransaction::installDone(qulonglong jobId, const QDBusObjectPath &jobPath, int status)
{
    Q_UNUSED(jobId);
    qCInfo(SYSTEMDSYSUPDATE_LOG) << "Install job" << jobPath.path() << "for target" << m_resource->name() << "finished with status" << status;
    auto backend = qobject_cast<SystemdSysupdateBackend *>(m_resource->backend());
    Q_ASSERT(backend);
    disconnect(backend, &SystemdSysupdateBackend::transactionRemoved, this, &SystemdSysupdateTransaction::installDone);
    setStatus(status == 0 ? DoneStatus : DoneWithErrorStatus);
    deleteLater();
}

void SystemdSysupdateTransaction::cancel()
{
    if (!m_job) {
        qWarning(SYSTEMDSYSUPDATE_LOG) << "Can't cancel transaction without a job";
        return;
    }

    auto watcher = new QDBusPendingCallWatcher(m_job->Cancel(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qWarning(SYSTEMDSYSUPDATE_LOG) << "Failed to cancel job:" << watcher->error().message();
            Q_EMIT passiveMessage(watcher->error().message());
            return;
        }

        qDebug(SYSTEMDSYSUPDATE_LOG) << "Job cancelled";
    });
    setStatus(CancelledStatus);
}
