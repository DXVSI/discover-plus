/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ResourcesProxyModel.h"

#include "libdiscover_debug.h"
#include <QMetaProperty>
#include <cmath>
#include <qnamespace.h>
#include <utils.h>

#include "ResourcesModel.h"
#include <Category/CategoryModel.h>
#include <KLocalizedString>
#include <ReviewsBackend/Rating.h>
#include <Transaction/TransactionModel.h>

const QHash<int, QByteArray> ResourcesProxyModel::s_roles = {{NameRole, "name"},
                                                             {IconRole, "icon"},
                                                             {CommentRole, "comment"},
                                                             {StateRole, "state"},
                                                             {RatingRole, "rating"},
                                                             {RatingPointsRole, "ratingPoints"},
                                                             {RatingCountRole, "ratingCount"},
                                                             {SortableRatingRole, "sortableRating"},
                                                             {SearchRelevanceRole, "searchRelevance"},
                                                             {InstalledRole, "isInstalled"},
                                                             {ApplicationRole, "application"},
                                                             {OriginRole, "origin"},
                                                             {DisplayOriginRole, "displayOrigin"},
                                                             {CanUpgrade, "canUpgrade"},
                                                             {PackageNameRole, "packageName"},
                                                             {CategoryRole, "category"},
                                                             {SectionRole, "section"},
                                                             {MimeTypes, "mimetypes"},
                                                             {LongDescriptionRole, "longDescription"},
                                                             {SourceIconRole, "sourceIcon"},
                                                             {SizeRole, "size"},
                                                             {ReleaseDateRole, "releaseDate"}};

int levenshteinDistance(QStringView source, QStringView target)
{
    if (source.compare(target, Qt::CaseInsensitive) == 0) {
        return 0;
    }

    const int sourceCount = source.size();
    const int targetCount = target.size();

    if (source.isEmpty()) {
        return targetCount;
    }

    if (target.isEmpty()) {
        return sourceCount;
    }

    QVector<int> column;
    column.fill(0, targetCount + 1);
    QVector<int> previousColumn;
    previousColumn.reserve(targetCount + 1);
    for (int i = 0; i < targetCount + 1; i++) {
        previousColumn.append(i);
    }

    for (int i = 0; i < sourceCount; i++) {
        column[0] = i + 1;
        for (int j = 0; j < targetCount; j++) {
            column[j + 1] =
                std::min({1 + column.at(j), 1 + previousColumn.at(1 + j), previousColumn.at(j) + ((source.at(i).toUpper() == target.at(j).toUpper()) ? 0 : 1)});
        }
        column.swap(previousColumn);
    }

    return previousColumn.at(targetCount);
}

ResourcesProxyModel::ResourcesProxyModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_sortRole(NameRole)
    , m_sortOrder(Qt::AscendingOrder)
    , m_currentStream(nullptr)
{
    // new QAbstractItemModelTester(this, this);

    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesProxyModel::invalidateFilter);
    connect(ResourcesModel::global(), &ResourcesModel::backendDataChanged, this, &ResourcesProxyModel::refreshBackend);
    connect(ResourcesModel::global(), &ResourcesModel::resourceDataChanged, this, &ResourcesProxyModel::refreshResource);
    connect(ResourcesModel::global(), &ResourcesModel::resourceRemoved, this, &ResourcesProxyModel::removeResource);

    m_countTimer.setInterval(10);
    m_countTimer.setSingleShot(true);
    connect(&m_countTimer, &QTimer::timeout, this, &ResourcesProxyModel::countChanged);

    connect(this, &QAbstractItemModel::modelReset, &m_countTimer, qOverload<>(&QTimer::start));
    connect(this, &QAbstractItemModel::rowsInserted, &m_countTimer, qOverload<>(&QTimer::start));
    connect(this, &QAbstractItemModel::rowsRemoved, &m_countTimer, qOverload<>(&QTimer::start));

    connect(this, &ResourcesProxyModel::busyChanged, &m_countTimer, qOverload<>(&QTimer::start));
}

void ResourcesProxyModel::componentComplete()
{
    m_setup = true;
    invalidateFilter();
}

QHash<int, QByteArray> ResourcesProxyModel::roleNames() const
{
    return s_roles;
}

