/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"
#include "libdiscover_debug.h"
#include <Category/CategoryModel.h>
#include <KFormat>
#include <KLocalizedString>
#include <KShell>
#include <QList>
#include <QProcess>
#include <QString>
#include <ReviewsBackend/AbstractReviewsBackend.h>

AbstractResource::AbstractResource(AbstractResourcesBackend *parent)
    : QObject(parent)
{
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::sizeChanged);
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::versionsChanged);
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::reportNewState);
}

AbstractResource::~AbstractResource() = default;

QUrl AbstractResource::homepage()
{
    return QUrl();
}

QUrl AbstractResource::helpURL()
{
    return QUrl();
}

QUrl AbstractResource::bugURL()
{
    return QUrl();
}

QUrl AbstractResource::donationURL()
{
    return QUrl();
}

QUrl AbstractResource::contributeURL()
{
    return {};
}

void AbstractResource::addMetadata(const QLatin1StringView &key, const QJsonValue &value)
{
    m_metadata.insert(key, value);
}

QJsonValue AbstractResource::getMetadata(const QLatin1StringView &key)
{
    return m_metadata.value(key);
}

bool AbstractResource::canUpgrade()
{
    return state() == Upgradeable;
}

bool AbstractResource::isInstalled()
{
    return state() >= Installed;
}

void AbstractResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched({});
}

QStringList AbstractResource::mimetypes() const
{
    return QStringList();
}

AbstractResourcesBackend *AbstractResource::backend() const
{
    return static_cast<AbstractResourcesBackend *>(parent());
}

QString AbstractResource::status()
{
    switch (state()) {
    case Broken:
        return i18n("Broken");
    case None:
        return i18n("Available");
    case Installed:
        return i18n("Installed");
    case Upgradeable:
        return i18n("Upgradeable");
    }
    return QString();
}

QString AbstractResource::sizeDescription()
{
    return KFormat().formatByteSize(size());
}

QCollatorSortKey AbstractResource::nameSortKey()
{
    if (!m_collatorKey.has_value()) {
        m_collatorKey = QCollator::defaultSortKey(name());
    }
    return m_collatorKey.value();
}

Rating AbstractResource::rating() const
{
    if (auto reviewsBackend = backend()->reviewsBackend()) {
        return reviewsBackend->ratingForApplication(const_cast<AbstractResource *>(this));
    }
    return {};
}

QStringList AbstractResource::extends() const
{
    return {};
}

QString AbstractResource::appstreamId() const
{
    return {};
}

void AbstractResource::reportNewState()
{
    static const QVector<QByteArray> properties = {
        "state",
        "status",
        "canUpgrade",
        "size",
        "sizeDescription",
        "installedVersion",
        "availableVersion",
    };
    Q_EMIT backend()->resourcesChanged(this, properties);
}

static bool shouldFilter(AbstractResource *resource, const CategoryFilter &filter)
{
    bool ret = true;
    switch (filter.type) {
    case CategoryFilter::CategoryNameFilter:
        ret = resource->hasCategory(std::get<QString>(filter.value));
        break;
    case CategoryFilter::PkgSectionFilter:
        ret = resource->section() == std::get<QString>(filter.value);
        break;
    case CategoryFilter::PkgWildcardFilter: {
        QString wildcard = std::get<QString>(filter.value);
        wildcard.remove(QLatin1Char('*'));
        ret = resource->packageName().contains(wildcard);
        break;
    }
    case CategoryFilter::AppstreamIdWildcardFilter: {
        QString wildcard = std::get<QString>(filter.value);
        wildcard.remove(QLatin1Char('*'));
        ret = resource->appstreamId().contains(wildcard);
        break;
    }
    case CategoryFilter::PkgNameFilter: // Only useful in the not filters
        ret = resource->packageName() == std::get<QString>(filter.value);
        break;
    case CategoryFilter::AndFilter: {
        const auto filters = std::get<QList<CategoryFilter>>(filter.value);
        ret = std::all_of(filters.begin(), filters.end(), [resource](const CategoryFilter &filter) {
            return shouldFilter(resource, filter);
        });
        break;
    }
    case CategoryFilter::OrFilter: {
        const auto filters = std::get<QList<CategoryFilter>>(filter.value);
        ret = std::any_of(filters.begin(), filters.end(), [resource](const CategoryFilter &filter) {
            return shouldFilter(resource, filter);
        });
        break;
    }
    case CategoryFilter::NotFilter: {
        const auto filters = std::get<QList<CategoryFilter>>(filter.value);
        ret = !std::any_of(filters.begin(), filters.end(), [resource](const CategoryFilter &filter) {
            return shouldFilter(resource, filter);
        });
        break;
    }
    }
    return ret;
}

bool AbstractResource::categoryMatches(const std::shared_ptr<Category> &cat)
{
    return shouldFilter(this, cat->filter());
}

static QSet<std::shared_ptr<Category>> walkCategories(AbstractResource *resource, const QList<std::shared_ptr<Category>> &categories)
{
    QSet<std::shared_ptr<Category>> ret;
    for (const auto &category : categories) {
        if (resource->categoryMatches(category)) {
            const auto subcats = walkCategories(resource, category->subCategories());
            if (subcats.isEmpty()) {
                ret += category;
            } else {
                ret += subcats;
            }
        }
    }

    return ret;
}

QSet<std::shared_ptr<Category>> AbstractResource::categoryObjects(const QList<std::shared_ptr<Category>> &categories) const
{
    return walkCategories(const_cast<AbstractResource *>(this), categories);
}

QUrl AbstractResource::url() const
{
    const QString asid = appstreamId();
    return asid.isEmpty() ? QUrl(backend()->name() + QStringLiteral("://") + packageName()) : QUrl(QStringLiteral("appstream://") + asid);
}

QString AbstractResource::displayOrigin() const
{
    return origin();
}

QString AbstractResource::executeLabel() const
{
    return i18n("Launch");
}

QString AbstractResource::upgradeText() const
{
    const auto installed = installedVersion();
    const auto available = availableVersion();

    if (installed == available) {
        // Update of the same version; show when old and new are
        // the same (common with Flatpak runtimes)
        return i18nc("@info 'Refresh' is used as a noun here, and %1 is an app's version number", "Refresh of version %1", available);
    } else if (!installed.isEmpty() && !available.isEmpty()) {
        // Old and new version numbers
        // This thing with \u009C is a fancy feature in QML text handling:
        // when the string will be elided, it shows the string after
        // the last \u009C. This allows us to show a smaller string
        // when there's now enough room

        // All of this is mostly for the benefit of KDE Neon users,
        // since the version strings there are really really long
        return i18nc("Do not translate or alter \\u009C", "%1 → %2\u009C%1 → %2\u009C%2", installed, available);
    } else {
        // Available version only, for when the installed version
        // isn't available for some reason
        return available;
    }
}

QString AbstractResource::versionString()
{
    const QString version = isInstalled() ? installedVersion() : availableVersion();
    if (version.isEmpty()) {
        return {};
    }

    return version;
}

QString AbstractResource::contentRatingDescription() const
{
    return {};
}

uint AbstractResource::contentRatingMinimumAge() const
{
    return 0;
}

QStringList AbstractResource::topObjects() const
{
    return {};
}

QStringList AbstractResource::bottomObjects() const
{
    return {};
}

bool AbstractResource::hasResolvedIcon() const
{
    return true;
}

void AbstractResource::resolveIcon()
{
}

QString AbstractResource::disambiguatedOrigin() const
{
    return origin();
}

#include "moc_AbstractResource.cpp"
