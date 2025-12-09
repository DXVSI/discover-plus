/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractResourcesBackend.h"
#include "Category/Category.h"
#include "libdiscover_debug.h"
#include "utils.h"
#include <KLocalizedString>
#include <QHash>
#include <QMetaObject>
#include <QMetaProperty>
#include <QTimer>

QDebug operator<<(QDebug debug, const AbstractResourcesBackend::Filters &filters)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Filters(";
    if (filters.category)
        debug.nospace() << "category: " << filters.category.get() << ',';
    if (filters.state)
        debug.nospace() << "state: " << filters.state << ',';
    if (!filters.mimetype.isEmpty())
        debug.nospace() << "mimetype: " << filters.mimetype << ',';
    if (!filters.search.isEmpty())
        debug.nospace() << "search: " << filters.search << ',';
    if (!filters.extends.isEmpty())
        debug.nospace() << "extends:" << filters.extends << ',';
    if (!filters.origin.isEmpty())
        debug.nospace() << "origin:" << filters.origin << ',';
    if (!filters.resourceUrl.isEmpty())
        debug.nospace() << "resourceUrl:" << filters.resourceUrl << ',';
    debug.nospace() << ')';

    return debug;
}

QDebug operator<<(QDebug debug, const StreamResult &sss)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "StreamResult(" << sss.resource << ", " << sss.sortScore << ')';
    return debug;
}

ResultsStream::ResultsStream(const QString &objectName, const QVector<StreamResult> &resources)
    : ResultsStream(objectName)
{
    Q_ASSERT(!kContains(resources, [](const StreamResult &result) {
        return result.resource == nullptr;
    }));
    QTimer::singleShot(0, this, [resources, this]() {
        if (!resources.isEmpty())
            Q_EMIT resourcesFound(resources);
        finish();
    });
}

ResultsStream::ResultsStream(const QString &objectName)
{
    setObjectName(objectName);
    QTimer::singleShot(5000, this, [objectName]() {
        qCDebug(LIBDISCOVER_LOG) << "stream took really long" << objectName;
    });
}

ResultsStream::~ResultsStream()
{
}

void ResultsStream::finish()
{
    deleteLater();
}

AbstractResourcesBackend::AbstractResourcesBackend(QObject *parent)
    : QObject(parent)
{
    connect(this, &AbstractResourcesBackend::contentsChanged, this, &AbstractResourcesBackend::fetchingUpdatesProgressChanged);
}

Transaction *AbstractResourcesBackend::installApplication(AbstractResource *app)
{
    return installApplication(app, AddonList());
}

void AbstractResourcesBackend::setName(const QString &name)
{
    m_name = name;
}

QString AbstractResourcesBackend::name() const
{
    return m_name;
}

void AbstractResourcesBackend::emitRatingsReady()
{
    Q_EMIT allDataChanged({"rating", "ratingPoints", "ratingCount", "sortableRating"});
}

bool AbstractResourcesBackend::Filters::shouldFilter(AbstractResource *resourse) const
{
    Q_ASSERT(resourse);

    if (!extends.isEmpty() && !resourse->extends().contains(extends)) {
        return false;
    }

    if (!origin.isEmpty()) {
        const QString resourceOrigin = resourse->origin();
        // Use contains for partial matching (e.g., "rpmfusion" matches "rpmfusion-free-updates")
        // But exact match for special cases like "COPR"
        if (origin == QStringLiteral("COPR")) {
            if (resourceOrigin != origin) {
                return false;
            }
        } else {
            if (!resourceOrigin.contains(origin, Qt::CaseInsensitive)) {
                return false;
            }
        }
    }

    bool stateCheck = filterMinimumState ? (resourse->state() < state) : (resourse->state() != state);
    if (stateCheck) {
        return false;
    }

    if (!mimetype.isEmpty() && !resourse->mimetypes().contains(mimetype)) {
        return false;
    }

    // Check category match if category filter is set
    if (category) {
        // For top-level categories (like "All Applications"),
        // don't filter by category - show all resources from all backends
        // This allows PackageKit packages without AppStream metadata to be visible
        bool isTopLevelCategory = !category->parentCategory();
        if (isTopLevelCategory) {
            return true;
        }

        // For subcategories, apply category filter
        if (!resourse->categoryMatches(category)) {
            return false;
        }
    }

    return true;
}

void AbstractResourcesBackend::Filters::filterJustInCase(QVector<AbstractResource *> &resources) const
{
    for (auto it = resources.begin(); it != resources.end();) {
        if (shouldFilter(*it)) {
            ++it;
        } else {
            it = resources.erase(it);
        }
    }
}

void AbstractResourcesBackend::Filters::filterJustInCase(QVector<StreamResult> &results) const
{
    for (auto it = results.begin(); it != results.end();) {
        if (shouldFilter(it->resource)) {
            ++it;
        } else {
            it = results.erase(it);
        }
    }
}

bool AbstractResourcesBackend::extends(const QString & /*id*/) const
{
    return false;
}

uint AbstractResourcesBackend::fetchingUpdatesProgressWeight() const
{
    return 1;
}

InlineMessage *AbstractResourcesBackend::explainDysfunction() const
{
    return new InlineMessage(InlineMessage::Error, QStringLiteral("network-disconnect"), i18n("Please verify Internet connectivity"));
}

#include "moc_AbstractResourcesBackend.cpp"
