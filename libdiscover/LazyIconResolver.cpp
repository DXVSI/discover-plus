// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include "LazyIconResolver.h"

#include <QCoreApplication>
#include <QEvent>

namespace
{
constexpr auto LoopEvent = QEvent::User;
} // namespace

LazyIconResolver *LazyIconResolver::instance()
{
    static LazyIconResolver resolver;
    return &resolver;
}

void LazyIconResolver::queue(AbstractResource *resource)
{
    if (m_resources.isEmpty()) {
        QCoreApplication::postEvent(this, new QEvent(LoopEvent), Qt::LowEventPriority);
    }
    m_resources.append(QPointer{resource});
}

void LazyIconResolver::resolve()
{
    if (m_resources.empty()) {
        return;
    }

    // We take the last so the most recent request gets resolved first.
    auto resource = m_resources.takeLast();
    if (!resource) {
        return;
    }

    if (resource->hasResolvedIcon()) {
        return;
    }

    resource->resolveIcon();
}

void LazyIconResolver::customEvent(QEvent *event)
{
    if (event->type() == LoopEvent) {
        resolve();
        // Schedule a new loop run unconditionally of whether we resolved anything. So long as the queue is not empty.
        if (!m_resources.isEmpty()) {
            QCoreApplication::postEvent(this, new QEvent(LoopEvent), Qt::LowEventPriority);
        }
    }
    QObject::customEvent(event);
}

#include "moc_LazyIconResolver.cpp"
