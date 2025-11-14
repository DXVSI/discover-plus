/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakTransactionThread.h"
#include "FlatpakJobTransaction.h"
#include "FlatpakResource.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <KLocalizedString>
#include <KOSRelease>
#include <QDesktopServices>

#include <span>

using namespace Qt::StringLiterals;

static int FLATPAK_CLI_UPDATE_FREQUENCY = 150;

gboolean FlatpakTransactionThread::add_new_remote_cb(FlatpakTransaction *object,
                                                     gint /*reason*/,
                                                     gchar *from_id,
                                                     gchar *suggested_remote_name,
                                                     gchar *url,
                                                     gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);

    // TODO ask instead
    auto name = QString::fromUtf8(suggested_remote_name);
    obj->m_addedRepositories[FlatpakResource::installationPath(flatpak_transaction_get_installation(object))].append(name);
    Q_EMIT obj->passiveMessage(i18n("Adding remote '%1' in %2 from %3", name, QString::fromUtf8(url), QString::fromUtf8(from_id)));
    return true;
}

void FlatpakTransactionThread::progress_changed_cb(FlatpakTransactionProgress *progress, gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);

    if (flatpak_transaction_progress_get_is_estimating(progress)) {
        Q_EMIT obj->statusChanged(Transaction::SetupStatus);
        return;
    }
    // We do not know if downloading or installing, but downloading generally takes longer
    Q_EMIT obj->statusChanged(Transaction::DownloadingStatus);

    g_autolist(GObject) ops = flatpak_transaction_get_operations(obj->m_transaction);
    g_autoptr(FlatpakTransactionOperation) op = flatpak_transaction_get_current_operation(obj->m_transaction);
    const int idx = g_list_index(ops, op);
    obj->setProgress(qMin<int>(99, (100 * idx + flatpak_transaction_progress_get_progress(progress)) / g_list_length(ops)));

#ifdef FLATPAK_VERBOSE_PROGRESS
    guint64 start_time = flatpak_transaction_progress_get_start_time(progress);
    guint64 elapsed_time = (g_get_monotonic_time() - start_time) / G_USEC_PER_SEC;
    if (elapsed_time > 0) {
        guint64 transferred = flatpak_transaction_progress_get_bytes_transferred(progress);
        obj->setSpeed(transferred / elapsed_time);
    }
#endif
}

void FlatpakTransactionThread::new_operation_cb(FlatpakTransaction * /*object*/,
                                                FlatpakTransactionOperation *operation,
                                                FlatpakTransactionProgress *progress,
                                                gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);

    obj->setCurrentRef(flatpak_transaction_operation_get_ref(operation));

    g_signal_connect(progress, "changed", G_CALLBACK(&FlatpakTransactionThread::progress_changed_cb), obj);
    flatpak_transaction_progress_set_update_frequency(progress, FLATPAK_CLI_UPDATE_FREQUENCY);
}

void operation_error_cb(FlatpakTransaction * /*object*/, FlatpakTransactionOperation * /*operation*/, GError *error, gint /*details*/, gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);
    obj->operationError(error);
}

gboolean
FlatpakTransactionThread::webflowStart(FlatpakTransaction *transaction, const char *remote, const char *url, GVariant *options, guint id, gpointer user_data)
{
    Q_UNUSED(transaction);
    Q_UNUSED(options);

    QUrl webflowUrl(QString::fromUtf8(url));
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "starting web flow" << webflowUrl << remote << id;
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);
    obj->m_webflows << id;
    Q_EMIT obj->webflowStarted(webflowUrl, id);
    return true;
}

void FlatpakTransactionThread::webflowDoneCallback(FlatpakTransaction *transaction, GVariant *options, guint id, gpointer user_data)
{
    Q_UNUSED(transaction);
    Q_UNUSED(options);
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);
    obj->m_webflows << id;
    Q_EMIT obj->webflowDone(id);
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "webflow done" << id;
}

namespace Callbacks
{
void operation_done([[maybe_unused]] FlatpakTransaction *transaction,
                    [[maybe_unused]] FlatpakTransactionOperation *operation,
                    const char *commit,
                    [[maybe_unused]] FlatpakTransactionResult details)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << commit;
}