void ResourcesProxyModel::setSortRole(Roles sortRole)
{
    if (sortRole != m_sortRole) {
        Q_ASSERT(roleNames().contains(sortRole));

        m_sortRole = sortRole;
        Q_EMIT sortRoleChanged(sortRole);
        invalidateSorting();
    }
}

void ResourcesProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    if (sortOrder != m_sortOrder) {
        m_sortOrder = sortOrder;
        Q_EMIT sortOrderChanged(sortOrder);
        invalidateSorting();
    }
}

void ResourcesProxyModel::setSearch(const QString &_searchText)
{
    // 1-character searches are painfully slow. >= 2 chars are fine, though
    const QString searchText = _searchText.size() <= 1 ? QString() : _searchText;

    if (m_filters.search != searchText) {
        m_filters.search = searchText;
        invalidateFilter();
        Q_EMIT searchChanged(m_filters.search);
    }
}

void ResourcesProxyModel::removeDuplicates(QVector<StreamResult> &resources)
{
    const auto currentApplicationBackend = ResourcesModel::global()->currentApplicationBackend();
    QHash<QString, QString> aliases;
    QHash<QString, QVector<StreamResult>::iterator> storedIds;
    for (auto it = m_displayedResources.begin(); it != m_displayedResources.end(); ++it) {
        const auto appstreamid = it->resource->appstreamId();
        if (appstreamid.isEmpty()) {
            continue;
        }
        auto at = storedIds.find(appstreamid);
        if (at == storedIds.end()) {
            storedIds[appstreamid] = it;
        } else {
            qCWarning(LIBDISCOVER_LOG) << "We should have sanitized the displayed resources. There is a bug";
            Q_UNREACHABLE();
        }

        const auto alts = it->resource->alternativeAppstreamIds();
        for (const auto &alias : alts) {
            aliases[alias] = appstreamid;
        }
    }

    QHash<QString, QVector<StreamResult>::iterator> ids;
    for (auto it = resources.begin(); it != resources.end();) {
        const auto appstreamid = it->resource->appstreamId();
        if (appstreamid.isEmpty()) {
            ++it;
            continue;
        }
        auto at = storedIds.find(appstreamid);
        if (at == storedIds.end()) {
            auto aliased = aliases.constFind(appstreamid);
            if (aliased != aliases.constEnd()) {
                at = storedIds.find(aliased.value());
            }
        }

        if (at == storedIds.end()) {
            const auto alts = it->resource->alternativeAppstreamIds();
            for (const auto &alt : alts) {
                at = storedIds.find(alt);
                if (at == storedIds.end()) {
                    break;
                }

                auto aliased = aliases.constFind(alt);
                if (aliased != aliases.constEnd()) {
                    at = storedIds.find(aliased.value());
                    if (at != storedIds.end()) {
                        break;
                    }
                }
            }
        }
        if (at == storedIds.end()) {
            auto at = ids.find(appstreamid);
            if (at == ids.end()) {
                auto aliased = aliases.constFind(appstreamid);
                if (aliased != aliases.constEnd()) {
                    at = ids.find(aliased.value());
                }
            }
            if (at == ids.end()) {
                const auto alts = it->resource->alternativeAppstreamIds();
                for (const auto &alt : alts) {
                    at = ids.find(alt);
                    if (at != ids.end()) {
                        break;
                    }

                    auto aliased = aliases.constFind(appstreamid);
                    if (aliased != aliases.constEnd()) {
                        at = ids.find(aliased.value());
                        if (at != ids.end()) {
                            break;
                        }
                    }
                }
            }
            if (at == ids.end()) {
                ids[appstreamid] = it;
                const auto alts = it->resource->alternativeAppstreamIds();
                for (const auto &alias : alts) {
                    aliases[alias] = appstreamid;
                }
                ++it;
            } else {
                if (it->resource->backend() == currentApplicationBackend && it->resource->backend() != (**at).resource->backend()) {
                    qSwap(*it, **at);
                }
                it = resources.erase(it);
            }
        } else {
            if (it->resource->backend() == currentApplicationBackend) {
                **at = *it;
                auto pos = index(*at - m_displayedResources.begin(), 0);
                Q_EMIT dataChanged(pos, pos);
            }
            it = resources.erase(it);
        }
    }
}

