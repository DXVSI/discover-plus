/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UnattendedUpdates.h"
#include "DiscoverNotifier.h"
#include "updatessettings.h"
#include <KIdleTime>
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <chrono>

UnattendedUpdates::UnattendedUpdates(DiscoverNotifier *parent)
    : QObject()
    , m_notifier(parent)
{
    connect(parent, &DiscoverNotifier::stateChanged, this, &UnattendedUpdates::checkNewState);
    connect(KIdleTime::instance(), QOverload<int, int>::of(&KIdleTime::timeoutReached), this, &UnattendedUpdates::triggerUpdate);

    checkNewState();
}

void UnattendedUpdates::checkNewState()
{
    using namespace std::chrono_literals;

    // Only allow offline updating every 3h. It should keep some peace to our users, especially on rolling distros
    const QDateTime updateableTime = m_notifier->settings()->lastUnattendedTrigger().addSecs(std::chrono::seconds(3h).count());
    if (updateableTime > QDateTime::currentDateTimeUtc()) {
        qDebug() << "skipping update, already updated on" << m_notifier->settings()->lastUnattendedTrigger().toString();
        return;
    }

    const bool doTrigger = m_notifier->isSystemUpdateable();
    if (doTrigger && !m_idleTimeoutId.has_value()) {
        qDebug() << "waiting for an idle moment";
        // If the system is untouched for 1 minute, trigger the unattended update.
        // The trick here is that we want to trigger the update, but ideally not when the user is using the computer.
        // The idle timeout cannot be too long or it won't ever update, but it also shouldn't be none because then
        // we definitely trigger the update while the user is using the computer. So we arrived at 1 minute.
        // Should this not work out, a more expansive solution needs inventing (e.g. cgroup constraining of resource use).
        m_idleTimeoutId.emplace(1min);
    } else if (!doTrigger && m_idleTimeoutId.has_value()) {
        qDebug() << "nothing to do";
        m_idleTimeoutId.reset();
    }
}

void UnattendedUpdates::triggerUpdate(int timeoutId)
{
    if (!m_idleTimeoutId.has_value() || timeoutId != m_idleTimeoutId->m_id) {
        return;
    }

    m_idleTimeoutId.reset();

    if (!m_notifier->isSystemUpdateable()) {
        qWarning() << "Refusing to start unattended update because of state" << m_notifier->state();
        return;
    }

    m_notifier->startUnattendedUpdates();
}

UnattendedUpdates::IdleHandle::IdleHandle(const std::chrono::milliseconds &idleTimeout)
    : m_id(KIdleTime::instance()->addIdleTimeout(int(idleTimeout.count())))
{
}

UnattendedUpdates::IdleHandle::~IdleHandle()
{
    KIdleTime::instance()->removeIdleTimeout(m_id);
}

#include "moc_UnattendedUpdates.cpp"
