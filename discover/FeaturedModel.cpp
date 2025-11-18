/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FeaturedModel.h"

#include "discover_debug.h"
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KSharedConfig>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QtGlobal>

#include <resources/ResourcesModel.h>
#include <resources/StoredResultsStream.h>
#include <utils.h>

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC(QString, featuredCache)

static QUrl featuredURL()
{
    QString config = QStringLiteral("/usr/share/discover/featuredurlrc");
    KConfigGroup grp(KSharedConfig::openConfig(config), u"Software"_s);
    if (grp.hasKey("FeaturedListingURL")) {
        return grp.readEntry("FeaturedListingURL", QUrl());
    }
    const auto baseURL = QLatin1StringView("https://autoconfig.kde.org/discover/");

    static const bool isMobile = QByteArrayList{"1", "true"}.contains(qgetenv("QT_QUICK_CONTROLS_MOBILE"));
    const QLatin1StringView fileName(isMobile ? "featured-mobile-5.9.json" : "featured-5.9.json");

    return QUrl(baseURL + fileName);
}

FeaturedModel::FeaturedModel()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);

    const static QUrl url = featuredURL();
    const QString fileName = url.fileName();
    *featuredCache = dir + QLatin1Char('/') + fileName;
    const bool shouldBlock = !QFileInfo::exists(*featuredCache);
    auto *fetchJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    if (shouldBlock) {
        acquireFetching(true);
    }
    connect(fetchJob, &KIO::StoredTransferJob::result, this, [this, fetchJob, shouldBlock]() {
        const auto dest = qScopeGuard([this, shouldBlock] {
            if (shouldBlock) {
                acquireFetching(false);
            }
            refresh();
        });
        if (fetchJob->error() != 0)
            return;

        QFile f(*featuredCache);
        if (!f.open(QIODevice::WriteOnly))
            qCWarning(DISCOVER_LOG) << "could not open" << *featuredCache << f.errorString();
        f.write(fetchJob->data());
        f.close();
    });
    if (!shouldBlock) {
        refresh();
    }
}

void FeaturedModel::refresh()
{
    // usually only useful if launching just fwupd or kns backends
    if (!currentApplicationBackend())
        return;

    acquireFetching(true);
    const auto dest = qScopeGuard([this] {
        acquireFetching(false);
    });

    // Popular applications that users actually want
    // Using correct appstream IDs that are available in Fedora/Flathub repos
    static const QVector<QUrl> popularApps = {
        // Communication
        QUrl(QStringLiteral("appstream://telegram-desktop.desktop")),       // Telegram (Fedora)
        QUrl(QStringLiteral("appstream://org.telegram.desktop.desktop")),   // Telegram (Flatpak)
        QUrl(QStringLiteral("appstream://discord.desktop")),                // Discord (RPM)
        QUrl(QStringLiteral("appstream://com.discordapp.Discord.desktop")), // Discord (Flatpak)

        // Browsers
        QUrl(QStringLiteral("appstream://google-chrome.desktop")),          // Google Chrome
        QUrl(QStringLiteral("appstream://firefox.desktop")),                // Firefox (Fedora)
        QUrl(QStringLiteral("appstream://org.mozilla.firefox.desktop")),    // Firefox (Flatpak)
        QUrl(QStringLiteral("appstream://chromium-browser.desktop")),       // Chromium
        QUrl(QStringLiteral("appstream://brave-browser.desktop")),          // Brave

        // Gaming
        QUrl(QStringLiteral("appstream://steam.desktop")),                  // Steam (RPM)
        QUrl(QStringLiteral("appstream://com.valvesoftware.Steam.desktop")), // Steam (Flatpak)

        // Media
        QUrl(QStringLiteral("appstream://vlc.desktop")),                    // VLC (Fedora)
        QUrl(QStringLiteral("appstream://org.videolan.VLC.desktop")),       // VLC (Flatpak)
        QUrl(QStringLiteral("appstream://spotify.desktop")),                // Spotify
        QUrl(QStringLiteral("appstream://com.spotify.Client.desktop")),     // Spotify (Flatpak)
        QUrl(QStringLiteral("appstream://obs-studio.desktop")),             // OBS
        QUrl(QStringLiteral("appstream://com.obsproject.Studio.desktop")),  // OBS (Flatpak)

        // Development
        QUrl(QStringLiteral("appstream://code.desktop")),                   // VS Code (RPM)
        QUrl(QStringLiteral("appstream://com.visualstudio.code.desktop")),  // VS Code (Flatpak)

        // Graphics
        QUrl(QStringLiteral("appstream://gimp.desktop")),                   // GIMP (Fedora)
        QUrl(QStringLiteral("appstream://org.gimp.GIMP.desktop")),          // GIMP (Flatpak)
        QUrl(QStringLiteral("appstream://org.kde.krita.desktop")),          // Krita
        QUrl(QStringLiteral("appstream://inkscape.desktop")),               // Inkscape
        QUrl(QStringLiteral("appstream://org.inkscape.Inkscape.desktop")),  // Inkscape (Flatpak)

        // Office
        QUrl(QStringLiteral("appstream://libreoffice-startcenter.desktop")), // LibreOffice
        QUrl(QStringLiteral("appstream://org.libreoffice.LibreOffice.desktop")), // LibreOffice (Flatpak)

        // Utilities
        QUrl(QStringLiteral("appstream://qbittorrent.desktop")),            // qBittorrent
        QUrl(QStringLiteral("appstream://org.qbittorrent.qBittorrent.desktop")), // qBittorrent (Flatpak)
        QUrl(QStringLiteral("appstream://thunderbird.desktop")),            // Thunderbird
        QUrl(QStringLiteral("appstream://org.mozilla.Thunderbird.desktop")) // Thunderbird (Flatpak)
    };

    // Try to load additional apps from cache if available
    QFile f(*featuredCache);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError error;
        const auto array = QJsonDocument::fromJson(f.readAll(), &error).array();
        if (!error.error && !array.isEmpty()) {
            // Parse cached URIs
            const auto cachedUris = kTransform<QVector<QUrl>>(array, [](const QJsonValue &uri) {
                return QUrl(uri.toString());
            });

            // Combine popular apps with cached ones (popular apps first)
            QVector<QUrl> combined = popularApps;
            for (const auto &uri : cachedUris) {
                if (!combined.contains(uri)) {
                    combined.append(uri);
                }
            }
            setUris(combined);
            return;
        }
    }

    // Use only popular apps if cache is not available or invalid
    setUris(popularApps);
}

#include "moc_FeaturedModel.cpp"
