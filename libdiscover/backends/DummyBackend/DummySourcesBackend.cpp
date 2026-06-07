/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DummySourcesBackend.h"
#include "resources/DiscoverAction.h"
#include <QDebug>

DummySourcesBackend::DummySourcesBackend(AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new QStandardItemModel(this))
    , m_testAction(new DiscoverAction(QStringLiteral("kalgebra"), QStringLiteral("DummyAction"), this))
{
    for (int i = 0; i < 10; ++i)
        addSource(QStringLiteral("DummySource%1").arg(i));

    connect(m_testAction, &DiscoverAction::triggered, []() {
        qDebug() << "action triggered!";
    });
    connect(m_sources, &QStandardItemModel::itemChanged, this, [](QStandardItem *item) {
        qDebug() << "DummySource changed" << item << item->checkState();
    });
}

QAbstractItemModel *DummySourcesBackend::sources()
{
    return m_sources;
}

bool DummySourcesBackend::addSource(const QString &id)
{
    if (id.isEmpty())
        return false;

    QStandardItem *it = new QStandardItem(id);
    it->setData(id, AbstractSourcesBackend::IdRole);
    it->setData(QVariant(id + QLatin1Char(' ') + id), Qt::ToolTipRole);
    it->setCheckable(true);
    it->setCheckState(Qt::Checked);
    m_sources->appendRow(it);
    return true;
}

QStandardItem *DummySourcesBackend::sourceForId(const QString &id) const
{
    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        const auto it = m_sources->item(i, 0);
        if (it->text() == id)
            return it;
    }
    return nullptr;
}

bool DummySourcesBackend::removeSource(const QString &id)
{
    const auto it = sourceForId(id);
    if (!it) {
        Q_EMIT passiveMessage(QStringLiteral("Could not find %1").arg(id));
        return false;
    }
    return m_sources->removeRow(it->row());
}

QVariantList DummySourcesBackend::actions() const
{
    return QVariantList() << QVariant::fromValue<QObject *>(m_testAction);
}

bool DummySourcesBackend::moveSource(const QString &sourceId, int delta)
{
    int row = sourceForId(sourceId)->row();
    auto prevRow = m_sources->takeRow(row);
    Q_ASSERT(!prevRow.isEmpty());

    const auto destRow = row + delta;
    m_sources->insertRow(destRow, prevRow);
    if (destRow == 0 || row == 0)
        Q_EMIT firstDisambiguatedSourceIdChanged();
    if (destRow == m_sources->rowCount() - 1 || row == m_sources->rowCount() - 1)
        Q_EMIT lastDisambiguatedSourceIdChanged();
    return true;
}

#include "moc_DummySourcesBackend.cpp"