void ResourcesProxyModel::addResources(const QVector<StreamResult> &results)
{
    auto resultsCopy = results;
    m_filters.filterJustInCase(resultsCopy);

    if (resultsCopy.isEmpty()) {
        return;
    }
    std::sort(resultsCopy.begin(), resultsCopy.end(), [this](const auto &left, const auto &right) {
        return orderedLessThan(left, right);
    });

    sortedInsertion(resultsCopy);
    fetchSubcategories();
}

void ResourcesProxyModel::invalidateSorting()
{
    if (m_displayedResources.isEmpty()) {
        return;
    }

    beginResetModel();
    std::sort(m_displayedResources.begin(), m_displayedResources.end(), [this](const auto &left, const auto &right) {
        return orderedLessThan(left, right);
    });
    endResetModel();
}

QString ResourcesProxyModel::lastSearch() const
{
    return m_filters.search;
}

void ResourcesProxyModel::setOriginFilter(const QString &origin)
{
    if (m_filters.origin != origin) {
        m_filters.origin = origin;
        invalidateFilter();
    }
}

QString ResourcesProxyModel::originFilter() const
{
    return m_filters.origin;
}

QString ResourcesProxyModel::filteredCategoryName() const
{
    return m_categoryName;
}

void ResourcesProxyModel::setFilteredCategoryName(const QString &categoryName)
{
    if (categoryName == m_categoryName) {
        return;
    }

    m_categoryName = categoryName;

    if (auto category = CategoryModel::global()->findCategoryByName(categoryName)) {
        setFiltersFromCategory(category);
    } else {
        qDebug() << "looking up wrong category or too early" << m_categoryName;
        auto f = [this, categoryName] {
            auto category = CategoryModel::global()->findCategoryByName(categoryName);
            setFiltersFromCategory(category);
        };
        auto action = new OneTimeAction(f, this);
        connect(CategoryModel::global(), &CategoryModel::rootCategoriesChanged, action, &OneTimeAction::trigger);
    }
}

void ResourcesProxyModel::setFiltersFromCategory(const std::shared_ptr<Category> &category)
{
    if (m_filters.category != category) {
        m_filters.category = category;
        invalidateFilter();
        Q_EMIT categoryChanged();
    }
}

void ResourcesProxyModel::fetchSubcategories()
{
    auto categories = kToSet(m_filters.category ? m_filters.category->subCategories() : CategoryModel::global()->rootCategories());

    const int count = rowCount();
    QSet<std::shared_ptr<Category>> done;
    for (int i = 0; i < count && !categories.isEmpty(); ++i) {
        const auto resource = m_displayedResources[i].resource;
        const auto found = resource->categoryObjects(kSetToVector(categories));
        done.unite(found);
        categories.subtract(found);
    }

    const QVariantList ret = kTransform<QVariantList>(done, [](const std::shared_ptr<Category> &category) {
        return QVariant::fromValue<std::shared_ptr<Category>>(category);
    });
    if (m_subcategories != ret) {
        m_subcategories = ret;
        Q_EMIT subcategoriesChanged(m_subcategories);
    }
}

QVariantList ResourcesProxyModel::subcategories() const
{
    return m_subcategories;
}

void ResourcesProxyModel::invalidateFilter()
{
    if (!m_setup || ResourcesModel::global()->backends().isEmpty()) {
        return;
    }

    if (!m_categoryName.isEmpty() && m_filters.category == nullptr) {
        return;
    }

    if (m_currentStream) {
        qCWarning(LIBDISCOVER_LOG) << "last stream isn't over yet" << m_filters << this;
        delete m_currentStream;
    }

    m_currentStream = m_filters.backend ? m_filters.backend->search(m_filters) : ResourcesModel::global()->search(m_filters);
    Q_EMIT busyChanged();

    if (!m_displayedResources.isEmpty()) {
        beginResetModel();
        m_displayedResources.clear();
        endResetModel();
    }

    connect(m_currentStream, &ResultsStream::resourcesFound, this, &ResourcesProxyModel::addResources);
    connect(m_currentStream, &ResultsStream::destroyed, this, [this]() {
        m_currentStream = nullptr;
        Q_EMIT busyChanged();
    });
}

int ResourcesProxyModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_displayedResources.count();
}