int choose_remote_for_ref([[maybe_unused]] FlatpakTransaction *transaction,
                          const char *for_ref,
                          const char *runtime_ref,
                          const char *const *remotes,
                          gpointer user_data)
{
    const auto remotesCount = g_strv_length(const_cast<char **>(remotes));
    if (LIBDISCOVER_BACKEND_FLATPAK_LOG().isDebugEnabled()) {
        const auto remotesSpan = std::span{remotes, remotesCount};
        QStringList remotesStrings;
        remotesStrings.reserve(remotesCount);
        for (const auto &remote : remotesSpan) {
            remotesStrings.append(QString::fromUtf8(remote));
        }
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << for_ref << runtime_ref << remotesStrings;
    }
    // NOTE: this shouldn't actually happen for us, we always know the remote beforehand. If we fail the assertion here that indicates we have
    // a problem earlier in the workflow producing multiple (unexpected) remotes.
    Q_ASSERT(remotesCount <= 1);

    return static_cast<FlatpakTransactionThread *>(user_data)->choose_remote_for_ref(for_ref, runtime_ref, remotes, remotesCount);
}

void end_of_lifed([[maybe_unused]] FlatpakTransaction *transaction, const char *ref, const char *reason, const char *rebase)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << ref << reason << rebase;
}

gboolean ready([[maybe_unused]] FlatpakTransaction *transaction)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP";
    return true; // continue with transaction
}

gboolean run([[maybe_unused]] FlatpakTransaction *transaction, [[maybe_unused]] GCancellable *cancellable, [[maybe_unused]] GError **error)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP";
    return true; // continue with transaction
}

gboolean end_of_lifed_with_rebase([[maybe_unused]] FlatpakTransaction *transaction,
                                  const char *remote,
                                  const char *ref,
                                  const char *reason,
                                  const char *rebased_to_ref,
                                  const char **previous_ids,
                                  gpointer user_data)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "end_of_lifed_with_rebase" << remote << ref << reason << rebased_to_ref << previous_ids;
    return static_cast<FlatpakTransactionThread *>(user_data)->end_of_lifed_with_rebase(remote, ref, reason, rebased_to_ref, previous_ids);
}

gboolean basic_auth_start([[maybe_unused]] FlatpakTransaction *transaction,
                          const char *remote,
                          const char *realm,
                          [[maybe_unused]] GVariant *options,
                          [[maybe_unused]] guint id)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << remote << realm;
    return true; // continue with transaction
}

void install_authenticator([[maybe_unused]] FlatpakTransaction *transaction, const char *remote, const char *authenticator_ref)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << remote << authenticator_ref;
}

gboolean ready_pre_auth([[maybe_unused]] FlatpakTransaction *transaction)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP";
    return true; // continue with transaction
}
} // namespace Callbacks

FlatpakTransactionThread::FlatpakTransactionThread(Transaction::Role role, FlatpakInstallation *installation)
    : m_cancellable(g_cancellable_new())
    , m_role(role)
    , m_installation(installation)
{
}

