/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "LimitedRowCountProxyModel.h"

using namespace std::chrono_literals;

LimitedRowCountProxyModel::LimitedRowCountProxyModel(QObject *object)
    : QSortFilterProxyModel(object)
{
    // Compress events to avoid excessive filter runs
    m_invalidateTimer.setInterval(250ms);
    m_invalidateTimer.setSingleShot(true);
    connect(&m_invalidateTimer, &QTimer::timeout, this, [this] {
        beginFilterChange();
        endFilterChange();
    });

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this] {
        if (!sourceModel()) {
            return;
        }

        // Only support running once to not have to implement disconnecting.
        Q_ASSERT(!m_connected);
        if (m_connected) {
            return;
        }
        m_connected = true;

        connect(sourceModel(), &QAbstractItemModel::rowsInserted, &m_invalidateTimer, QOverload<>::of(&QTimer::start));
        connect(sourceModel(), &QAbstractItemModel::rowsRemoved, &m_invalidateTimer, QOverload<>::of(&QTimer::start));
        connect(sourceModel(), &QAbstractItemModel::modelReset, &m_invalidateTimer, QOverload<>::of(&QTimer::start));
        beginFilterChange();
        endFilterChange();
    });
}

LimitedRowCountProxyModel::~LimitedRowCountProxyModel() = default;

int LimitedRowCountProxyModel::pageSize() const
{
    return m_pageSize;
}

void LimitedRowCountProxyModel::setPageSize(int count)
{
    if (count == m_pageSize) {
        return;
    }

    m_pageSize = count;
    Q_EMIT pageSizeChanged();

    beginFilterChange();
    endFilterChange();
}

bool LimitedRowCountProxyModel::filterAcceptsRow(int source_row, [[maybe_unused]] const QModelIndex &source_parent) const
{
    return source_row >= 0 && source_row < m_pageSize;
}