// This comparator takes m_sortRole and m_sortOrder into account. It has a
// fallback mechanism to use secondary sort role and order.
bool ResourcesProxyModel::orderedLessThan(const StreamResult &left, const StreamResult &right) const
{
    // (role, order) pair
    using SortCombination = std::pair<Roles, Qt::SortOrder>;
    const std::array<SortCombination, 2> sortFallbackChain = {{
        {m_sortRole, m_sortOrder},
        {NameRole, Qt::AscendingOrder},
    }};

    for (const auto &[role, order] : sortFallbackChain) {
        QVariant leftValue = roleToOrderedValue(left, role);
        QVariant rightValue = roleToOrderedValue(right, role);

        if (leftValue == rightValue) {
            continue;
        }

        const auto result = QVariant::compare(leftValue, rightValue);

        // Should not happen, but it's better to skip than assert
        if (!(result == QPartialOrdering::Less || result == QPartialOrdering::Greater)) {
            continue;
        }

        // Yes, there is a shorter but incomprehensible way of rewriting this
        return (order == Qt::AscendingOrder) ? (result == QPartialOrdering::Less) : (result == QPartialOrdering::Greater);
    }

    // They compared equal, so it is definitely not a "less than" relation
    return false;
}

std::shared_ptr<Category> ResourcesProxyModel::filteredCategory() const
{
    return m_filters.category;
}

void ResourcesProxyModel::setStateFilter(AbstractResource::State s)
{
    if (s != m_filters.state) {
        m_filters.state = s;
        invalidateFilter();
        Q_EMIT stateFilterChanged();
    }
}

AbstractResource::State ResourcesProxyModel::stateFilter() const
{
    return m_filters.state;
}

QString ResourcesProxyModel::mimeTypeFilter() const
{
    return m_filters.mimetype;
}

void ResourcesProxyModel::setMimeTypeFilter(const QString &mime)
{
    if (m_filters.mimetype != mime) {
        m_filters.mimetype = mime;
        invalidateFilter();
    }
}

QString ResourcesProxyModel::extends() const
{
    return m_filters.extends;
}

void ResourcesProxyModel::setExtends(const QString &extends)
{
    if (m_filters.extends != extends) {
        m_filters.extends = extends;
        invalidateFilter();
    }
}

void ResourcesProxyModel::setFilterMinimumState(bool filterMinimumState)
{
    if (filterMinimumState != m_filters.filterMinimumState) {
        m_filters.filterMinimumState = filterMinimumState;
        invalidateFilter();
        Q_EMIT filterMinimumStateChanged(m_filters.filterMinimumState);
    }
}

bool ResourcesProxyModel::filterMinimumState() const
{
    return m_filters.filterMinimumState;
}

QUrl ResourcesProxyModel::resourcesUrl() const
{
    return m_filters.resourceUrl;
}

void ResourcesProxyModel::setResourcesUrl(const QUrl &resourcesUrl)
{
    if (m_filters.resourceUrl != resourcesUrl) {
        m_filters.resourceUrl = resourcesUrl;
        invalidateFilter();
    }
}

bool ResourcesProxyModel::allBackends() const
{
    return m_filters.allBackends;
}

void ResourcesProxyModel::setAllBackends(bool allBackends)
{
    m_filters.allBackends = allBackends;
}

AbstractResourcesBackend *ResourcesProxyModel::backendFilter() const
{
    return m_filters.backend;
}

void ResourcesProxyModel::setBackendFilter(AbstractResourcesBackend *filtered)
{
    m_filters.backend = filtered;
}

QVariant ResourcesProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    const auto result = m_displayedResources[index.row()];
    return roleToValue(result, role);
}

QHash<int, int> ResourcesProxyModel::createRoleToProperty()
{
    QHash<int, int> ret;
    static const QMetaObject *m = &AbstractResource::staticMetaObject;
    for (const auto &[role, roleText] : ResourcesProxyModel::s_roles.asKeyValueRange()) {
        int propidx = roleText.isEmpty() ? -1 : m->indexOfProperty(roleText.constData());
        ret.insert(role, propidx);
    }
    return ret;
}