bool FlatpakTransactionThread::setupTransaction()
{
    if (m_transaction) {
        g_object_unref(m_transaction);
        m_transaction = nullptr;
    }

    g_autoptr(GError) localError = nullptr;
    g_cancellable_reset(m_cancellable);
    m_transaction = flatpak_transaction_new_for_installation(m_installation, m_cancellable, &localError);
    if (localError) {
        m_initializationErrorMessage = QString::fromUtf8(localError->message);
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to create transaction" << m_initializationErrorMessage;
        return false;
    }

    g_signal_connect(m_transaction, "add-new-remote", G_CALLBACK(add_new_remote_cb), this);
    g_signal_connect(m_transaction, "new-operation", G_CALLBACK(new_operation_cb), this);
    g_signal_connect(m_transaction, "operation-error", G_CALLBACK(operation_error_cb), this);

    g_signal_connect(m_transaction, "basic-auth-start", G_CALLBACK(Callbacks::basic_auth_start), this);
    g_signal_connect(m_transaction, "choose-remote-for-ref", G_CALLBACK(Callbacks::choose_remote_for_ref), this);
    g_signal_connect(m_transaction, "end-of-lifed", G_CALLBACK(Callbacks::end_of_lifed), this);
    g_signal_connect(m_transaction, "end-of-lifed-with-rebase", G_CALLBACK(Callbacks::end_of_lifed_with_rebase), this);
    g_signal_connect(m_transaction, "install-authenticator", G_CALLBACK(Callbacks::install_authenticator), this);
    g_signal_connect(m_transaction, "operation-done", G_CALLBACK(Callbacks::operation_done), this);
    g_signal_connect(m_transaction, "ready", G_CALLBACK(Callbacks::ready), this);
    g_signal_connect(m_transaction, "ready-pre-auth", G_CALLBACK(Callbacks::ready_pre_auth), this);

    if (qEnvironmentVariableIntValue("DISCOVER_FLATPAK_WEBFLOW")) {
        g_signal_connect(m_transaction, "webflow-start", G_CALLBACK(webflowStart), this);
        g_signal_connect(m_transaction, "webflow-done", G_CALLBACK(webflowDoneCallback), this);
    }

    return true;
}

FlatpakTransactionThread::~FlatpakTransactionThread()
{
    g_object_unref(m_transaction);
    g_object_unref(m_cancellable);
}

void FlatpakTransactionThread::cancel()
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "cancelling transaction thread";
    QMutexLocker lock(&m_proceedMutex);
    m_proceed = false;
    m_proceedCondition.wakeAll();

    for (int id : std::as_const(m_webflows)) {
        flatpak_transaction_abort_webflow(m_transaction, id);
    }
    g_cancellable_cancel(m_cancellable);
}

void FlatpakTransactionThread::run()
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Running new transaction";
    // Make sure all transaction are finished when we return.
    // This in particular makes sure that if we abort half way through all transactions will correctly enter some state.
    auto finish = qScopeGuard([this] {
        finishAllJobTransactions();
    });

    if (!setupTransaction()) {
        return;
    }
    Q_ASSERT(m_transaction);

    for (const auto &[refName, jobTransaction] : m_jobTransactionsByRef.asKeyValueRange()) {
        g_autoptr(GError) localError = nullptr;

        auto app = jobTransaction->m_app;
        if (m_role == Transaction::Role::InstallRole) {
            bool correct = false;
            if (app->state() == AbstractResource::Upgradeable && app->isInstalled()) {
                correct = flatpak_transaction_add_update(m_transaction, refName.toUtf8().constData(), nullptr, nullptr, &localError);
                for (const QByteArray &subref : app->toUpdate()) {
                    correct = correct && flatpak_transaction_add_update(m_transaction, subref.constData(), nullptr, nullptr, &localError);
                }
            } else if (app->flatpakFileType() == FlatpakResource::FileFlatpak) {
                g_autoptr(GFile) file = g_file_new_for_path(app->resourceFile().toLocalFile().toUtf8().constData());
                if (!file) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to install bundled application" << refName;
                    m_operationSuccess = false;
                    return;
                }
                correct = flatpak_transaction_add_install_bundle(m_transaction, file, nullptr, &localError);
            } else if (app->flatpakFileType() == FlatpakResource::FileFlatpakRef && app->resourceFile().isLocalFile()) {
                g_autoptr(GFile) file = g_file_new_for_path(app->resourceFile().toLocalFile().toUtf8().constData());
                if (!file) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to install flatpakref application" << refName;
                    m_operationSuccess = false;
                    return;
                }
                g_autoptr(GBytes) bytes = g_file_load_bytes(file, m_cancellable, nullptr, &localError);
                correct = flatpak_transaction_add_install_flatpakref(m_transaction, bytes, &localError);
            } else {
                correct = flatpak_transaction_add_install(m_transaction, //
                                                        app->origin().toUtf8().constData(),
                                                        refName.toUtf8().constData(),
                                                        nullptr,
                                                        &localError);
            }

            if (!correct) {
                fail(qUtf8Printable(refName), localError);
                return;
            }
        } else if (m_role == Transaction::Role::RemoveRole) {
            if (!flatpak_transaction_add_uninstall(m_transaction, refName.toUtf8().constData(), &localError)) {
                m_operationSuccess = false;
                m_errorMessage = QString::fromUtf8(localError->message);
                // We are done so we can set the progress to 100
                setProgress(100);
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to uninstall" << refName << ':' << m_errorMessage;
                return;
            }
        }
    }

    g_autoptr(GError) localError = nullptr;
    m_operationSuccess = flatpak_transaction_run(m_transaction, m_cancellable, &localError);
    if (!m_operationSuccess) {
        m_errorMessage = QString::fromUtf8(localError->message);
#if defined(FLATPAK_LIST_UNUSED_REFS)
    } else {
        const auto installation = flatpak_transaction_get_installation(m_transaction);
        g_autoptr(GError) refsError = nullptr;
        g_autoptr(GPtrArray) refs = flatpak_installation_list_unused_refs(installation, nullptr, m_cancellable, &refsError);
        if (!refs) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not fetch unused refs" << refsError->message;
        } else if (refs->len > 0) {
            g_autoptr(GError) localError = nullptr;
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "found unused refs:" << refs->len;
            auto transaction = flatpak_transaction_new_for_installation(installation, m_cancellable, &localError);
            if (!transaction) {
                m_errorMessage = i18nc("@info:status",
                                       "Cannot create transaction for '%1':<nl/>%2<nl/><nl/>Please report this to <a href='%3'>%3</a>",
                                       QString::fromUtf8(flatpak_installation_get_display_name(installation)),
                                       QString::fromUtf8(localError->message),
                                       KOSRelease().bugReportUrl());
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not create transaction" << localError->message;
                return;
            }
            for (uint i = 0; i < refs->len; i++) {
                FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
                const gchar *strRef = flatpak_ref_format_ref_cached(ref);
                qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "unused ref:" << strRef;
                if (!flatpak_transaction_add_uninstall(transaction, strRef, &localError)) {
                    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed to uninstall unused ref" << strRef << localError->message;
                    break;
                }
            }
            if (!flatpak_transaction_run(transaction, m_cancellable, &localError)) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not properly clean the elements" << refs->len << localError->message;
            }
        }
