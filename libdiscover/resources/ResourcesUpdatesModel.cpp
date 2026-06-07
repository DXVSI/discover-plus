/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ResourcesUpdatesModel.h"
#include "AbstractBackendUpdater.h"
#include "AbstractResource.h"
#include "ResourcesModel.h"
#include "libdiscover_debug.h"
#include "utils.h"
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>

#include <KConfigGroup>
#include <KConfigWatcher>
#include <KFormat>
#include <KLocalizedString>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

class UpdateTransaction : public Transaction
{
    Q_OBJECT
public:
    UpdateTransaction(ResourcesUpdatesModel * /*parent*/, const QVector<AbstractBackendUpdater *> &updaters)
        : Transaction(nullptr, nullptr, Transaction::InstallRole)
        , m_allUpdaters(updaters)
    {
        setVisible(false);
        bool cancelable = false;
        for (auto updater : std::as_const(m_allUpdaters)) {
            connect(updater, &AbstractBackendUpdater::progressingChanged, this, &UpdateTransaction::slotProgressingChanged);
            connect(updater, &AbstractBackendUpdater::downloadSpeedChanged, this, &UpdateTransaction::slotDownloadSpeedChanged);
            connect(updater, &AbstractBackendUpdater::progressChanged, this, &UpdateTransaction::slotUpdateProgress);
            connect(updater, &AbstractBackendUpdater::proceedRequest, this, &UpdateTransaction::processProceedRequest);
            connect(updater, &AbstractBackendUpdater::distroErrorMessage, this, &UpdateTransaction::distroErrorMessage);
            connect(updater, &AbstractBackendUpdater::cancelableChanged, this, [this](bool) {
                setCancellable(kContains(m_allUpdaters, [](AbstractBackendUpdater *updater) {
                    return updater->isCancelable() && updater->isProgressing();
                }));
            });
            cancelable |= updater->isCancelable();
        }
        setCancellable(cancelable);
    }

    void processProceedRequest(const QString &title, const QString &message)
    {
        m_updatersWaitingForFeedback += qobject_cast<AbstractBackendUpdater *>(sender());
        Q_EMIT proceedRequest(title, message);
    }

    void cancel() override
    {
        const QVector<AbstractBackendUpdater *> toCancel = m_updatersWaitingForFeedback.isEmpty() ? m_allUpdaters : m_updatersWaitingForFeedback;

        for (auto updater : toCancel) {
            updater->cancel();
        }
    }

    void proceed() override
    {
        m_updatersWaitingForFeedback.takeFirst()->proceed();
    }

    bool isProgressing() const
    {
        bool progressing = false;
        for (AbstractBackendUpdater *upd : std::as_const(m_allUpdaters)) {
            progressing |= upd->isProgressing();
        }
        return progressing;
    }

    void slotProgressingChanged()
    {
        if (status() > SetupStatus && status() < DoneStatus && !isProgressing()) {
            setStatus(Transaction::DoneStatus);
            Q_EMIT finished();
            deleteLater();
        }
    }

    void slotUpdateProgress()
    {
        qreal total = 0;
        for (AbstractBackendUpdater *updater : std::as_const(m_allUpdaters)) {
            total += updater->progress();
        }
        setProgress(total / m_allUpdaters.count());
    }

    void slotDownloadSpeedChanged()
    {
        quint64 total = 0;
        for (AbstractBackendUpdater *updater : std::as_const(m_allUpdaters)) {
            total += updater->downloadSpeed();
        }
        setDownloadSpeed(total);
    }

    QVariant icon() const override
    {
        return QStringLiteral("update-low");
    }
    QString name() const override
    {
        return i18n("Updates");
    }

Q_SIGNALS:
    void finished();

private:
    QVector<AbstractBackendUpdater *> m_updatersWaitingForFeedback;
    const QVector<AbstractBackendUpdater *> m_allUpdaters;
};

ResourcesUpdatesModel::ResourcesUpdatesModel(QObject *parent)
    : QStandardItemModel(parent)
    , m_lastIsProgressing(false)
    , m_transaction(nullptr)
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesUpdatesModel::init);

    init();
}

