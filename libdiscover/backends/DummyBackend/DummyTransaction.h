/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <Transaction/Transaction.h>

class DummyResource;

class OverseeTransactions : public QObject
{
    Q_OBJECT
public:
    static OverseeTransactions *self()
    {
        static OverseeTransactions *m_self = nullptr;
        if (!m_self) {
            m_self = new OverseeTransactions;
        }
        return m_self;
    }
Q_SIGNALS:
    void transactionFinished();
};

class DummyTransaction : public Transaction
{
    Q_OBJECT
public:
    DummyTransaction(DummyResource *app, Role role);
    DummyTransaction(DummyResource *app, const AddonList &list, Role role);

    void cancel() override;
    void proceed() override;

private Q_SLOTS:
    void iterateTransaction();
    void finishTransaction();

private:
    void considerStarting();
    bool m_iterate = true;
    DummyResource *m_app;
};