#endif
    }
}

int FlatpakTransactionThread::progress() const
{
    return m_progress;
}

void FlatpakTransactionThread::setProgress(int progress)
{
    Q_ASSERT(qBound(0, progress, 100) == progress);
    if (m_progress > progress) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "progress is regressing :(" << m_progress << progress;
        return;
    }

    if (m_progress != progress) {
        m_progress = progress;
        Q_EMIT progressChanged(m_progress);
    }
}

void FlatpakTransactionThread::setSpeed(quint64 speed)
{
    if (m_speed != speed) {
        m_speed = speed;
        Q_EMIT speedChanged(m_speed);
    }
}

QString FlatpakTransactionThread::errorMessage() const
{
    QStringList messages;
    for (const auto &msg : {m_initializationErrorMessage, m_errorMessage}) {
        if (!msg.isEmpty()) {
            messages.push_back(msg);
        }
    }
    return messages.join('\n'_L1);
}

void FlatpakTransactionThread::addErrorMessage(const QString &error)
{
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.append(QLatin1Char('\n'));
    }
    m_errorMessage.append(error);
}

bool FlatpakTransactionThread::cancelled() const
{
    return g_cancellable_is_cancelled(m_cancellable);
}

FlatpakTransactionThread::Repositories FlatpakTransactionThread::addedRepositories() const
{
    return m_addedRepositories;
}

void FlatpakTransactionThread::fail(const char *refName, GError *error)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << Q_FUNC_INFO;
    m_operationSuccess = false;
    m_errorMessage = error
        ? QString::fromUtf8(error->message)
        : i18nc("fallback error message", "An internal error occurred. Please file a report at https://bugs.kde.org/enter_bug.cgi?product=Discover");
    // We are done so we can set the progress to 100
    setProgress(100);
    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to install" << refName << ':' << m_errorMessage;
}