void ResourcesUpdatesModel::init()
{
    const QVector<AbstractResourcesBackend *> backends = ResourcesModel::global()->backends();
    m_lastIsProgressing = false;
    for (AbstractResourcesBackend *b : backends) {
        AbstractBackendUpdater *updater = b->backendUpdater();
        if (updater && !m_updaters.contains(updater)) {
            connect(updater, &AbstractBackendUpdater::statusMessageChanged, this, &ResourcesUpdatesModel::message);
            connect(updater, &AbstractBackendUpdater::statusDetailChanged, this, &ResourcesUpdatesModel::message);
            connect(updater, &AbstractBackendUpdater::downloadSpeedChanged, this, &ResourcesUpdatesModel::downloadSpeedChanged);
            connect(updater, &AbstractBackendUpdater::resourceProgressed, this, &ResourcesUpdatesModel::resourceProgressed);
            connect(updater, &AbstractBackendUpdater::passiveMessage, this, &ResourcesUpdatesModel::passiveMessage);
            connect(updater, &AbstractBackendUpdater::needsRebootChanged, this, &ResourcesUpdatesModel::needsRebootChanged);
            connect(updater, &AbstractBackendUpdater::destroyed, this, &ResourcesUpdatesModel::updaterDestroyed);
            connect(updater, &AbstractBackendUpdater::errorMessageChanged, this, &ResourcesUpdatesModel::errorMessagesChanged);
            connect(updater, &AbstractBackendUpdater::fetchingChanged, this, &ResourcesUpdatesModel::refreshFetching);
            m_updaters += updater;

            m_lastIsProgressing |= updater->isProgressing();
        }
    }
    refreshFetching();

    // To enable from command line use:
    // kwriteconfig6 --file discoverrc --group Software --key UseOfflineUpdates true
    auto sharedConfig = KSharedConfig::openConfig();
    KConfigGroup group(sharedConfig, u"Software"_s);
    m_offlineUpdates = group.readEntry<bool>("UseOfflineUpdates", false);

    KConfigWatcher::Ptr watcher = KConfigWatcher::create(sharedConfig);
    connect(watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        // Ensure it is for the right file
        if (!names.contains("UseOfflineUpdates") || group.name() != QLatin1String("Software")) {
            return;
        }

        const bool offlineUpdates = group.readEntry<bool>("UseOfflineUpdates", false);
        if (m_offlineUpdates == offlineUpdates) {
            return;
        }
        m_offlineUpdates = offlineUpdates;
        Q_EMIT useUnattendedUpdatesChanged();
    });

    auto tm = TransactionModel::global();
    const auto transactions = tm->transactions();
    for (auto t : transactions) {
        auto updateTransaction = qobject_cast<UpdateTransaction *>(t);
        if (updateTransaction) {
            setTransaction(updateTransaction);
        }
    }

    Q_EMIT errorMessagesChanged();
}

void ResourcesUpdatesModel::updaterDestroyed(QObject *obj)
{
    for (auto it = m_updaters.begin(); it != m_updaters.end();) {
        if (*it == obj)
            it = m_updaters.erase(it);
        else
            ++it;
    }
}

void ResourcesUpdatesModel::message(const QString &msg)
{
    if (msg.isEmpty())
        return;

    appendRow(new QStandardItem(msg));
}

void ResourcesUpdatesModel::prepare()
{
    if (isProgressing()) {
        qCWarning(LIBDISCOVER_LOG) << "trying to set up a running instance";
        return;
    }

    for (AbstractBackendUpdater *upd : std::as_const(m_updaters)) {
        upd->setOfflineUpdates(m_offlineUpdates);
        upd->prepare();
    }
}

