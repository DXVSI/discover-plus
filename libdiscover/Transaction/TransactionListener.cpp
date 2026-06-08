/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "TransactionListener.h"

#include "../resources/AbstractResource.h"
#include "TransactionModel.h"
#include "libdiscover_debug.h"
#include <QMetaProperty>

TransactionListener::TransactionListener(QObject *parent)
    : QObject(parent)
    , m_resource(nullptr)
    , m_transaction(nullptr)
{
    connect(TransactionModel::global(), &TransactionModel::transactionAdded, this, &TransactionListener::transactionAdded);
}

void TransactionListener::cancel()
{
    if (!isCancellable()) {
        return;
    }
    m_transaction->cancel();
}

bool TransactionListener::isCancellable() const
{
    return m_transaction && m_transaction->isCancellable();
}

bool TransactionListener::isActive() const
{
    return m_transaction;
}

QString TransactionListener::statusText() const
{
    QModelIndex index = TransactionModel::global()->indexOf(m_resource);

    return index.data(TransactionModel::StatusTextRole).toString();
}

void TransactionListener::setResource(AbstractResource *resource)
{
    setResourceInternal(resource);
    // Catch already-started transactions
    setTransaction(TransactionModel::global()->transactionFromResource(resource));
}

void TransactionListener::setResourceInternal(AbstractResource *resource)
{
    if (m_resource == resource) {
        return;
    }

    m_resource = resource;
    Q_EMIT resourceChanged();
}

void TransactionListener::transactionAdded(Transaction *trans)
{
    if (trans->resource() != m_resource) {
        return;
    }

    setTransaction(trans);
}

class CheckChange
{
public:
    CheckChange(QObject *obj, const QByteArray &prop)
        : m_object(obj)
        , m_prop(obj->metaObject()->property(obj->metaObject()->indexOfProperty(prop.constData())))
        , m_oldValue(m_prop.read(obj))
    {
        Q_ASSERT(obj->metaObject()->indexOfProperty(prop.constData()) >= 0);
    }

    ~CheckChange()
    {
        const QVariant newValue = m_prop.read(m_object);
        if (newValue != m_oldValue) {
            QMetaMethod m = m_prop.notifySignal();
            m.invoke(m_object, Qt::DirectConnection);
        }
    }

private:
    QObject *m_object;
    QMetaProperty m_prop;
    QVariant m_oldValue;
};

void TransactionListener::setTransaction(Transaction *trans)
{
    if (m_transaction == trans) {
        return;
    }

    if (m_transaction) {
        disconnect(m_transaction, nullptr, this, nullptr);
    }

    CheckChange change1(this, "isCancellable");
    CheckChange change2(this, "isActive");
    CheckChange change3(this, "statusText");
    CheckChange change4(this, "progress");

    m_transaction = trans;
    if (m_transaction) {
        connect(m_transaction, &Transaction::cancellableChanged, this, &TransactionListener::cancellableChanged);
        connect(m_transaction, &Transaction::statusChanged, this, &TransactionListener::transactionStatusChanged);
        connect(m_transaction, &Transaction::progressChanged, this, &TransactionListener::progressChanged);
        connect(m_transaction, &QObject::destroyed, this, [this]() {
            qCDebug(LIBDISCOVER_LOG) << "destroyed transaction before finishing";
            setTransaction(nullptr);
        });
        setResourceInternal(trans->resource());
    }
    Q_EMIT transactionChanged(trans);
}

void TransactionListener::transactionStatusChanged(Transaction::Status status)
{
    switch (status) {
    case Transaction::CancelledStatus:
        setTransaction(nullptr);
        Q_EMIT cancelled();
        break;
    case Transaction::DoneWithErrorStatus:
    case Transaction::DoneStatus:
        setTransaction(nullptr);
        break;
    default:
        break;
    }

    Q_EMIT statusTextChanged();
}

int TransactionListener::progress() const
{
    return m_transaction ? m_transaction->progress() : 0;
}

#include "moc_TransactionListener.cpp"
