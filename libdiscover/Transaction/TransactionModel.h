/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>

#include "Transaction.h"

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT TransactionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString mainTransactionText READ mainTransactionText NOTIFY mainTransactionTextChanged)
public:
    enum Roles {
        TransactionRoleRole = Qt::UserRole,
        TransactionStatusRole,
        CancellableRole,
        ProgressRole,
        StatusTextRole,
        ResourceRole,
        TransactionRole,
        VisibleRole,
    };
    Q_ENUM(Roles)

    explicit TransactionModel(QObject *parent = nullptr);
    static TransactionModel *global();

    // Reimplemented from QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_SCRIPTABLE Transaction *transactionFromResource(AbstractResource *resource) const;
    QModelIndex indexOf(Transaction *transaction) const;
    QModelIndex indexOf(AbstractResource *resource) const;

    void addTransaction(Transaction *transaction);
    void removeTransaction(Transaction *transaction);

    bool contains(Transaction *transaction) const
    {
        return m_transactions.contains(transaction);
    }
    int progress() const;
    QVector<Transaction *> transactions() const
    {
        return m_transactions;
    }

    QString mainTransactionText() const;

    int visibleTransactionCount() const;

private:
    QVector<Transaction *> m_transactions;

Q_SIGNALS:
    void startingFirstTransaction();
    void lastTransactionFinished();
    void transactionAdded(Transaction *transaction);
    void transactionRemoved(Transaction *transaction);
    void countChanged();
    void progressChanged();
    void proceedRequest(Transaction *transaction, const QString &title, const QString &description);
    void mainTransactionTextChanged();

private:
    void transactionChanged(Transaction *transaction, int role);
};
