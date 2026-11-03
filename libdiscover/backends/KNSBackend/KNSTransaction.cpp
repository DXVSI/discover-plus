/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KNSTransaction.h"
#include "KNSBackend.h"
#include <KLocalizedString>
#include <KNSCore/EngineBase>
#include <KNSCore/Question>
#include <KNSCore/Transaction>

#include <QTimer>
#include <Transaction/TransactionModel.h>

KNSTransaction::KNSTransaction(KNSResource *res, Role role)
    : Transaction(res, role)
    , m_id(res->entry().uniqueId())
{
    setCancellable(false);
    TransactionModel::global()->addTransaction(this);

    QTimer::singleShot(0, res, [this, res, role] {
        auto engine = res->knsBackend()->engine();
        KNSCore::Transaction *knsTransaction = nullptr;

        if (role == RemoveRole) {
            knsTransaction = KNSCore::Transaction::uninstall(engine, res->entry());
        } else if (res->entry().status() == KNSCore::Entry::Updateable) {
            knsTransaction = KNSCore::Transaction::installLatest(engine, res->entry());
        } else if (res->linkIds().isEmpty()) {
            qWarning() << "No installable candidates in the KNewStuff entry" << res->entry().name() << "with id" << res->entry().uniqueId() << "on the backend"
                       << res->backend()->name()
                       << "There should always be at least one downloadable item in an OCS entry, and if there isn't, we should consider it broken. OCS "
                          "can technically show them, but if there is nothing to install, it cannot be installed.";
            setStatus(DoneStatus);
            return;
        } else {
            knsTransaction = KNSCore::Transaction::installLatest(engine, res->entry());
        }

        connect(knsTransaction, &KNSCore::Transaction::signalEntryEvent, this, [this, res](const KNSCore::Entry &entry, KNSCore::Entry::EntryEvent event) {
            switch (event) {
            case KNSCore::Entry::StatusChangedEvent:
                anEntryChanged(entry);
                break;
            case KNSCore::Entry::DetailsLoadedEvent:
            case KNSCore::Entry::AdoptedEvent:
            case KNSCore::Entry::UnknownEvent:
            default:
                break;
            }
            res->knsBackend()->slotEntryEvent(entry, event);
        });
        connect(knsTransaction, &KNSCore::Transaction::finished, this, [this] {
            if (status() != DoneStatus) {
                setStatus(DoneStatus);
            }
        });
        connect(knsTransaction, &KNSCore::Transaction::signalErrorCode, this, [this](KNSCore::ErrorCode::ErrorCode /*errorCode*/, const QString &message) {
            Q_EMIT passiveMessage(message);
        });
    });
}

void KNSTransaction::addQuestion(KNSCore::Question *question)
{
    Q_ASSERT(question->questionType() == KNSCore::Question::ContinueCancelQuestion);
    m_questions << question;

    Q_EMIT proceedRequest(question->title(), question->question());
}

void KNSTransaction::anEntryChanged(const KNSCore::Entry &entry)
{
    if (entry.uniqueId() == m_id) {
        switch (entry.status()) {
        case KNSCore::Entry::Invalid:
            qWarning() << "invalid status for" << entry.uniqueId() << entry.status();
            break;
        case KNSCore::Entry::Installing:
        case KNSCore::Entry::Updating:
            setStatus(CommittingStatus);
            break;
        case KNSCore::Entry::Downloadable:
        case KNSCore::Entry::Installed:
        case KNSCore::Entry::Deleted:
        case KNSCore::Entry::Updateable:
            break;
        }
    }
}

void KNSTransaction::cancel()
{
    for (auto q : std::as_const(m_questions)) {
        q->setResponse(KNSCore::Question::CancelResponse);
    }
    setStatus(CancelledStatus);
}

void KNSTransaction::proceed()
{
    m_questions.takeFirst()->setResponse(KNSCore::Question::ContinueResponse);
}

#include "moc_KNSTransaction.cpp"
