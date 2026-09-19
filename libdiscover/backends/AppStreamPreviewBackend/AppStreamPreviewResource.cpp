/*
 *   SPDX-FileCopyrightText: 2025 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamPreviewResource.h"

#include <AppStreamQt/developer.h>

#include <appstream/AppStreamUtils.h>

const QStringList AppStreamPreviewResource::s_topObjects({
    QStringLiteral("qrc:/qml/AppStreamPreviewMessage.qml"),
});

AppStreamPreviewResource::AppStreamPreviewResource(const AppStream::Component &component, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_component(component)
{
}

QString AppStreamPreviewResource::packageName() const
{
    return m_component.id();
}

QString AppStreamPreviewResource::name() const
{
    return m_component.name();
}

QString AppStreamPreviewResource::comment()
{
    const auto summary = m_component.summary();
    if (!summary.isEmpty()) {
        return summary;
    }

    return QString();
}

QVariant AppStreamPreviewResource::icon() const
{
    return QLatin1String("application-x-executable");
}

bool AppStreamPreviewResource::canExecute() const
{
    return false;
}

void AppStreamPreviewResource::invokeApplication() const
{
}

AbstractResource::State AppStreamPreviewResource::state()
{
    return State::Installed;
}

bool AppStreamPreviewResource::hasCategory(const QString &category) const
{
    if (m_component.kind() != AppStream::Component::KindAddon && category == QStringLiteral("Application"))
        return true;
    return m_component.hasCategory(category);
}

AbstractResource::Type AppStreamPreviewResource::type() const
{
    switch (m_component.kind()) {
    case AppStream::Component::KindAddon:
        return AbstractResource::Addon;
    case AppStream::Component::KindRuntime:
        return AbstractResource::ApplicationSupport;
    case AppStream::Component::KindOperatingSystem:
        return AbstractResource::System;
    default:
        return AbstractResource::Application;
    }
}

quint64 AppStreamPreviewResource::size()
{
    return 0;
}

QJsonArray AppStreamPreviewResource::licenses()
{
    return AppStreamUtils::licenses(m_component);
}

QString AppStreamPreviewResource::installedVersion() const
{
    return QStringLiteral("1.0");
}

QString AppStreamPreviewResource::availableVersion() const
{
    return QStringLiteral("2.0");
}

QString AppStreamPreviewResource::longDescription()
{
    return m_component.description();
}

QString AppStreamPreviewResource::origin() const
{
    return m_component.origin();
}

QString AppStreamPreviewResource::section()
{
    return QString();
}

QString AppStreamPreviewResource::author() const
{
    QString name = m_component.developer().name();

    if (name.isEmpty()) {
        name = m_component.projectGroup();
    }

    return name;
}

QList<PackageState> AppStreamPreviewResource::addonsInformation()
{
    return {};
}

QString AppStreamPreviewResource::sourceIcon() const
{
    return QString();
}

QDate AppStreamPreviewResource::releaseDate() const
{
    if (const auto releases = m_component.releasesPlain(); !releases.isEmpty()) {
        auto release = releases.indexSafe(0);
        if (release) {
            return release->timestamp().date();
        }
    }

    return {};
}

void AppStreamPreviewResource::fetchChangelog()
{
    QString changelog = AppStreamUtils::changelogToHtml(m_component);
    Q_EMIT changelogFetched(changelog);
}

void AppStreamPreviewResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched(AppStreamUtils::fetchScreenshots(m_component));
}

QUrl AppStreamPreviewResource::homepage()
{
    return m_component.url(AppStream::Component::UrlKindHomepage);
}

QUrl AppStreamPreviewResource::helpURL()
{
    return m_component.url(AppStream::Component::UrlKindHelp);
}

QUrl AppStreamPreviewResource::bugURL()
{
    return m_component.url(AppStream::Component::UrlKindBugtracker);
}

QUrl AppStreamPreviewResource::donationURL()
{
    return m_component.url(AppStream::Component::UrlKindDonation);
}

QUrl AppStreamPreviewResource::contributeURL()
{
    return m_component.url(AppStream::Component::UrlKindContribute);
}

QString AppStreamPreviewResource::versionString()
{
    if (m_component.releasesPlain().isEmpty()) {
        return QString();
    }

    const auto release = m_component.releasesPlain().indexSafe(0).value();
    return release.version();
}

QString AppStreamPreviewResource::contentRatingDescription() const
{
    return AppStreamUtils::contentRatingDescription(m_component);
}

uint AppStreamPreviewResource::contentRatingMinimumAge() const
{
    return AppStreamUtils::contentRatingMinimumAge(m_component);
}

bool AppStreamPreviewResource::isRemovable() const
{
    return false;
}

QString AppStreamPreviewResource::appstreamId() const
{
    return m_component.id();
}

QUrl AppStreamPreviewResource::url() const
{
    return QUrl();
}

QStringList AppStreamPreviewResource::topObjects() const
{
    static QStringList s_objects{QStringLiteral("qrc:/qml/AppStreamPreviewMessage.qml")};
    return s_objects;
}
