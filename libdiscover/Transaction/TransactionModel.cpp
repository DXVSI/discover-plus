/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "TransactionModel.h"

// Qt includes
#include <KLocalizedString>
#include <QDebug>
#include <QMetaProperty>

// Own includes
#include "libdiscover_debug.h"
#include "resources/AbstractResource.h"

Q_GLOBAL_STATIC(TransactionModel, globalTransactionModel)

TransactionModel *TransactionModel::global()
{
    return globalTransactionModel;
}

TransactionModel::TransactionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &TransactionModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &TransactionModel::countChanged);
    connect(this, &TransactionModel::countChanged, this, &TransactionModel::progressChanged);
}

QHash<int, QByteArray> TransactionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TransactionRoleRole] = "transactionRole";
    roles[TransactionStatusRole] = "status";
    roles[CancellableRole] = "cancellable";
    roles[ProgressRole] = "progress";
    roles[StatusTextRole] = "statusText";
    roles[ResourceRole] = "resource";
    roles[TransactionRole] = "transaction";
    roles[VisibleRole] = "visible";
    return roles;
}

int TransactionModel::rowCount(const QModelIndex &parent) const
{
    // Root element parents all children
    if (!parent.isValid()) {
        return m_transactions.size();
    }

    // Child elements have no children themselves
    return 0;
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Transaction *transaction = m_transactions[index.row()];
    switch (role) {
    case TransactionRoleRole:
        return transaction->role();
    case TransactionStatusRole:
        return transaction->status();
    case CancellableRole:
        return transaction->isCancellable();
    case ProgressRole:
        return transaction->progress();
    case StatusTextRole:
        switch (transaction->status()) {
        case Transaction::SetupStatus:
            return i18nc("@info:status", "Starting");
        case Transaction::QueuedStatus:
            return i18nc("@info:status", "Waiting");
        case Transaction::DownloadingStatus:
            return i18nc("@info:status", "Downloading");
        case Transaction::CommittingStatus:
            switch (transaction->role()) {
            case Transaction::InstallRole:
                return i18nc("@info:status", "Installing");
            case Transaction::RemoveRole:
                return i18nc("@info:status", "Removing");
            case Transaction::ChangeAddonsRole:
                return i18nc("@info:status", "Changing Addons");
            }
            break;
        case Transaction::DoneStatus:
            return i18nc("@info:status", "Done");
        case Transaction::DoneWithErrorStatus:
            return i18nc("@info:status", "Failed");
        case Transaction::CancelledStatus:
            return i18nc("@info:status", "Cancelled");
        }
        break;
    case TransactionRole:
        return QVariant::fromValue<QObject *>(transaction);
    case ResourceRole:
        return QVariant::fromValue<QObject *>(transaction->resource());
    case VisibleRole:
        return transaction->isVisible();
    }

    return QVariant();
}

Transaction *TransactionModel::transactionFromResource(AbstractResource *resource) const
{
    for (const auto transaction : std::as_const(m_transactions)) {
        if (transaction->resource() == resource) {
            return transaction;
        }
    }

    return nullptr;
}

QModelIndex TransactionModel::indexOf(Transaction *transaction) const
{
    int row = m_transactions.indexOf(transaction);
    QModelIndex ret = index(row);
    Q_ASSERT(!transaction || ret.isValid());
    return ret;
}

QModelIndex TransactionModel::indexOf(AbstractResource *resource) const
{
    Transaction *transaction = transactionFromResource(resource);

    return indexOf(transaction);
}

void TransactionModel::addTransaction(Transaction *transaction)
{
    if (!transaction) {
        return;
    }

    if (m_transactions.contains(transaction)) {
        return;
    }

    if (m_transactions.isEmpty()) {
        Q_EMIT startingFirstTransaction();
    }

    int before = m_transactions.size();
    beginInsertRows(QModelIndex(), before, before + 1);
    m_transactions.append(transaction);

    if (before == 0) { // Should emit before count changes
        Q_EMIT mainTransactionTextChanged();
    }
    endInsertRows();

    connect(transaction, &Transaction::statusChanged, this, [this, transaction]() {
        transactionChanged(transaction, StatusTextRole);
    });
    connect(transaction, &Transaction::cancellableChanged, this, [this, transaction]() {
        transactionChanged(transaction, CancellableRole);
    });
    connect(transaction, &Transaction::progressChanged, this, [this, transaction]() {
        transactionChanged(transaction, ProgressRole);
        Q_EMIT progressChanged();
    });

    Q_EMIT transactionAdded(transaction);
}

void TransactionModel::removeTransaction(Transaction *transaction)
{
    Q_ASSERT(transaction);
    transaction->deleteLater();
    int index = m_transactions.indexOf(transaction);
    if (index < 0) {
        qCWarning(LIBDISCOVER_LOG) << "transaction not part of the model" << transaction;
        return;
    }

    disconnect(transaction, nullptr, this, nullptr);

    beginRemoveRows(QModelIndex(), index, index);
    m_transactions.removeAt(index);
    endRemoveRows();

    Q_EMIT transactionRemoved(transaction);
    if (m_transactions.isEmpty()) {
        Q_EMIT lastTransactionFinished();
    }

    if (index == 0) {
        Q_EMIT mainTransactionTextChanged();
    }
}

void TransactionModel::transactionChanged(Transaction *transaction, int role)
{
    QModelIndex index = indexOf(transaction);
    Q_EMIT dataChanged(index, index, {role});
}

int TransactionModel::progress() const
{
    int sum = 0;
    int count = 0;
    for (const auto transaction : std::as_const(m_transactions)) {
        if (transaction->isActive() && transaction->isVisible()) {
            sum += transaction->progress();
            ++count;
        }
    }
    return count == 0 ? 0 : sum / count;
}

QString TransactionModel::mainTransactionText() const
{
    return m_transactions.isEmpty() ? QString() : m_transactions.constFirst()->name();
}

int TransactionModel::visibleTransactionCount() const
{
    const auto iterator = std::count_if(m_transactions.constBegin(), m_transactions.constEnd(), [](const Transaction *transaction) {
        return transaction->isVisible();
    });

    return iterator;
}

#include "moc_TransactionModel.cpp"
