/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UpdateModel.h"

#include <algorithm>

// Qt includes
#include "libdiscover_debug.h"
#include <QFont>
#include <QTimer>

// KDE includes
#include <KFormat>
#include <KLocalizedString>

// Own includes
#include "UpdateItem.h"
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesUpdatesModel.h>

UpdateModel::UpdateModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_updateSizeTimer(new QTimer(this))
    , m_updates(nullptr)
{
    connect(ResourcesModel::global(), &ResourcesModel::fetchingChanged, this, &UpdateModel::activityChanged);
    connect(ResourcesModel::global(), &ResourcesModel::updatesCountChanged, this, &UpdateModel::activityChanged);
    connect(this, &UpdateModel::toUpdateChanged, this, &UpdateModel::updateSizeChanged);

    m_updateSizeTimer->setInterval(100);
    m_updateSizeTimer->setSingleShot(true);
    connect(m_updateSizeTimer, &QTimer::timeout, this, &UpdateModel::updateSizeChanged);
}

UpdateModel::~UpdateModel()
{
    qDeleteAll(m_updateItems);
    m_updateItems.clear();
}

QHash<int, QByteArray> UpdateModel::roleNames() const
{
    auto ret = QAbstractItemModel::roleNames();
    ret.insert(Qt::CheckStateRole, "checked");
    ret.insert(ResourceProgressRole, "resourceProgress");
    ret.insert(ResourceStateRole, "resourceState");
    ret.insert(ResourceStateIsDoneRole, "resourceStateIsDone");
    ret.insert(ResourceRole, "resource");
    ret.insert(SizeRole, "size");
    ret.insert(SectionRole, "section");
    ret.insert(ChangelogRole, "changelog");
    ret.insert(ExtendedRole, "extended");
    return ret;
}

void UpdateModel::setBackend(ResourcesUpdatesModel *updates)
{
    if (m_updates) {
        disconnect(m_updates, &ResourcesUpdatesModel::progressingChanged, this, &UpdateModel::activityChanged);
        disconnect(m_updates, &ResourcesUpdatesModel::resourceProgressed, this, &UpdateModel::resourceHasProgressed);
    }

    m_updates = updates;

    if (m_updates) {
        connect(m_updates, &ResourcesUpdatesModel::progressingChanged, this, &UpdateModel::activityChanged);
        connect(m_updates, &ResourcesUpdatesModel::resourceProgressed, this, &UpdateModel::resourceHasProgressed);
    }

    activityChanged();
}

void UpdateModel::resourceHasProgressed(AbstractResource *res, qreal progress, AbstractBackendUpdater::State state)
{
    UpdateItem *item = itemFromResource(res);
    if (!item) {
        return;
    }
    item->setProgress(progress);
    item->setState(state);

    const QModelIndex idx = indexFromItem(item);
    Q_EMIT dataChanged(idx, idx, {ResourceProgressRole, ResourceStateRole, ResourceStateIsDoneRole, SectionResourceProgressRole});
}

void UpdateModel::activityChanged()
{
    if (m_updates) {
        if (!m_updates->isProgressing()) {
            m_updates->prepare();
            setResources(m_updates->toUpdate());

            for (auto item : std::as_const(m_updateItems)) {
                item->setProgress(0);
            }
        } else {
            setResources(m_updates->toUpdate());
        }
    }
}