bool FlatpakTransactionThread::end_of_lifed_with_rebase(const char *remote,
                                                        const char *ref,
                                                        const char *reason,
                                                        const char *rebased_to_ref,
                                                        const char **previous_ids)
{
    QMutexLocker lock(&m_proceedMutex);

    // Make sure we are connected to a fronting transaction.
    setCurrentRef(ref);

    enum class Execute { Rebase, Uninstall };
    auto target = Execute::Rebase;
    if (!rebased_to_ref) { // ref has no replacement -> uninstall
        target = Execute::Uninstall;
    }

    if (QString::fromUtf8(ref).startsWith("runtime/"_L1) || QString::fromUtf8(rebased_to_ref).startsWith("runtime/"_L1)) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Automatically transitioning runtime";
        m_proceed = true;

        switch (target) {
        case Execute::Rebase:
            break;
        case Execute::Uninstall:
            return false; // do not attempt to automatically remove runtimes, they may still be used and we may fail the transaction
        }
    } else {
        m_proceed = false;

        switch (target) {
        case Execute::Rebase:
            Q_EMIT proceedRequest(i18nc("@title", "Replacement Available"),
                                  xi18nc("@info %1 and 2 are flatpak ids e.g. org.kde.krita (can be rather lengthy though)",
                                         "<resource>%1</resource> is no longer receiving updates.<nl/><nl/>Replace it with the supported version provided by "
                                         "<resource>%2</resource>?",
                                         QString::fromUtf8(ref),
                                         QString::fromUtf8(rebased_to_ref)));
            break;
        case Execute::Uninstall:
            Q_EMIT proceedRequest(i18nc("@title", "Automatic Removal"),
                                  xi18nc("@info %1 is a flatpak id e.g. org.kde.krita (can be rather lengthy though)",
                                         "<resource>%1</resource> is no longer receiving updates.<nl/><p>The reason given is: "
                                         "<message>%2</message></p><nl/><p>Automatically remove it?</p>",
                                         QString::fromUtf8(ref),
                                         QString::fromUtf8(reason)));
            break;
        }

        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Waiting for proceed signal";
        m_proceedCondition.wait(&m_proceedMutex);
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Received proceed signal";
    }

    if (!m_proceed) {
        return false;
    }

    g_autoptr(GError) localError = nullptr;
    auto correct = false;
    switch (target) {
    case Execute::Rebase:
#if FLATPAK_CHECK_VERSION(1, 15, 0)
        correct = flatpak_transaction_add_rebase_and_uninstall(m_transaction, remote, rebased_to_ref, ref, nullptr, previous_ids, &localError);
        if (correct) {
            auto job = m_jobTransactionsByRef.value(QLatin1String(ref));
            m_jobTransactionsByRef.insert(QLatin1String(rebased_to_ref), job);
        }
#else
        correct = flatpak_transaction_add_rebase(m_transaction, remote, rebased_to_ref, nullptr, previous_ids, &localError)
            && flatpak_transaction_add_uninstall(m_transaction, ref, &localError);
#endif
        break;
    case Execute::Uninstall:
        correct = flatpak_transaction_add_uninstall(m_transaction, ref, &localError);
        break;
    }
    if (!correct || localError) {
        fail(ref, localError);
        return false;
    }
    return m_proceed;
}

void FlatpakTransactionThread::proceed()
{
    QMutexLocker lock(&m_proceedMutex);
    m_proceed = true;
    m_proceedCondition.wakeAll();
}

void FlatpakTransactionThread::addJobTransaction(FlatpakJobTransaction *jobTransaction)
{
    Q_ASSERT(jobTransaction);
    const QString ref = jobTransaction->m_app->ref();
    Q_ASSERT(!m_jobTransactionsByRef.contains(ref));
    m_jobTransactionsByRef.insert(ref, jobTransaction);
}