QVariant ResourcesProxyModel::roleToValue(const StreamResult &result, int role) const
{
    const auto resource = result.resource;
    switch (role) {
    case ApplicationRole:
        return QVariant::fromValue<QObject *>(resource);
    case RatingPointsRole: {
        const auto rating = resource->rating();
        return rating.ratingPoints();
    }
    case RatingRole: {
        const auto rating = resource->rating();
        return rating.rating();
    }
    case RatingCountRole: {
        const auto rating = resource->rating();
        return rating.ratingCount();
    }
    case SortableRatingRole: {
        const auto rating = resource->rating();
        return rating.sortableRating();
    }
    case SearchRelevanceRole: {
        qreal rating = roleToValue(result, SortableRatingRole).value<qreal>();

        qreal reverseDistance = 0;
        const QString name = resource->name();
        const auto words = QStringView(name).split(QLatin1Char(' '));
        for (QStringView word : words) {
            const qreal maxLength = std::max(word.length(), m_filters.search.length());
            reverseDistance =
                std::max(reverseDistance, (maxLength - std::min(reverseDistance, qreal(levenshteinDistance(word, m_filters.search)))) / maxLength * 10.0);
        }

        qreal exactMatch = 0.0;
        if (resource->name().toUpper() == m_filters.search.toUpper()) {
            exactMatch = 10.0;
        } else if (resource->name().contains(m_filters.search, Qt::CaseInsensitive)) {
            exactMatch = 5.0;
        }
        // Increase weight of sortScore from backends to prioritize their relevance calculations
        // sortScore typically ranges from 0-100, so we use it directly instead of dividing
        return qreal(result.sortScore) + rating + reverseDistance + exactMatch;
    }
    case Qt::DecorationRole:
    case Qt::DisplayRole:
    case Qt::StatusTipRole:
    case Qt::ToolTipRole:
        return {};
    default: {
        static QHash<int, int> s_roleToProperty = createRoleToProperty();
        const int propidx = s_roleToProperty[role];
        if (Q_UNLIKELY(propidx < 0)) {
            qCWarning(LIBDISCOVER_LOG) << "unknown role:" << role << roleNames()[role];
            return {};
        } else {
            return AbstractResource::staticMetaObject.property(propidx).read(resource);
        }
    }
    }
}

// Wraps roleToValue with additional features for sorting/comparison.
QVariant ResourcesProxyModel::roleToOrderedValue(const StreamResult &result, int role) const
{
    AbstractResource *resource = result.resource;
    switch (role) {
    case NameRole:
        return QVariant::fromValue(resource->nameSortKey());
    default:
        return roleToValue(result, role);
    }
}

bool ResourcesProxyModel::isSorted(const QVector<StreamResult> &results)
{
    auto last = results.constFirst();
    for (auto it = results.constBegin() + 1, itEnd = results.constEnd(); it != itEnd; ++it) {
        auto v1 = roleToValue(last, m_sortRole);
        auto v2 = roleToValue(*it, m_sortRole);
        if (!orderedLessThan(last, *it) && v1 != v2) {
            qCDebug(LIBDISCOVER_LOG) << "Faulty sort" << last.resource->name() << (*it).resource->name() << last.resource << (*it).resource;
            return false;
        }
        last = *it;
    }
    return true;
}

void ResourcesProxyModel::sortedInsertion(const QVector<StreamResult> &results)
{
    Q_ASSERT(results.size() == QSet(results.constBegin(), results.constEnd()).size());

    auto resultsCopy = results;
    Q_ASSERT(!resultsCopy.isEmpty());

    if (!m_filters.allBackends) {
        removeDuplicates(resultsCopy);
        if (resultsCopy.isEmpty()) {
            return;
        }
    }

    if (m_displayedResources.isEmpty()) {
        int rows = rowCount();
        beginInsertRows({}, rows, rows + resultsCopy.count() - 1);
        m_displayedResources += resultsCopy;
        endInsertRows();
        return;
    }

    for (const auto &result : std::as_const(resultsCopy)) {
        const auto finder = [this](const StreamResult &left, const StreamResult &right) {
            return orderedLessThan(left, right);
        };
        const auto it = std::upper_bound(m_displayedResources.constBegin(), m_displayedResources.constEnd(), result, finder);
        const auto prev = it - 1;
        const auto newIdx = it == m_displayedResources.constEnd() ? m_displayedResources.count() : (it - m_displayedResources.constBegin());

        if (prev != m_displayedResources.constEnd() && prev->resource == result.resource) {
            continue;
        }

        beginInsertRows({}, newIdx, newIdx);
        m_displayedResources.insert(newIdx, result);
        endInsertRows();
        // Q_ASSERT(isSorted(resultsCopy));
    }
}

