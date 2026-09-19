/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "Transaction.h"

#include "TransactionModel.h"
#include "libdiscover_debug.h"
#include <KFormat>
#include <KLocalizedString>
#include <resources/AbstractResource.h>

Transaction::Transaction(QObject *parent, AbstractResource *resource, Role role, const AddonList &addons, bool visible)
    : QObject(parent)
    , m_resource(resource)
    , m_role(role)
    , m_status(CommittingStatus)
    , m_addons(addons)
    , m_isCancellable(true)
    , m_progress(0)
    , m_visible(visible)
{
}

Transaction::~Transaction()
{
    if (status() < DoneStatus || TransactionModel::global()->contains(this)) {
        qCWarning(LIBDISCOVER_LOG) << "destroying Transaction before it's over" << this;
        TransactionModel::global()->removeTransaction(this);
    }
}

AbstractResource *Transaction::resource() const
{
    return m_resource;
}

Transaction::Role Transaction::role() const
{
    return m_role;
}

Transaction::Status Transaction::status() const
{
    return m_status;
}

AddonList Transaction::addons() const
{
    return m_addons;
}

bool Transaction::isCancellable() const
{
    return m_isCancellable;
}

int Transaction::progress() const
{
    return m_progress;
}

void Transaction::setStatus(Status status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT statusChanged(m_status);

        if (m_status == DoneStatus || m_status == CancelledStatus || m_status == DoneWithErrorStatus) {
            setCancellable(false);

            TransactionModel::global()->removeTransaction(this);
        }
    }
}

void Transaction::setCancellable(bool isCancellable)
{
    if (m_isCancellable != isCancellable) {
        m_isCancellable = isCancellable;
        Q_EMIT cancellableChanged(m_isCancellable);
    }
}

void Transaction::setProgress(int progress)
{
    if (m_progress != progress) {
        Q_ASSERT(qBound(0, progress, 100) == progress);
        m_progress = qBound(0, progress, 100);
        Q_EMIT progressChanged(m_progress);
    }
}

bool Transaction::isActive() const
{
    return m_status == DownloadingStatus || m_status == CommittingStatus;
}

QString Transaction::name() const
{
    return m_resource->name();
}

QVariant Transaction::icon() const
{
    return m_resource->icon();
}

bool Transaction::isVisible() const
{
    return m_visible;
}

void Transaction::setDownloadSpeed(quint64 downloadSpeed)
{
    if (downloadSpeed != m_downloadSpeed) {
        m_downloadSpeed = downloadSpeed;
        Q_EMIT downloadSpeedChanged(downloadSpeed);
    }
}

QString Transaction::downloadSpeedString() const
{
    return i18nc("@label Download rate", "%1/s", KFormat().formatByteSize(downloadSpeed()));
}

void Transaction::setRemainingTime(uint remainingTime)
{
    if (remainingTime != m_remainingTime) {
        m_remainingTime = remainingTime;
        Q_EMIT remainingTimeChanged(remainingTime);
    }
}

QString Transaction::remainingTimeString() const
{
    return KFormat().formatSpelloutDuration(m_remainingTime * 1000);
}

#include "moc_Transaction.cpp"
