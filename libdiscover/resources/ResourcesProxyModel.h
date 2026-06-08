/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QQmlParserStatus>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <Category/Category.h>

#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"
#include "discovercommon_export.h"

class AggregatedResultsStream;

/*
 * This class encapsulates model's counter. The string property is a
 * pre-formatted number possibly with extra symbols for a "rough" appearance.
 * If counter is not valid, accessing its number and string is pointless.
 */
class DISCOVERCOMMON_EXPORT ResourcesCount
{
    Q_GADGET
    Q_PROPERTY(bool valid MEMBER m_valid CONSTANT FINAL)
    Q_PROPERTY(int number MEMBER m_number CONSTANT FINAL)
    Q_PROPERTY(QString string MEMBER m_string CONSTANT FINAL)

public:
    explicit ResourcesCount();
    explicit ResourcesCount(int number);
    explicit ResourcesCount(int number, const QString &string);

private:
    bool m_valid;
    int m_number;
    QString m_string;
};

class DISCOVERCOMMON_EXPORT ResourcesProxyModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Roles sortRole READ sortRole WRITE setSortRole NOTIFY sortRoleChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(std::shared_ptr<Category> filteredCategory READ filteredCategory WRITE setFiltersFromCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString filteredCategoryName READ filteredCategoryName WRITE setFilteredCategoryName NOTIFY categoryChanged)
    Q_PROPERTY(QString originFilter READ originFilter WRITE setOriginFilter)
    Q_PROPERTY(AbstractResource::State stateFilter READ stateFilter WRITE setStateFilter NOTIFY stateFilterChanged)
    Q_PROPERTY(bool filterMinimumState READ filterMinimumState WRITE setFilterMinimumState NOTIFY filterMinimumStateChanged)
    Q_PROPERTY(QString mimeTypeFilter READ mimeTypeFilter WRITE setMimeTypeFilter)
    Q_PROPERTY(AbstractResourcesBackend *backendFilter READ backendFilter WRITE setBackendFilter)
    Q_PROPERTY(QString search READ lastSearch WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(QUrl resourcesUrl READ resourcesUrl WRITE setResourcesUrl NOTIFY resourcesUrlChanged)
    Q_PROPERTY(QString extending READ extends WRITE setExtends)
    Q_PROPERTY(bool allBackends READ allBackends WRITE setAllBackends)
    Q_PROPERTY(QVariantList subcategories READ subcategories NOTIFY subcategoriesChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(ResourcesCount count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(bool categorize READ categorize WRITE setCategorize NOTIFY categorizeChanged)
public:
    explicit ResourcesProxyModel(QObject *parent = nullptr);
    // Make sure the role values don't change since some get stored in config files.
    enum Roles {
        NameRole = Qt::UserRole,
        IconRole,
        CommentRole,
        StateRole,
        RatingRole,
        RatingPointsRole,
        RatingCountRole,
        SortableRatingRole,
        InstalledRole,
        ApplicationRole,
        OriginRole,
        DisplayOriginRole,
        CanUpgrade,
        PackageNameRole,
        CategoryRole,
        CategoryNameRole,
        SectionRole,
        MimeTypes,
        SizeRole,
        LongDescriptionRole,
        SourceIconRole,
        ReleaseDateRole,
        // This is better that's always the last value as this one should be never saved to disk
        SearchRelevanceRole
    };
    Q_ENUM(Roles)

    QHash<int, QByteArray> roleNames() const override;

    void setSearch(const QString &text);
    QString lastSearch() const;
    void setOriginFilter(const QString &origin);
    QString originFilter() const;
    void setFiltersFromCategory(const std::shared_ptr<Category> &category);
    void setStateFilter(AbstractResource::State s);
    AbstractResource::State stateFilter() const;
    void setSortRole(Roles sortRole);
    Roles sortRole() const
    {
        return m_sortRole;
    }
    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const
    {
        return m_sortOrder;
    }
    void setFilterMinimumState(bool filterMinimumState);
    bool filterMinimumState() const;

    std::shared_ptr<Category> filteredCategory() const;
    QString filteredCategoryName() const;
    void setFilteredCategoryName(const QString &categoryName);

    QString mimeTypeFilter() const;
    void setMimeTypeFilter(const QString &mime);

    QString extends() const;
    void setExtends(const QString &extends);

    QUrl resourcesUrl() const;
    void setResourcesUrl(const QUrl &resourcesUrl);

    bool allBackends() const;
    void setAllBackends(bool allBackends);

    AbstractResourcesBackend *backendFilter() const;
    void setBackendFilter(AbstractResourcesBackend *filtered);

    bool categorize() const;
    void setCategorize(bool categorize);

    QVariantList subcategories() const;

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = {}) const override;

    Q_SCRIPTABLE int indexOf(AbstractResource *res);
    Q_SCRIPTABLE AbstractResource *resourceAt(int row) const;

    bool isBusy() const
    {
        return m_currentStream != nullptr;
    }

    bool orderedLessThan(const StreamResult &left, const StreamResult &right) const;
    Q_SCRIPTABLE void invalidateFilter();
    void invalidateSorting();

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    void classBegin() override
    {
    }
    void componentComplete() override;

    ResourcesCount count() const;

private Q_SLOTS:
    void refreshBackend(AbstractResourcesBackend *backend, const QVector<QByteArray> &properties);
    void refreshResource(AbstractResource *resource, const QVector<QByteArray> &properties);
    void removeResource(AbstractResource *resource);

private:
    void sortedInsertion(const QVector<StreamResult> &results);
    QVariant roleToValue(const StreamResult &result, int role) const;
    QVariant roleToOrderedValue(const StreamResult &result, int role) const;

    QVector<int> propertiesToRoles(const QVector<QByteArray> &properties) const;
    void addResources(const QVector<StreamResult> &results);
    void fetchSubcategories();
    void removeDuplicates(QVector<StreamResult> &newResources);
    bool isSorted(const QVector<StreamResult> &results);

    Roles m_sortRole;
    Qt::SortOrder m_sortOrder;

    bool m_setup = false;
    QString m_categoryName;

    AbstractResourcesBackend::Filters m_filters;
    QVariantList m_subcategories;

    QVector<StreamResult> m_displayedResources;
    static const QHash<int, QByteArray> s_roles;
    static QHash<int, int> createRoleToProperty();
    ResultsStream *m_currentStream;
    QTimer m_countTimer;
    bool m_categorize = false;

Q_SIGNALS:
    void busyChanged();
    void sortRoleChanged(int sortRole);
    void sortOrderChanged(Qt::SortOrder order);
    void categoryChanged();
    void stateFilterChanged();
    void searchChanged(const QString &search);
    void subcategoriesChanged(const QVariantList &subcategories);
    void resourcesUrlChanged(const QUrl &url);
    void countChanged();
    void filterMinimumStateChanged(bool filterMinimumState);
    void categorizeChanged();
};
