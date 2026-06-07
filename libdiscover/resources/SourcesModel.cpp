/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SourcesModel.h"
#include "libdiscover_debug.h"
#include "resources/AbstractResourcesBackend.h"
#include "resources/AbstractSourcesBackend.h"
#include <QtGlobal>
#include <utils.h>

Q_GLOBAL_STATIC(SourcesModel, s_sources)

const auto DisplayName = "DisplayName";
const auto SourcesBackendId = "SourcesBackend";

SourcesModel::SourcesModel(QObject *parent)
    : QConcatenateTablesProxyModel(parent)
{
}

SourcesModel::~SourcesModel() = default;

SourcesModel *SourcesModel::global()
{
    return s_sources;
}

QHash<int, QByteArray> SourcesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QConcatenateTablesProxyModel::roleNames();
    roles.insert(AbstractSourcesBackend::IdRole, "sourceId");
    roles.insert(AbstractSourcesBackend::DisambiguatedIdRole, "disambiguatedSourceId");
    roles.insert(Qt::DisplayRole, "display");
    roles.insert(Qt::ToolTipRole, "toolTip");
    roles.insert(Qt::CheckStateRole, "checkState");
    roles.insert(SourceNameRole, "sourceName");
    roles.insert(SourcesBackend, "sourcesBackend");
    roles.insert(ResourcesBackend, "resourcesBackend");
    roles.insert(EnabledRole, "enabled");
    return roles;
}

void SourcesModel::addSourcesBackend(AbstractSourcesBackend *sources)
{
    auto backend = qobject_cast<AbstractResourcesBackend *>(sources->parent());

    auto m = sources->sources();
    m->setProperty(DisplayName, backend->displayName());
    m->setProperty(SourcesBackendId, QVariant::fromValue<QObject *>(sources));

    // QConcatenateTablesProxyModel will consider empty models as column==0. Empty models
    // will have 0 columns so it ends up showing nothing
    if (m->rowCount() == 0) {
        qWarning() << "adding empty sources model" << m;
        auto action = new OneTimeAction(
            [this, m] {
                addSourceModel(m);
                Q_EMIT sourcesChanged();
            },
            this);
        connect(m, &QAbstractItemModel::rowsInserted, action, &OneTimeAction::trigger);
    } else {
        addSourceModel(m);
        Q_EMIT sourcesChanged();
    }
}

const QAbstractItemModel *SourcesModel::modelAt(const QModelIndex &index) const
{
    return mapToSource(index).model();
}

QVariant SourcesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    switch (role) {
    case SourceNameRole:
        return modelAt(index)->property(DisplayName);
    case SourcesBackend:
        return modelAt(index)->property(SourcesBackendId);
    case EnabledRole:
        return QVariant(flags(index) & Qt::ItemIsEnabled);
    default:
        return QConcatenateTablesProxyModel::data(index, role);
    }
}

AbstractSourcesBackend *SourcesModel::sourcesBackendByName(const QString &id) const
{
    for (int i = 0, c = rowCount(); i < c; ++i) {
        const auto idx = index(i, 0);
        if (idx.data(SourceNameRole) == id) {
            return qobject_cast<AbstractSourcesBackend *>(idx.data(SourcesBackend).value<QObject *>());
        }
    }
    return nullptr;
}

QVector<AbstractSourcesBackend *> SourcesModel::sources() const
{
    QVector<AbstractSourcesBackend *> sources;
    for (int i = 0, c = rowCount(); i < c; ++i) {
        const auto idx = index(i, 0);
        auto source = qobject_cast<AbstractSourcesBackend *>(modelAt(idx)->property(SourcesBackendId).value<QObject *>());
        if (!sources.contains(source)) {
            sources += source;
        }
    }
    return sources;
}

#include "moc_SourcesModel.cpp"