void FlatpakTransactionThread::setCurrentRef(const char *ref_cstr)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << Q_FUNC_INFO << ref_cstr;
    Q_ASSERT(ref_cstr);

    const auto optionalRef = refToAppRef(QString::fromUtf8(ref_cstr));
    if (!optionalRef.has_value()) {
        // Pretend we still have work to do on the previous ref.
        return;
    }
    const auto &ref = optionalRef.value();

    Q_ASSERT(m_jobTransactionsByRef.contains(ref));
    auto job = m_jobTransactionsByRef.value(ref);

    if (job == m_currentJobTransaction) {
        // Still on the same transaction, nothing to do
        return;
    }

    if (m_currentJobTransaction) { // Finalize the previous job
        setProgress(100);
        Q_EMIT finished(cancelled(), errorMessage(), addedRepositories(), success());
        disconnect(static_cast<QObject *>(m_currentJobTransaction));
    }

    m_errorMessage.clear();
    m_addedRepositories.clear();
    m_operationSuccess.reset();
    m_progress = 0;
    m_speed = 0;

    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Connecting to ref" << ref << job;
    m_currentJobTransaction = job;
    connect(this, &FlatpakTransactionThread::finished, job, &FlatpakJobTransaction::finishTransaction);
    connect(this, &FlatpakTransactionThread::progressChanged, job, &FlatpakJobTransaction::setProgress);
    connect(this, &FlatpakTransactionThread::speedChanged, job, &FlatpakJobTransaction::setDownloadSpeed);
    connect(this, &FlatpakTransactionThread::passiveMessage, job, &FlatpakJobTransaction::passiveMessage);
    connect(this, &FlatpakTransactionThread::webflowStarted, job, &FlatpakJobTransaction::webflowStarted);
    connect(this, &FlatpakTransactionThread::webflowDone, job, &FlatpakJobTransaction::webflowDone);
    connect(this, &FlatpakTransactionThread::proceedRequest, job, &FlatpakJobTransaction::proceedRequest);
    connect(this, &FlatpakTransactionThread::statusChanged, job, &FlatpakJobTransaction::setStatus);
}

void FlatpakTransactionThread::operationError(GError *error)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << Q_FUNC_INFO;
    m_operationSuccess = false;
    if (error) {
        addErrorMessage(QString::fromUtf8(error->message));
    }
}

bool FlatpakTransactionThread::success() const
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << Q_FUNC_INFO;
    // When the operation hasn't explicitly errored out we default to assume that it passed.
    return m_operationSuccess.value_or(true);
}

void FlatpakTransactionThread::finishAllJobTransactions()
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Finishing all transactions";
    for (const auto &jobTransaction : std::as_const(m_jobTransactionsByRef)) {
        // As jobs get finished they also get deleted. Don't trip over them!
        if (!jobTransaction) {
            continue;
        }

        // Invoke in correct thread obviously. Note that `this` may not be used in the queued lambda because it may be out of scope already!
        QMetaObject::invokeMethod(
            jobTransaction,
            [jobTransaction, cancelled = cancelled(), errorMessage = errorMessage(), addedRepositories = addedRepositories(), success = success()] {
                jobTransaction->finishTransaction(cancelled, errorMessage, addedRepositories, success);
            },
            Qt::QueuedConnection);
    }
    m_jobTransactionsByRef.clear();
}

int FlatpakTransactionThread::choose_remote_for_ref(const char *for_ref,
                                                    const char *runtime_ref,
                                                    [[maybe_unused]] const char *const *remotes,
                                                    [[maybe_unused]] unsigned int remotesCount)
{
    m_runtimeToAppRef.insert(QString::fromUtf8(runtime_ref), QString::fromUtf8(for_ref));
    return 0;
}

std::optional<QString> FlatpakTransactionThread::refToAppRef(const QString &ref) const
{
    const auto isRuntime = ref.startsWith("runtime/"_L1);
    const auto isExplicitRef = m_jobTransactionsByRef.contains(ref);
    if (!isRuntime || isExplicitRef) {
        return ref;
    }

    if (auto appRef = m_runtimeToAppRef.value(ref); !appRef.isEmpty()) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Runtime" << ref << "is associated with app" << appRef;
        return appRef;
    }

    return {};
}

#include "moc_FlatpakTransactionThread.cpp"
