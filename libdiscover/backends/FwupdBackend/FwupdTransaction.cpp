/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FwupdTransaction.h"

#include <QTimer>
#include <resources/AbstractBackendUpdater.h>

FwupdTransaction::FwupdTransaction(FwupdResource *app, FwupdBackend *backend)
    : Transaction(backend, app, Transaction::InstallRole, {})
    , m_app(app)
    , m_backend(backend)
    , m_cancellable(g_cancellable_new())
{
    setCancellable(true);
    setStatus(QueuedStatus);

    Q_ASSERT(!m_app->deviceId().isEmpty());
    QTimer::singleShot(0, this, &FwupdTransaction::install);
}

FwupdTransaction::~FwupdTransaction()
{
    g_object_unref(m_cancellable);
}

void FwupdTransaction::install()
{
    g_autoptr(GError) error = nullptr;

    if (m_app->isDeviceLocked()) {
        QString device_id = m_app->deviceId();
        if (device_id.isEmpty()) {
            qWarning() << "Fwupd Error: No Device ID set, cannot unlock device " << this << m_app->name();
        } else if (!fwupd_client_unlock(m_backend->client, device_id.toUtf8().constData(), m_cancellable, &error)) {
            m_backend->handleError(error);
        }
        setStatus(DoneWithErrorStatus);
        return;
    }

    const QString fileName = m_app->cacheFile();
    if (!QFileInfo::exists(fileName)) {
        const QUrl uri(m_app->updateURI());
        setStatus(DownloadingStatus);
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        auto req = QNetworkRequest(uri);

        const QString userAgent = QString::fromUtf8(fwupd_client_get_user_agent(m_backend->client));
        req.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        auto reply = manager->get(req);
        QFile *file = new QFile(fileName);
        if (!file->open(QFile::WriteOnly)) {
            qWarning() << "Fwupd Error: Could not open to write" << fileName << uri;
            setStatus(DoneWithErrorStatus);
            file->deleteLater();
            return;
        }

        connect(reply, &QNetworkReply::finished, this, [this, file, reply]() {
            file->close();
            file->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                qWarning() << "Fwupd Error: Could not download" << reply->url() << reply->errorString();
                file->remove();
                setStatus(DoneWithErrorStatus);
            } else {
                fwupdInstall(file->fileName());
            }
        });
        connect(reply, &QNetworkReply::readyRead, this, [file, reply]() {
            file->write(reply->readAll());
        });
    } else {
        fwupdInstall(fileName);
    }
}

void FwupdTransaction::fwupdInstall(const QString &file)
{
    FwupdInstallFlags install_flags = FWUPD_INSTALL_FLAG_NONE;
    g_autoptr(GError) error = nullptr;

    if (!fwupd_client_install(m_backend->client, m_app->deviceId().toUtf8().constData(), file.toUtf8().constData(), install_flags, m_cancellable, &error)) {
        m_backend->handleError(error);
        setStatus(DoneWithErrorStatus);
    } else
        finishTransaction();
}

void FwupdTransaction::updateProgress()
{
    setProgress(fwupd_client_get_percentage(m_backend->client));
}

void FwupdTransaction::proceed()
{
    finishTransaction();
}

void FwupdTransaction::cancel()
{
    g_cancellable_cancel(m_cancellable);
    setStatus(CancelledStatus);
}

void FwupdTransaction::finishTransaction()
{
    if (g_cancellable_is_cancelled(m_cancellable)) {
        return;
    }

    AbstractResource::State newState = AbstractResource::None;
    switch (role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setState(newState);
    if (m_app->needsReboot()) {
        m_app->backend()->backendUpdater()->setNeedsReboot(true);
    }
    setStatus(DoneStatus);
}

#include "moc_FwupdTransaction.cpp"
