/*
 *   SPDX-FileCopyrightText: 2024 KDE Developers
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SEARCHHISTORY_H
#define SEARCHHISTORY_H

#include <QObject>
#include <QStringList>
#include <QSettings>
#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT SearchHistory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int maxItems READ maxItems WRITE setMaxItems NOTIFY maxItemsChanged)

public:
    explicit SearchHistory(QObject *parent = nullptr);
    ~SearchHistory() override;

    QStringList history() const;
    int maxItems() const;
    void setMaxItems(int max);

    Q_INVOKABLE void addSearchTerm(const QString &term);
    Q_INVOKABLE void removeSearchTerm(const QString &term);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE QStringList suggestionsForTerm(const QString &term) const;

Q_SIGNALS:
    void historyChanged();
    void maxItemsChanged();

private:
    void loadHistory();
    void saveHistory();

    QStringList m_history;
    int m_maxItems;
    QSettings m_settings;
};

#endif // SEARCHHISTORY_H