QVariant UpdateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    UpdateItem *item = itemFromIndex(index);

    switch (role) {
    case Qt::DisplayRole:
        return item->name();
    case Qt::DecorationRole:
        return item->icon();
    case Qt::CheckStateRole:
        return item->checked();
    case SizeRole:
        return item->size() > 0 ? KFormat().formatByteSize(item->size()) : i18n("Unknown");
    case ResourceRole:
        return QVariant::fromValue<QObject *>(item->resource());
    case ResourceProgressRole:
        return item->progress();
    case ResourceStateRole:
        return item->state();
    case ResourceStateIsDoneRole:
        return item->state() == AbstractBackendUpdater::State::Done;
    case ChangelogRole:
        return item->changelog();
    case ExtendedRole:
        return item->isExtended();
    case SectionRole: {
        static const QString appUpdatesSection = i18nc("@item:inlistbox", "Applications");
        static const QString applicationSupportSection = i18nc("@item:inlistbox", "Application Support");
        static const QString systemUpdateSection = i18nc("@item:inlistbox", "System Software");
        static const QString addonsSection = i18nc("@item:inlistbox", "Addons");
        switch (item->resource()->type()) {
        case AbstractResource::Application:
            return appUpdatesSection;
        case AbstractResource::ApplicationSupport:
            return applicationSupportSection;
        case AbstractResource::System:
            return systemUpdateSection;
        case AbstractResource::Addon:
            return addonsSection;
        }
        Q_UNREACHABLE();
    }
    case SectionResourceProgressRole:
        return (100 - item->progress()) + (101 * item->resource()->type());
    default:
        break;
    }

    return QVariant();
}

void UpdateModel::checkResources(const QList<AbstractResource *> &resource, bool checked)
{
    if (checked) {
        m_updates->addResources(resource);
    } else {
        m_updates->removeResources(resource);
    }
}

Qt::ItemFlags UpdateModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int UpdateModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_updateItems.count() : 0;
}

bool UpdateModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        UpdateItem *item = itemFromIndex(idx);
        const bool newValue = value.toInt() == Qt::Checked;
        const QList<AbstractResource *> apps = {item->app()};

        checkResources(apps, newValue);
        Q_ASSERT(idx.data(Qt::CheckStateRole) == value);

        // When un/checking some backends will decide to add or remove a bunch of packages, so refresh it all
        auto m = idx.model();
        Q_EMIT dataChanged(m->index(0, 0), m->index(m->rowCount() - 1, 0), {Qt::CheckStateRole});
        Q_EMIT toUpdateChanged();

        return true;
    } else if (role == ExtendedRole) {
        UpdateItem *item = itemFromIndex(idx);
        if (item->isExtended() != value.toBool()) {
            item->setExtended(value.toBool());
            Q_EMIT dataChanged(idx, idx, {ExtendedRole});
        }
    }

    return false;
}

void UpdateModel::fetchUpdateDetails(int row)
{
    UpdateItem *item = itemFromIndex(index(row, 0));
    Q_ASSERT(item);
    if (!item) {
        return;
    }

    item->app()->fetchUpdateDetails();
}

void UpdateModel::integrateChangelog(const QString &changelog)
{
    auto app = qobject_cast<AbstractResource *>(sender());
    Q_ASSERT(app);
    auto item = itemFromResource(app);
    if (!item) {
        return;
    }

    item->setChangelog(changelog);

    const QModelIndex idx = indexFromItem(item);
    Q_ASSERT(idx.isValid());
    Q_EMIT dataChanged(idx, idx, {ChangelogRole});
}

void UpdateModel::setResources(const QList<AbstractResource *> &resources)
{
    if (resources == m_resources) {
        return;
    }
    const auto oldResources = m_resources;
    for (auto resource : std::as_const(m_resources)) {
        if (!resources.contains(resource)) {
            disconnect(resource, nullptr, this, nullptr);
        }
    }
    m_resources = resources;

    beginResetModel();
    qDeleteAll(m_updateItems);
    m_updateItems.clear();

    QVector<UpdateItem *> appItems, appSupportItems, systemItems, addonItems;
    for (AbstractResource *resource : resources) {
        if (!oldResources.contains(resource)) {
            connect(resource, &AbstractResource::changelogFetched, this, &UpdateModel::integrateChangelog, Qt::UniqueConnection);
            connect(resource, &AbstractResource::sizeChanged, this, [this, resource] {
                const auto index = indexFromResource(resource);
                Q_EMIT dataChanged(index, index, {SizeRole});
                m_updateSizeTimer->start();
            });
            connect(resource, &AbstractResource::iconChanged, this, [this, resource] {
                const auto index = indexFromResource(resource);
                Q_EMIT dataChanged(index, index, {Qt::DecorationRole});
            });
        }

        UpdateItem *updateItem = new UpdateItem(resource);

        switch (resource->type()) {
        case AbstractResource::Application:
            appItems += updateItem;
            break;
        case AbstractResource::ApplicationSupport:
            appSupportItems += updateItem;
            break;
        case AbstractResource::System:
            systemItems += updateItem;
            break;
        case AbstractResource::Addon:
            addonItems += updateItem;
            break;
        }
    }

    QCollator collator;
    const auto sortUpdateItems = [&collator](UpdateItem *a, UpdateItem *b) -> bool {
        return collator(a->name(), b->name());
    };
    std::ranges::sort(appItems, sortUpdateItems);
    std::ranges::sort(appSupportItems, sortUpdateItems);
    std::ranges::sort(systemItems, sortUpdateItems);
    std::ranges::sort(addonItems, sortUpdateItems);
    m_updateItems = (QVector<UpdateItem *>() << appItems << addonItems << appSupportItems << systemItems);
    endResetModel();

    Q_EMIT hasUpdatesChanged(!resources.isEmpty());
    Q_EMIT toUpdateChanged();
}