void ResourcesProxyModel::refreshResource(AbstractResource *resource, const QVector<QByteArray> &properties)
{
    const auto row = indexOf(resource);
    if (row < 0) {
        return;
    }

    if (!m_filters.shouldFilter(resource)) {
        beginRemoveRows({}, row, row);
        m_displayedResources.removeAt(row);
        endRemoveRows();
        return;
    }

    const QModelIndex idx = index(row, 0);
    Q_ASSERT(idx.isValid());
    const auto roles = propertiesToRoles(properties);
    if (roles.contains(m_sortRole)) {
        beginRemoveRows({}, row, row);
        m_displayedResources.removeAt(row);
        endRemoveRows();

        sortedInsertion({{resource, 0}});
    } else {
        Q_EMIT dataChanged(idx, idx, roles);
    }
}

void ResourcesProxyModel::removeResource(AbstractResource *resource)
{
    const auto residx = indexOf(resource);
    if (residx < 0) {
        return;
    }
    beginRemoveRows({}, residx, residx);
    m_displayedResources.removeAt(residx);
    endRemoveRows();
}

void ResourcesProxyModel::refreshBackend(AbstractResourcesBackend *backend, const QVector<QByteArray> &properties)
{
    auto roles = propertiesToRoles(properties);
    const int count = m_displayedResources.count();

    bool found = false;

    for (int i = 0; i < count; ++i) {
        if (backend != m_displayedResources[i].resource->backend()) {
            continue;
        }

        int j = i + 1;
        while (j < count && backend == m_displayedResources[j].resource->backend()) {
            j++;
        }

        Q_EMIT dataChanged(index(i, 0), index(j - 1, 0), roles);
        i = j;
        found = true;
    }

    if (found && properties.contains(s_roles.value(m_sortRole))) {
        invalidateSorting();
    }
}

QVector<int> ResourcesProxyModel::propertiesToRoles(const QVector<QByteArray> &properties) const
{
    return kFilterTransform<QVector<int>>(properties, [this](const QByteArray &property) {
        const auto role = roleNames().key(property, -1);
        return role == -1 ? std::nullopt : std::optional(role);
    });
}

int ResourcesProxyModel::indexOf(AbstractResource *resource)
{
    return kIndexOf(m_displayedResources, [resource](const StreamResult &result) {
        return result.resource == resource;
    });
}

AbstractResource *ResourcesProxyModel::resourceAt(int row) const
{
    return m_displayedResources[row].resource;
}

bool ResourcesProxyModel::canFetchMore(const QModelIndex &parent) const
{
    Q_ASSERT(!parent.isValid());
    return m_currentStream;
}

void ResourcesProxyModel::fetchMore(const QModelIndex &parent)
{
    Q_ASSERT(!parent.isValid());
    if (m_currentStream) {
        Q_EMIT m_currentStream->fetchMore();
    }
}

ResourcesCount ResourcesProxyModel::count() const
{
    const int rows = rowCount();
    if (isBusy()) {
        // We return an empty string because it's evidently confusing
        if (rows == 0) {
            return ResourcesCount();
        }

        // We convert rows=1234 into round=1000
        const int round = std::pow(10, std::floor(std::log10(rows)));
        if (round >= 1) {
            const int roughCount = (rows / round) * round;
            const auto string = i18nc("an approximation number, like 3000+", "%1+", roughCount);
            return ResourcesCount(roughCount, string);
        }
    }
    return ResourcesCount(rows);
}

ResourcesCount::ResourcesCount()
    : m_valid(false)
    , m_number(0)
    , m_string()
{
}

ResourcesCount::ResourcesCount(int number)
    : m_valid(true)
    , m_number(number)
    , m_string(QString::number(number))
{
}

ResourcesCount::ResourcesCount(int number, const QString &string)
    : m_valid(true)
    , m_number(number)
    , m_string(string)
{
}

#include "moc_ResourcesProxyModel.cpp"
