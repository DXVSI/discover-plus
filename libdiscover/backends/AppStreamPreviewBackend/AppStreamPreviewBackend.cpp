/*
 *   SPDX-FileCopyrightText: 2025 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamPreviewBackend.h"

#include <AppStreamQt/metadata.h>

#include "AppStreamPreviewResource.h"

using namespace Qt::Literals::StringLiterals;

DISCOVER_BACKEND_PLUGIN(AppStreamPreviewBackend)

AppStreamPreviewBackend::AppStreamPreviewBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new AppStreamPreviewBackendUpdater(this))
    , m_reviews(OdrsReviewsBackend::global())
{
}

ResultsStream *AppStreamPreviewBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (!filter.resourceUrl.isLocalFile()) {
        return new ResultsStream(u"AppStreamPreview"_s, {});
    }

    const auto path = filter.resourceUrl.path();

    if (!path.endsWith(u".metainfo.xml"_s) && !path.endsWith(u".appdata.xml"_s)) {
        return new ResultsStream(u"AppStreamPreview"_s, {});
    }

    AppStream::Metadata metadata;
    metadata.setFormatStyle(AppStream::Metadata::FormatStyleMetainfo);
    AppStream::Metadata::MetadataError error = metadata.parseFile(path, AppStream::Metadata::FormatKindXml);
    if (error != AppStream::Metadata::MetadataErrorNoError) {
        qWarning() << "Failed to parse appstream metadata: " << error;
        return new ResultsStream(u"AppStreamPreview"_s, {});
    }

    auto resource = new AppStreamPreviewResource(metadata.component(), this);

    return new ResultsStream(u"AppStreamPreview"_s, {resource});
}

bool AppStreamPreviewBackend::isValid() const
{
    return true;
}

AbstractBackendUpdater *AppStreamPreviewBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *AppStreamPreviewBackend::reviewsBackend() const
{
    return m_reviews.data();
}

int AppStreamPreviewBackend::updatesCount() const
{
    return 0;
}

QString AppStreamPreviewBackend::displayName() const
{
    return QStringLiteral("AppStreamPreview");
}

int AppStreamPreviewBackend::fetchingUpdatesProgress() const
{
    return 100;
}

Transaction *AppStreamPreviewBackend::installApplication(AbstractResource *app)
{
    Q_UNUSED(app)

    return NULL;
}

Transaction *AppStreamPreviewBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(app)
    Q_UNUSED(addons)

    return NULL;
}

Transaction *AppStreamPreviewBackend::removeApplication(AbstractResource *app)
{
    Q_UNUSED(app)

    return NULL;
}

void AppStreamPreviewBackend::checkForUpdates()
{
}

#include "AppStreamPreviewBackend.moc"
