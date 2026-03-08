/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ResourcesModel.h"
#include "libdiscover_debug.h"
#include "utils.h"
#include <KLocalizedString>
#include <QIcon>
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/StandardBackendUpdater.h>

StandardBackendUpdater::StandardBackendUpdater(AbstractResourcesBackend *parent)
    : AbstractBackendUpdater(parent)
    , m_backend(parent)
    , m_progress(0)
    , m_lastUpdate(QDateTime())
{
    connect(m_backend, &AbstractResourcesBackend::contentsChanged, this, &StandardBackendUpdater::refreshUpdateable);
    connect(m_backend, &AbstractResourcesBackend::invalidated, this, &StandardBackendUpdater::refreshUpdateable);
    connect(m_backend, &AbstractResourcesBackend::resourcesChanged, this, &StandardBackendUpdater::resourcesChanged);
    connect(m_backend, &AbstractResourcesBackend::fetchingUpdatesProgressChanged, this, &StandardBackendUpdater::fetchingChanged);
    connect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, [this](AbstractResource *resource) {
        if (m_upgradeable.remove(resource)) {
            Q_EMIT updatesCountChanged(updatesCount());
        }
        m_toUpgrade.remove(resource);
    });
    connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &StandardBackendUpdater::transactionRemoved);
    connect(TransactionModel::global(), &TransactionModel::transactionAdded, this, &StandardBackendUpdater::transactionAdded);

    m_timer.setSingleShot(true);
    m_timer.setInterval(10);
    connect(&m_timer, &QTimer::timeout, this, &StandardBackendUpdater::refreshUpdateable);
}

void StandardBackendUpdater::resourcesChanged(AbstractResource *res, const QVector<QByteArray> &props)
{
    if (!m_settingUp && props.contains("state") && (res->state() == AbstractResource::Upgradeable || m_upgradeable.contains(res))) {
        m_timer.start();
    }
}

bool StandardBackendUpdater::hasUpdates() const
{
    return !m_upgradeable.isEmpty();
}

void StandardBackendUpdater::start()
{
    setSettingUp(true);
    Q_EMIT progressingChanged(true);
    setProgress(0);
    auto upgradeList = m_toUpgrade.values();
    std::sort(upgradeList.begin(), upgradeList.end(), [](const AbstractResource *a, const AbstractResource *b) {
        return a->name() < b->name();
    });

    const bool couldCancel = m_canCancel;
    for (AbstractResource *res : std::as_const(upgradeList)) {
        m_pendingResources += res;
        auto t = m_backend->installApplication(res);
        t->setProperty("updater", QVariant::fromValue<QObject *>(this));
        connect(t, &Transaction::downloadSpeedChanged, this, [this]() {
            Q_EMIT downloadSpeedChanged(downloadSpeed());
        });
        connect(t, &Transaction::cancellableChanged, this, [this, t]() {
            if (!m_canCancel && t->isCancellable()) {
                m_canCancel = true;
                Q_EMIT cancelableChanged(m_canCancel);
            }
        });
        connect(this, &StandardBackendUpdater::cancelTransaction, t, &Transaction::cancel);
        TransactionModel::global()->addTransaction(t);
        m_canCancel |= t->isCancellable();
    }
    if (m_canCancel != couldCancel) {
        Q_EMIT cancelableChanged(m_canCancel);
    }
    setSettingUp(false);

    if (m_pendingResources.isEmpty()) {
        cleanup();
    } else {
        setProgress(1);
    }
}

void StandardBackendUpdater::cancel()
{
    Q_EMIT cancelTransaction();
}

void StandardBackendUpdater::transactionAdded(Transaction *newTransaction)
{
    if (!m_pendingResources.contains(newTransaction->resource()))
        return;

    connect(newTransaction, &Transaction::progressChanged, this, &StandardBackendUpdater::transactionProgressChanged);
    connect(newTransaction, &Transaction::statusChanged, this, &StandardBackendUpdater::transactionProgressChanged);
}

AbstractBackendUpdater::State toUpdateState(Transaction *t)
{
    switch (t->status()) {
    case Transaction::SetupStatus:
    case Transaction::QueuedStatus:
    case Transaction::CancelledStatus:
        return AbstractBackendUpdater::None;
    case Transaction::DownloadingStatus:
        return AbstractBackendUpdater::Downloading;
    case Transaction::CommittingStatus:
        return AbstractBackendUpdater::Installing;
    case Transaction::DoneStatus:
    case Transaction::DoneWithErrorStatus:
        return AbstractBackendUpdater::Done;
    }
    Q_UNREACHABLE();
}

bool StandardBackendUpdater::isFetchingUpdates() const
{
    return m_backend->fetchingUpdatesProgress() != 100 || m_settingUp;
}

void StandardBackendUpdater::transactionProgressChanged()
{
    Transaction *t = qobject_cast<Transaction *>(sender());
    Q_EMIT resourceProgressed(t->resource(), t->progress(), toUpdateState(t));

    refreshProgress();
}