void ResourcesUpdatesModel::updateAll()
{
    if (!m_updaters.isEmpty()) {
        delete m_transaction;

        const auto updaters = kFilter<QVector<AbstractBackendUpdater *>>(m_updaters, [](AbstractBackendUpdater *u) {
            return u->hasUpdates();
        });
        if (updaters.isEmpty()) {
            qCWarning(LIBDISCOVER_LOG) << "Nothing to update";
            return;
        }

        auto transaction = new UpdateTransaction(this, updaters);
        transaction->setStatus(Transaction::SetupStatus);
        setTransaction(transaction);
        TransactionModel::global()->addTransaction(m_transaction);
        for (AbstractBackendUpdater *upd : updaters) {
            QMetaObject::invokeMethod(upd, &AbstractBackendUpdater::start, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(
            this,
            [this]() {
                m_transaction->setStatus(Transaction::CommittingStatus);
                m_transaction->slotProgressingChanged();
            },
            Qt::QueuedConnection);
    } else {
        qCWarning(LIBDISCOVER_LOG) << "No updaters!";
    }
}

bool ResourcesUpdatesModel::isProgressing() const
{
    return m_transaction && m_transaction->status() < Transaction::DoneStatus;
}

QList<AbstractResource *> ResourcesUpdatesModel::toUpdate() const
{
    QList<AbstractResource *> ret;
    for (AbstractBackendUpdater *upd : std::as_const(m_updaters)) {
        ret += upd->toUpdate();
    }
    return ret;
}

void ResourcesUpdatesModel::addResources(const QList<AbstractResource *> &resources)
{
    QHash<AbstractResourcesBackend *, QList<AbstractResource *>> sortedResources;
    for (AbstractResource *res : resources) {
        sortedResources[res->backend()] += res;
    }

    for (auto it = sortedResources.constBegin(), itEnd = sortedResources.constEnd(); it != itEnd; ++it) {
        it.key()->backendUpdater()->addResources(*it);
    }
}

void ResourcesUpdatesModel::removeResources(const QList<AbstractResource *> &resources)
{
    QHash<AbstractResourcesBackend *, QList<AbstractResource *>> sortedResources;
    for (AbstractResource *res : resources) {
        sortedResources[res->backend()] += res;
    }

    for (auto it = sortedResources.constBegin(), itEnd = sortedResources.constEnd(); it != itEnd; ++it) {
        it.key()->backendUpdater()->removeResources(*it);
    }
}

QDateTime ResourcesUpdatesModel::lastUpdate() const
{
    QDateTime ret;
    for (AbstractBackendUpdater *upd : std::as_const(m_updaters)) {
        QDateTime current = upd->lastUpdate();
        if (!ret.isValid() || (current.isValid() && current > ret)) {
            ret = current;
        }
    }
    return ret;
}

double ResourcesUpdatesModel::updateSize() const
{
    double ret = 0.;
    for (AbstractBackendUpdater *upd : m_updaters) {
        ret += std::max(0., upd->updateSize());
    }
    return ret;
}

qint64 ResourcesUpdatesModel::secsToLastUpdate() const
{
    return lastUpdate().secsTo(QDateTime::currentDateTime());
}

void ResourcesUpdatesModel::setTransaction(UpdateTransaction *transaction)
{
    if (m_transaction == transaction) {
        return;
    }

    if (m_transaction) {
        disconnect(m_transaction, &UpdateTransaction::finished, this, &ResourcesUpdatesModel::finished);
        disconnect(m_transaction, &UpdateTransaction::finished, this, &ResourcesUpdatesModel::progressingChanged);
    }
    m_transaction = transaction;
    connect(transaction, &UpdateTransaction::finished, this, &ResourcesUpdatesModel::finished);
    connect(transaction, &UpdateTransaction::finished, this, &ResourcesUpdatesModel::progressingChanged);

    Q_EMIT progressingChanged();
}

Transaction *ResourcesUpdatesModel::transaction() const
{
    return m_transaction.data();
}

bool ResourcesUpdatesModel::needsReboot() const
{
    for (auto upd : m_updaters) {
        if (upd->needsReboot())
            return true;
    }
    return false;
}

void ResourcesUpdatesModel::refreshFetching()
{
    bool fetching = false;
    for (auto updater : std::as_const(m_updaters)) {
        if (updater->isFetchingUpdates()) {
            fetching = true;
            break;
        }
    }
    if (fetching == m_fetching) {
        return;
    }
    m_fetching = fetching;
    Q_EMIT fetchingChanged();
}

bool ResourcesUpdatesModel::isFetching() const
{
    return m_fetching;
}

bool ResourcesUpdatesModel::readyToReboot() const
{
    return kContains(m_updaters, [](AbstractBackendUpdater *updater) {
        return !updater->needsReboot() || updater->isReadyToReboot();
    });
}

bool ResourcesUpdatesModel::useUnattendedUpdates() const
{
    return m_offlineUpdates;
}

QStringList ResourcesUpdatesModel::errorMessages() const
{
    QStringList ret;
    for (auto updater : m_updaters) {
        const auto error = updater->errorMessage();
        if (!error.isEmpty()) {
            ret << error;
        }
    }
    ret.removeDuplicates();
    return ret;
}

#include "ResourcesUpdatesModel.moc"
#include "moc_ResourcesUpdatesModel.cpp"
