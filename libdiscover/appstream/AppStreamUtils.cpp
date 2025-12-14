/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamUtils.h"

#include "utils.h"
#include <AppStreamQt/category.h>
#include <AppStreamQt/component.h>
#include <AppStreamQt/pool.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/spdx.h>
#include <AppStreamQt/version.h>
#include <Category/Category.h>
#include <KIconLoader>
#include <KLocalizedString>
#include <QCoroTimer>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>
#include <QUrlQuery>
#include <QtConcurrentRun>

using namespace std::chrono_literals;
using namespace AppStreamUtils;

AppStream::Image AppStreamUtils::imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind)
{
    for (const AppStream::Image &i : images) {
        if (i.kind() == kind) {
            return i;
        }
    }
    return {};
}

QString AppStreamUtils::changelogToHtml(const AppStream::Component &appdata)
{
    const auto releases = appdata.releasesPlain();
    if (releases.isEmpty()) {
        return {};
    }

    const auto release = releases.indexSafe(0).value();
    if (release.description().isEmpty())
        return {};

    QString changelog =
        QLatin1String("<h3>") + release.version() + QLatin1String("</h3>") + QStringLiteral("<p>") + release.description() + QStringLiteral("</p>");
    return changelog;
}

Screenshots AppStreamUtils::fetchScreenshots(const AppStream::Component &appdata)
{
    const auto appdataScreenshots = appdata.screenshotsAll();
    Screenshots ret;
    ret.reserve(appdataScreenshots.size());
    for (const AppStream::Screenshot &s : appdataScreenshots) {
        const auto images = s.images();
        const auto thumbnail = AppStreamUtils::imageOfKind(images, AppStream::Image::KindThumbnail);
        const auto full = AppStreamUtils::imageOfKind(images, AppStream::Image::KindSource);
        if (full.url().isEmpty()) {
            qWarning() << "AppStreamUtils: Invalid screenshot for" << appdata.name();
        }
        const bool isAnimated = s.mediaKind() == AppStream::Screenshot::MediaKindVideo;

        if (thumbnail.url().isEmpty()) {
            ret.append(Screenshot{full.url(), full.url(), isAnimated, full.size()});
        } else {
            ret.append(Screenshot{thumbnail.url(), full.url(), isAnimated, thumbnail.size()});
        }
    }
    return ret;
}

QJsonArray AppStreamUtils::licenses(const AppStream::Component &appdata)
{
    return licenses(appdata.projectLicense());
}

QJsonArray AppStreamUtils::licenses(const QString &spdx)
{
    static const QSet<QChar> tokens = {QLatin1Char('&'), QLatin1Char('+'), QLatin1Char('|'), QLatin1Char('^'), QLatin1Char('('), QLatin1Char(')')};

    QJsonArray ret;
    const auto licenses = AppStream::SPDX::tokenizeLicense(spdx);
    for (const auto &token : licenses) {
        if (token.size() == 1 && tokens.contains(token.at(0)))
            continue;

        // Sometimes tokenize prefixes with an @ for some reason
        if (token.startsWith(QStringLiteral("@"))) {
            ret += license(token.mid(1));
        } else {
            ret += license(token);
        }
    }
    return ret;
}

QJsonObject AppStreamUtils::license(const QString &license)
{
    QString name = license;
    QString url = QString();
    QString licenseType = QStringLiteral("unknown");

    if (license.isEmpty() || license.compare(QStringLiteral("unknown"), Qt::CaseInsensitive) == 0) {
        name = i18n("Unknown");
    } else if (license.startsWith(QLatin1String("LicenseRef-proprietary"), Qt::CaseInsensitive)) {
        name = i18n("Proprietary");
        licenseType = QStringLiteral("proprietary");
    } else if (license.startsWith(QLatin1String("LicenseRef-public-domain"), Qt::CaseInsensitive)
               || license.contains(QLatin1String("public domain"), Qt::CaseInsensitive)) {
        name = i18n("Public Domain");
        licenseType = QStringLiteral("free");
    } else if (!AppStream::SPDX::isLicenseId(license)) {
        licenseType = QStringLiteral("non-free");
    } else {
        url = AppStream::SPDX::licenseUrl(license);
        licenseType = AppStream::SPDX::isFreeLicense(license) ? QStringLiteral("free") : QStringLiteral("non-free");
    }

    return {
        {QLatin1StringView("name"), name},
        {QLatin1StringView("url"), {url}},
        {QLatin1StringView("licenseType"), licenseType},
    };
}