void StandardBackendUpdater::transactionRemoved(Transaction *t)
{
    const bool fromOurBackend = t->resource() && t->resource()->backend() == m_backend;
    if (!fromOurBackend) {
        return;
    }

    const bool found = fromOurBackend && m_pendingResources.remove(t->resource());
    m_anyTransactionFailed |= t->status() != Transaction::DoneStatus;

    if (found && !m_settingUp) {
        refreshProgress();
        if (m_pendingResources.isEmpty()) {
            cleanup();
            if (needsReboot() && !m_anyTransactionFailed) {
                enableReadyToReboot();
            }
        }
    }
}

void StandardBackendUpdater::refreshProgress()
{
    if (m_toUpgrade.isEmpty()) {
        return;
    }

    int allProgresses = (m_toUpgrade.size() - m_pendingResources.size()) * 100;
    const auto allTransactions = transactions();
    for (auto t : allTransactions) {
        allProgresses += t->progress();
    }
    setProgress(allProgresses / m_toUpgrade.size());
}

void StandardBackendUpdater::refreshUpdateable()
{
    if (!m_backend->isValid()) {
        qWarning() << "Invalidated backend, deactivating" << m_backend->name();
        if (m_settingUp) {
            setSettingUp(false);
            Q_EMIT progressingChanged(isProgressing());
        }
        return;
    }

    if (isProgressing()) {
        m_timer.start(1000);
        return;
    }

    m_timer.stop();
    setSettingUp(true);
    Q_EMIT progressingChanged(true);
    Q_EMIT fetchingChanged();
    AbstractResourcesBackend::Filters f;
    f.state = AbstractResource::Upgradeable;
    m_upgradeable.clear();
    auto r = m_backend->search(f);
    connect(r, &ResultsStream::resourcesFound, this, [this](const QVector<StreamResult> &resources) {
        const auto predicate = [](const StreamResult &result) -> bool {
            return result.resource->state() == AbstractResource::Upgradeable;
        };
        const auto count = std::count_if(resources.constBegin(), resources.constEnd(), predicate);
        m_upgradeable.reserve(m_upgradeable.count() + count);
        for (auto result : resources) {
            if (predicate(result)) {
                m_upgradeable.insert(result.resource);
            }
        }
    });
    connect(r, &ResultsStream::destroyed, this, [this]() {
        setSettingUp(false);
        Q_EMIT updatesCountChanged(updatesCount());
        Q_EMIT progressingChanged(false);
        Q_EMIT fetchingChanged();
    });
}

qreal StandardBackendUpdater::progress() const
{
    return m_progress;
}

void StandardBackendUpdater::setProgress(qreal p)
{
    if (p > m_progress || p < 0) {
        m_progress = p;
        Q_EMIT progressChanged(p);
    }
}

void StandardBackendUpdater::prepare()
{
    m_lastUpdate = QDateTime::currentDateTime();
    m_toUpgrade = m_upgradeable;
}

int StandardBackendUpdater::updatesCount() const
{
    return m_upgradeable.count();
}

void StandardBackendUpdater::addResources(const QList<AbstractResource *> &apps)
{
    const QSet<AbstractResource *> upgradeableApps = kToSet(apps);
    Q_ASSERT(m_upgradeable.contains(upgradeableApps));
    m_toUpgrade += upgradeableApps;
}

void StandardBackendUpdater::removeResources(const QList<AbstractResource *> &apps)
{
    const QSet<AbstractResource *> upgradeableApps = kToSet(apps);
    Q_ASSERT(m_upgradeable.contains(upgradeableApps));
    Q_ASSERT(m_toUpgrade.contains(upgradeableApps));
    m_toUpgrade -= upgradeableApps;
}

void StandardBackendUpdater::cleanup()
{
    m_lastUpdate = QDateTime::currentDateTime();
    m_toUpgrade.clear();
    Q_EMIT progressingChanged(false);

    refreshUpdateable();
}

QList<AbstractResource *> StandardBackendUpdater::toUpdate() const
{
    return m_toUpgrade.values();
}

bool StandardBackendUpdater::isMarked(AbstractResource *res) const
{
    return m_toUpgrade.contains(res);
}

QDateTime StandardBackendUpdater::lastUpdate() const
{
    return m_lastUpdate;
}

bool StandardBackendUpdater::isCancelable() const
{
    return m_canCancel;
}

bool StandardBackendUpdater::isProgressing() const
{
    return m_settingUp || !m_pendingResources.isEmpty();
}

double StandardBackendUpdater::updateSize() const
{
    double ret = 0.;
    for (AbstractResource *res : m_toUpgrade) {
        ret += res->size();
    }
    return ret;
}

QVector<Transaction *> StandardBackendUpdater::transactions() const
{
    const auto trans = TransactionModel::global()->transactions();
    return kFilter<QVector<Transaction *>>(trans, [this](Transaction *t) {
        return t->property("updater").value<QObject *>() == this;
    });
}

quint64 StandardBackendUpdater::downloadSpeed() const
{
    quint64 ret = 0;
    const auto trans = transactions();
    for (Transaction *t : trans) {
        ret += t->downloadSpeed();
    }
    return ret;
}

void StandardBackendUpdater::setSettingUp(bool settingUp)
{
    if (m_settingUp == settingUp) {
        return;
    }
    m_settingUp = settingUp;
    Q_EMIT settingUpChanged();
}

#include "moc_StandardBackendUpdater.cpp"
