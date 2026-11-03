/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "KNSResource.h"
#include "Transaction/Transaction.h"
#include <KNSCore/QuestionListener>

class KNSTransaction;

class KNSTransaction : public Transaction
{
public:
    KNSTransaction(KNSResource *res, Transaction::Role role);

    void anEntryChanged(const KNSCore::Entry &entry);

    void addQuestion(KNSCore::Question *question);
    void cancel() override;
    void proceed() override;

    QString uniqueId() const
    {
        return m_id;
    }

private:
    const QString m_id;
    QVector<KNSCore::Question *> m_questions;
};
