/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QVector>
#include <variant>

#include "discovercommon_export.h"

class QXmlStreamReader;
class QTimer;

class CategoryFilter
{
    Q_GADGET
public:
    enum FilterType {
        CategoryNameFilter,
        PkgSectionFilter,
        PkgWildcardFilter,
        PkgNameFilter,
        AppstreamIdWildcardFilter,
        OrFilter,
        AndFilter,
        NotFilter,
    };
    Q_ENUM(FilterType)

    FilterType type;
    std::variant<QString, QList<CategoryFilter>> value;

    bool operator==(const CategoryFilter &other) const;
    bool operator!=(const CategoryFilter &other) const
    {
        return !operator==(other);
    }
};

class DISCOVERCOMMON_EXPORT Category : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QVariantList subcategories READ subCategoriesVariant NOTIFY subCategoriesChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)

    // Whether to apply localization during parsing.
    enum class Localization {
        No, /* < Don't apply localization (i.e. use strings from xml verbatim) */
        Yes, /* < Route xml strings through i18n() */
        Force, /* < Use localization even when it'd break something (only use this for tests!) */
    };

    // The type of package this category contains.
    enum class Type {
        Addon, /* < Category contains Plasma addons */
        Driver, /* < Category contains hardware drivers */
        Font, /* < Category contains fonts */
        Package, /* < Category contains regular packages */
    };

    explicit Category(QSet<QString> pluginNames, const std::shared_ptr<Category> &parent = {});

    Category(const QString &name,
             const QString &iconName,
             const CategoryFilter &filters,
             const QSet<QString> &pluginName,
             const QList<std::shared_ptr<Category>> &subCategories,
             Type type = Type::Package);
    ~Category() override;

    QString name() const;
    // You should never attempt to change the name of anything that is not a leaf category
    // as the results could be potentially detrimental to the function of the category filters
    void setName(const QString &name);
    QString icon() const;
    void setFilter(const CategoryFilter &filter);
    CategoryFilter filter() const;
    const QList<std::shared_ptr<Category>> &subCategories() const;
    QVariantList subCategoriesVariant() const;

    static void sortCategories(QList<std::shared_ptr<Category>> &cats);
    static void addSubcategory(QList<std::shared_ptr<Category>> &cats, const std::shared_ptr<Category> &cat);
    /**
     * Add a subcategory to this category. This function should only
     * be used during the initialisation stage, before adding the local
     * root category to the global root category model.
     */
    void addSubcategory(const std::shared_ptr<Category> &cat);
    void parseData(const QString &path, QXmlStreamReader *xml, Localization localization);
    bool blacklistPlugins(const QSet<QString> &pluginName);
    Type type() const
    {
        return m_type;
    }
    qint8 priority() const
    {
        return m_priority;
    }
    bool matchesCategoryName(const QString &name) const;

    /**
     * @returns whether the category ought to be listed.
     *
     * It can be disabled passing <OnlyShowIn> with a vale differing from
     * XDG_CURRENT_DESKTOP environment variable.
     */
    bool isVisible() const
    {
        return m_visible;
    }

    Q_SCRIPTABLE bool contains(const std::shared_ptr<Category> &cat) const;
    Q_SCRIPTABLE bool contains(const QVariantList &cats) const;

    static bool categoryLessThan(const std::shared_ptr<Category> &c1, const std::shared_ptr<Category> &c2);
    static bool blacklistPluginsInVector(const QSet<QString> &pluginNames, QList<std::shared_ptr<Category>> &subCategories);

    QStringList involvedCategories() const;
    QString untranslatedName() const
    {
        return m_untranslatedName;
    }
    [[nodiscard]] static std::optional<QString> duplicatedNamesAsStringNested(const QList<std::shared_ptr<Category>> &categories);

    std::shared_ptr<Category> parentCategory() const
    {
        return m_parentCategory;
    }

    void hide()
    {
        if (m_hide) {
            return;
        }
        m_hide = true;
        m_visible = false;
        Q_EMIT visibleChanged();
    }

Q_SIGNALS:
    void subCategoriesChanged();
    void nameChanged();
    void visibleChanged();

private:
    // disable the QObject parent business
    QObject *parent() const;
    void setParent(QObject *);

    void setNameMembers(const QString &name, Localization localization);
    QString m_name;
    QString m_untranslatedName;
    QString m_iconString;
    CategoryFilter m_filter;
    QList<std::shared_ptr<Category>> m_subCategories;

    CategoryFilter parseIncludes(QXmlStreamReader *xml);
    QSet<QString> m_plugins;
    Type m_type;
    bool m_hide = false;
    qint8 m_priority = 0;
    QTimer *m_subCategoriesChanged;
    bool m_visible = true;
    std::shared_ptr<Category> m_parentCategory;
};

Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)
Q_DECLARE_METATYPE(std::shared_ptr<Category>)

DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const std::shared_ptr<Category> &category);