QStringList AppStreamUtils::appstreamIds(const QUrl &appstreamUrl)
{
    QStringList ret;
    ret += appstreamUrl.host().isEmpty() ? appstreamUrl.path() : appstreamUrl.host();
    if (appstreamUrl.hasQuery()) {
        QUrlQuery query(appstreamUrl);
        ret << query.queryItemValue(QStringLiteral("alt")).split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    if (ret.removeDuplicates() != 0) {
        qDebug() << "AppStreamUtils: Received malformed url" << appstreamUrl;
    }
    return ret;
}

QString AppStreamUtils::contentRatingDescription(const AppStream::Component &appdata)
{
    const auto ratings = appdata.contentRatings();
    QString ret;
    for (const auto &r : ratings) {
        const auto ratingIds = r.ratingIds();
        for (const auto &id : ratingIds) {
            if (r.value(id) != AppStream::ContentRating::RatingValueNone) {
                ret += QLatin1String("* ") + r.description(id) + QLatin1Char('\n');
            }
        }
    }

    return ret;
}

uint AppStreamUtils::contentRatingMinimumAge(const AppStream::Component &appdata)
{
    const auto ratings = appdata.contentRatings();
    uint minimumAge = 0;
    for (const auto &r : ratings) {
        minimumAge = std::max(r.minimumAge(), minimumAge);
    }
    return minimumAge;
}

static void kRemoveDuplicates(AppStream::ComponentBox &input, AppStream::Bundle::Kind kind)
{
    QSet<QString> ret;
    for (auto it = input.begin(); it != input.end();) {
        const auto key = kind == AppStream::Bundle::KindUnknown ? it->id() : it->bundle(kind).id();
        if (!ret.contains(key)) {
            ret << key;
            ++it;
        } else {
            it = input.erase(it);
        }
    }
}

QFuture<AppStream::ComponentBox>
AppStreamUtils::componentsByCategoriesTask(AppStream::ConcurrentPool *pool, const std::shared_ptr<Category> &cat, AppStream::Bundle::Kind kind)
{
    if (cat->name() == QLatin1StringView("All Applications")) {
        return pool->componentsByKind(AppStream::Component::KindDesktopApp);
    }

    const auto categories = cat->involvedCategories();
    QList<QFuture<AppStream::ComponentBox>> futures;
    futures.reserve(categories.size());
    if (cat->type() == Category::Type::Driver) {
        futures += pool->componentsByKind(AppStream::Component::KindDriver);
    } else if (cat->type() == Category::Type::Font) {
        futures += pool->componentsByKind(AppStream::Component::KindFont);
    } else {
        for (const auto &categoryName : categories) {
            futures += pool->componentsByCategories({categoryName});
        }
    }

    if (futures.size() == 1) {
        return futures.constFirst();
    }

    return QtConcurrent::run([futures, kind]() -> AppStream::ComponentBox {
        AppStream::ComponentBox ret(AppStream::ComponentBox::FlagNoChecks);
        for (const auto &box : futures) {
            ret += box.result();
        }
        kRemoveDuplicates(ret, kind);
        return ret;
    });
}

DISCOVERCOMMON_EXPORT bool AppStreamUtils::kIconLoaderHasIcon(const QString &name)
{
    static auto icons = [] {
#if (KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 11, 0))
        auto icons = KIconLoader::global()->queryIcons();
#else
        const auto groups = QMetaEnum::fromType<KIconLoader::Group>();
        QStringList icons;
        for (int i = 0; i < groups.keyCount(); ++i) {
            icons.append(KIconLoader::global()->queryIcons(groups.value(i)));
        }
#endif
        return QSet<QString>(icons.cbegin(), icons.cend());
    }();
    return icons.contains(name);
}
