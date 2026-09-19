/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamIntegration.h"

#include <AppStreamQt/systeminfo.h>
#include <AppStreamQt/utils.h>
#include <AppStreamQt/version.h>
#include <QDebug>

#include "discover-appstream-settings.h"

using namespace Qt::StringLiterals;

AppStreamIntegration *AppStreamIntegration::global()
{
    static AppStreamIntegration *var = nullptr;
    if (!var) {
        var = new AppStreamIntegration;
    }

    return var;
}

std::optional<AppStream::Release> AppStreamIntegration::getDistroUpgrade(AppStream::Pool *pool, std::optional<QString> IdOverride)
{
    QString distroId;
    if (IdOverride) {
        distroId = *IdOverride;
    } else {
        distroId = AppStream::SystemInfo::currentDistroComponentId();
    }

    // Look at releases to see if we have a new major version available.
    const auto distroComponents = pool->componentsById(distroId);
    if (distroComponents.isEmpty()) {
        qWarning() << "AppStreamIntegration: No distro component found for" << distroId;
        return std::nullopt;
    }

    // Settings to control if pre-release and development releases are offered
    // for updates. Those options are not exposed to users via a graphical
    // interface and must be manually set in the configuration file.
    //
    // For example for Fedora:
    // - the development release is always Rawhide
    // - the pre-release is a version that has been branched from Rawhide but is
    //   not stable yet. For example Fedora 40 Beta
    //
    // In Appstream metadata terms:
    // - Development releases are specified as KindSnapshot
    // - Pre-releases are specified as KindDevelopment
    AppstreamSettings settings;
    bool allowPreRelease = settings.allowPreRelease();
    bool allowDevelopmentRelease = settings.allowDevelopmentRelease();

    QString currentVersion = osRelease()->versionId();

    // skip if current version is unknown
    if (currentVersion.isEmpty()) {
        qWarning() << "AppStreamIntegration: Cannot detect current distro version for" << distroId;
        return std::nullopt;
    }

    std::optional<AppStream::Release> nextRelease;
    std::optional<AppStream::Release> developmentRelease;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const AppStream::Component &dc : distroComponents) {
        const auto releases = dc.releasesPlain().entries();
        for (const auto &r : releases) {
            // Keep the development release for later only if requested
            if (r.kind() == AppStream::Release::KindSnapshot) {
                if (allowDevelopmentRelease) {
                    developmentRelease = r;
                }
                continue;
            }

            // Skip releases that are not stable
            if (r.kind() != AppStream::Release::KindStable) {
                // Skip pre-releases if not requested
                if (!(r.kind() == AppStream::Release::KindDevelopment && allowPreRelease)) {
                    continue;
                }
            }

            // Skip stable and development releases with a start date in the future if we did not enable pre-releases
            if (r.timestamp() > now && !allowPreRelease) {
                continue;
            }

            // Look at the potentially new version
            const QString newVersion = r.version();
            if (AppStream::Utils::vercmpSimple(newVersion, currentVersion) > 0) {
                if (!nextRelease) {
                    // No other newer version found yet so let's pick this one
                    nextRelease = r;
                    qInfo() << "Found new major release:" << newVersion;
                } else if (AppStream::Utils::vercmpSimple(nextRelease->version(), newVersion) > 0) {
                    // We only offer updating to the very next major release so
                    // we pick the smallest of all the newest versions
                    nextRelease = r;
                    qInfo() << "Found a closer new major release:" << newVersion;
                }
            }
        }
    }

    // If:
    // - we did not find a major upcoming release
    // - we are allowed to look at development releases
    // - we found a development release
    // then offer it.
    if (!nextRelease && allowDevelopmentRelease && developmentRelease) {
        nextRelease = developmentRelease;
    }

    return nextRelease;
}

#include "moc_AppStreamIntegration.cpp"