bool UpdateModel::hasUpdates() const
{
    return rowCount() > 0;
}

ResourcesUpdatesModel *UpdateModel::backend() const
{
    return m_updates;
}

int UpdateModel::toUpdateCount() const
{
    int ret = 0;
    QSet<QString> packages;
    for (UpdateItem *item : std::as_const(m_updateItems)) {
        const auto packageName = item->resource()->packageName();
        if (packages.contains(packageName)) {
            continue;
        }
        packages.insert(packageName);
        ret += item->checked() != Qt::Unchecked ? 1 : 0;
    }
    return ret;
}

int UpdateModel::totalUpdatesCount() const
{
    int ret = 0;
    QSet<QString> packages;
    for (UpdateItem *item : std::as_const(m_updateItems)) {
        const auto packageName = item->resource()->packageName();
        if (packages.contains(packageName)) {
            continue;
        }
        packages.insert(packageName);
        ret += 1;
    }
    return ret;
}

UpdateItem *UpdateModel::itemFromResource(AbstractResource *res) const
{
    for (UpdateItem *item : m_updateItems) {
        if (item->app() == res) {
            return item;
        }
    }
    return nullptr;
}

QString UpdateModel::updateSize() const
{
    if (!m_updates) {
        return QString();
    }
    if (m_updates->updateSize() != 0) {
        return KFormat().formatByteSize(m_updates->updateSize());
    }
    return i18n("Unknown");
}

QModelIndex UpdateModel::indexFromItem(UpdateItem *item) const
{
    return index(m_updateItems.indexOf(item), 0, {});
}

UpdateItem *UpdateModel::itemFromIndex(const QModelIndex &index) const
{
    return m_updateItems[index.row()];
}

QModelIndex UpdateModel::indexFromResource(AbstractResource *res) const
{
    return indexFromItem(itemFromResource(res));
}

void UpdateModel::checkAll()
{
    QList<AbstractResource *> updatedItems;

    for (int i = 0, c = rowCount(); i < c; ++i) {
        auto idx = index(i);
        if (idx.data(Qt::CheckStateRole) != Qt::Checked) {
            updatedItems.append(itemFromIndex(idx)->app());
        }
    }

    checkResources(updatedItems, true);

    Q_EMIT dataChanged(index(0), index(rowCount() - 1), {Qt::CheckStateRole});
    Q_EMIT toUpdateChanged();
}

void UpdateModel::uncheckAll()
{
    QList<AbstractResource *> updatedItems;

    for (int i = 0, c = rowCount(); i < c; ++i) {
        auto idx = index(i);
        if (idx.data(Qt::CheckStateRole) != Qt::Unchecked) {
            updatedItems.append(itemFromIndex(idx)->app());
        }
    }

    checkResources(updatedItems, false);

    Q_EMIT dataChanged(index(0), index(rowCount() - 1), {Qt::CheckStateRole});
    Q_EMIT toUpdateChanged();
}

#include "moc_UpdateModel.cpp"
