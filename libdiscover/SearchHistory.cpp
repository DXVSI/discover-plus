/*
 *   SPDX-FileCopyrightText: 2024 KDE Developers
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SearchHistory.h"
#include <QDebug>

SearchHistory::SearchHistory(QObject *parent)
    : QObject(parent)
    , m_maxItems(20)
    , m_settings(QStringLiteral("KDE"), QStringLiteral("Discover"))
{
    loadHistory();
}

SearchHistory::~SearchHistory()
{
    saveHistory();
}

QStringList SearchHistory::history() const
{
    return m_history;
}

int SearchHistory::maxItems() const
{
    return m_maxItems;
}

void SearchHistory::setMaxItems(int max)
{
    if (m_maxItems != max) {
        m_maxItems = max;

        // Trim history if it exceeds new max
        if (m_history.size() > m_maxItems) {
            m_history = m_history.mid(0, m_maxItems);
            saveHistory();
            Q_EMIT historyChanged();
        }

        Q_EMIT maxItemsChanged();
    }
}

void SearchHistory::addSearchTerm(const QString &term)
{
    if (term.isEmpty()) {
        return;
    }

    QString trimmedTerm = term.trimmed();

    // Remove if it already exists (we'll add it to the top)
    m_history.removeAll(trimmedTerm);

    // Add to the beginning
    m_history.prepend(trimmedTerm);

    // Keep within max items limit
    if (m_history.size() > m_maxItems) {
        m_history = m_history.mid(0, m_maxItems);
    }

    saveHistory();
    Q_EMIT historyChanged();
}

void SearchHistory::removeSearchTerm(const QString &term)
{
    if (m_history.removeAll(term) > 0) {
        saveHistory();
        Q_EMIT historyChanged();
    }
}

void SearchHistory::clearHistory()
{
    if (!m_history.isEmpty()) {
        m_history.clear();
        saveHistory();
        Q_EMIT historyChanged();
    }
}

QStringList SearchHistory::suggestionsForTerm(const QString &term) const
{
    if (term.isEmpty()) {
        // Return recent searches
        return m_history.mid(0, 10);
    }

    QString lowerTerm = term.toLower();
    QStringList suggestions;

    for (const QString &historyItem : m_history) {
        if (historyItem.toLower().contains(lowerTerm)) {
            suggestions.append(historyItem);
            if (suggestions.size() >= 10) {
                break;
            }
        }
    }

    return suggestions;
}

void SearchHistory::loadHistory()
{
    m_settings.beginGroup(QStringLiteral("SearchHistory"));
    m_history = m_settings.value(QStringLiteral("history")).toStringList();
    m_maxItems = m_settings.value(QStringLiteral("maxItems"), 20).toInt();
    m_settings.endGroup();
}

void SearchHistory::saveHistory()
{
    m_settings.beginGroup(QStringLiteral("SearchHistory"));
    m_settings.setValue(QStringLiteral("history"), m_history);
    m_settings.setValue(QStringLiteral("maxItems"), m_maxItems);
    m_settings.endGroup();
    m_settings.sync();
}