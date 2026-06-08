/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SnapTransaction.h"
#include "SnapBackend.h"
#include "SnapResource.h"
#include "libsnapclient/config-paths.h"
#include <KLocalizedString>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>

SnapTransaction::SnapTransaction(QSnapdClient *client, SnapResource *app, Role role, AbstractResource::State newState)
    : Transaction(app, app, role)
    , m_client(client)
    , m_app(app)
    , m_newState(newState)
{
    if (role == RemoveRole)
        setRequest(m_client->remove(app->packageName()));
    else if (app->state() == AbstractResource::Upgradeable)
        setRequest(m_client->refresh(app->packageName(), app->m_channel));
    else
        setRequest(m_client->install(app->packageName(), app->m_channel));
}

void SnapTransaction::cancel()
{
    m_request->cancel();
    if (m_request->error() != QSnapdRequest::NoError) {
        Q_EMIT passiveMessage(m_request->errorString());
    }
    setStatus(CancelledStatus);
}

void SnapTransaction::finishTransaction()
{
    switch (m_request->error()) {
    case QSnapdRequest::NoError:
        static_cast<SnapBackend *>(m_app->backend())->refreshStates();
        setStatus(DoneStatus);
        m_app->setState(m_newState);
        break;
    case QSnapdRequest::Cancelled:
        setStatus(CancelledStatus);
        break;
    case QSnapdRequest::AuthDataRequired:
        setStatus(CancelledStatus);
        break;
    case QSnapdRequest::NeedsClassic:
        setStatus(SetupStatus);
        if (role() == Transaction::InstallRole) {
            Q_EMIT proceedRequest(m_app->name(),
                                  i18n("This Snap application is not compatible with security sandboxing "
                                       "and will have full access to this system. Install it anyway?"));
            return;
        }
        break;
    default:
        qDebug() << "snap error" << m_request.get() << m_request->error() << m_request->errorString();
        Q_EMIT passiveMessage(m_request->errorString());
        setStatus(DoneWithErrorStatus);
        break;
    }
}

void SnapTransaction::proceed()
{
    setRequest(m_client->install(QSnapdClient::Classic, m_app->packageName()));
}

void SnapTransaction::setRequest(QSnapdRequest *req)
{
    m_request.reset(req);

    setCancellable(true);
    connect(m_request.data(), &QSnapdRequest::progress, this, &SnapTransaction::progressed);
    connect(m_request.data(), &QSnapdRequest::cancel, this, &SnapTransaction::finishTransaction);
    connect(m_request.data(), &QSnapdRequest::complete, this, &SnapTransaction::finishTransaction);

    setStatus(CommittingStatus);
    m_request->runAsync();
}

void SnapTransaction::progressed()
{
    Status status = SetupStatus;
    const auto change = m_request->change();
    int percentage = 0, downloadTaskPos = 0;
    qint64 totalProgressDone = 0, progressTotal = 0;

    for (int i = 0, c = change->taskCount(); i < c; ++i) {
        auto task = change->task(i);
        if (task->kind() == QLatin1String("download-snap")) {
            downloadTaskPos = i;
        } else if ((task->status() != QLatin1String("Doing") || task->status() == QLatin1String("Do")) && task->kind() != QLatin1String("download-snap")) {
            status = CommittingStatus;
        }
        if (!task->progressLabel().isEmpty()) {
            totalProgressDone += task->progressDone();
            progressTotal += task->progressTotal();
        }
    }
    percentage = (progressTotal > 0) ? (100 * totalProgressDone / progressTotal) : 0;
    setProgress(percentage);
    QSnapdTask *downloadTask = change->task(downloadTaskPos);
    if (downloadTask->status() == QLatin1String("Doing")) {
        status = DownloadingStatus;
        setSpeed(downloadTask);
    }
    setStatus(status);
}

void SnapTransaction::setSpeed(QSnapdTask *downloadTask)
{
    auto totalDownloadDone = downloadTask->progressDone();
    if (!m_downloadTimer.isValid()) {
        m_downloadTimer.start();
        m_lastProgressDone = totalDownloadDone;
    } else {
        qint64 elapsed = m_downloadTimer.elapsed();
        if (elapsed > 0) {
            qint64 deltaProgress = totalDownloadDone - m_lastProgressDone;
            qint64 speed = (deltaProgress * 1000) / elapsed;
            setDownloadSpeed(speed);

            m_downloadTimer.restart();
            m_lastProgressDone = totalDownloadDone;
        }
    }
}

#include "moc_SnapTransaction.cpp"
