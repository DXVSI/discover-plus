/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppPackageKitResource.h"
#include "utils.h"
#include <AppStreamQt/component.h>
#include <AppStreamQt/developer.h>
#include <AppStreamQt/icon.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/provided.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/systeminfo.h>
#include <AppStreamQt/version.h>
#include <KLocalizedString>
#include <KService>
#include <LazyIconResolver.h>
#include <PackageKit/Daemon>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QProcess>
#include <QStandardPaths>
#include <QUrlQuery>
#include <appstream/AppStreamUtils.h>

using namespace Qt::StringLiterals;

AppPackageKitResource::AppPackageKitResource(const AppStream::Component &data, const QString &packageName, PackageKitBackend *parent)
    : PackageKitResource(packageName, QString(), parent)
    , m_appdata(data)
{
    Q_ASSERT(data.isValid());
}

QString AppPackageKitResource::name() const
{
    if (m_name.isEmpty()) {
        if (!m_appdata.extends().isEmpty()) {
            const auto components = backend()->componentsById(m_appdata.extends().constFirst());

            if (components.isEmpty()) {
                qWarning() << "couldn't find" << m_appdata.extends() << "which is supposedly extended by" << m_appdata.id();
            } else {
                m_name = components.indexSafe(0)->name() + QLatin1String(" - ") + m_appdata.name();
            }
        }

        if (m_name.isEmpty()) {
            m_name = m_appdata.name();
        }
    }
    return m_name;
}

QString AppPackageKitResource::longDescription()
{
    const auto desc = m_appdata.description();
    if (!desc.isEmpty()) {
        return desc;
    }

    return PackageKitResource::longDescription();
}

static QIcon componentIcon(const AppStream::Component &comp)
{
    QIcon ret;
    const auto icons = comp.icons();
    for (const AppStream::Icon &icon : icons) {
        switch (icon.kind()) {
        case AppStream::Icon::KindLocal:
            ret.addFile(icon.url().toLocalFile(), icon.size());
            break;
        case AppStream::Icon::KindCached:
            ret.addFile(icon.url().toLocalFile(), icon.size());
            break;
        case AppStream::Icon::KindStock: {
            if (AppStreamUtils::kIconLoaderHasIcon(icon.name())) {
                return QIcon::fromTheme(icon.name());
            }
            break;
        }
        default:
            break;
        }
    }
    if (ret.isNull()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    }
    return ret;
}

QVariant AppPackageKitResource::icon() const
{
    if (m_icon.has_value()) {
        return m_icon.value();
    }

    LazyIconResolver::instance()->queue(const_cast<AppPackageKitResource *>(this));
    return u"package-x-generic"_s;
}

QJsonArray AppPackageKitResource::licenses()
{
    return m_appdata.projectLicense().isEmpty() ? PackageKitResource::licenses() : AppStreamUtils::licenses(m_appdata);
}

QStringList AppPackageKitResource::mimetypes() const
{
    return m_appdata.provided(AppStream::Provided::KindMimetype).items();
}

static constexpr auto s_addonKinds = {AppStream::Component::KindAddon, AppStream::Component::KindCodec};

bool AppPackageKitResource::hasCategory(const QString &category) const
{
    if (m_appdata.kind() != AppStream::Component::KindAddon && category == QLatin1StringView("Application"))
        return true;
    if (category == QLatin1StringView("Drivers")) {
        switch (m_appdata.kind()) {
        case AppStream::Component::KindDriver: {
            const auto mods = m_appdata.provided(AppStream::Provided::KindModalias).items();
            auto sys = AppStream::SystemInfo();
            for (const auto &mod : mods) {
                if (sys.hasDeviceMatchingModalias(mod)) {
                    return true;
                }
            }
        } break;
        case AppStream::Component::KindFirmware:
            return true;
        default:
            return false;
        }
    }
    if (m_appdata.kind() == AppStream::Component::KindFont && category == QStringLiteral("Fonts"))
        return true;
    return m_appdata.hasCategory(category);
}

QString AppPackageKitResource::comment()
{
    const auto summary = m_appdata.summary();
    if (!summary.isEmpty()) {
        return summary;
    }

    return PackageKitResource::comment();
}

QString AppPackageKitResource::appstreamId() const
{
    return m_appdata.id();
}

QSet<QString> AppPackageKitResource::alternativeAppstreamIds() const
{
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    const auto ret = m_appdata.provided(AppStream_Provided_KindId).items();
    return QSet<QString>(ret.begin(), ret.end());
}

QUrl AppPackageKitResource::url() const
{
    QUrl ret(QStringLiteral("appstream://") + appstreamId());
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    auto provided = m_appdata.provided(AppStream_Provided_KindId).items();
    provided.removeAll(appstreamId()); // Just in case, it has happened before
    if (!provided.isEmpty()) {
        QUrlQuery qq;
        qq.addQueryItem(u"alt"_s, provided.join(QLatin1Char(',')));
        ret.setQuery(qq);
    }
    return ret;
}

QUrl AppPackageKitResource::homepage()
{
    return m_appdata.url(AppStream::Component::UrlKindHomepage);
}

QUrl AppPackageKitResource::helpURL()
{
    return m_appdata.url(AppStream::Component::UrlKindHelp);
}

QUrl AppPackageKitResource::bugURL()
{
    return m_appdata.url(AppStream::Component::UrlKindBugtracker);
}

QUrl AppPackageKitResource::donationURL()
{
    return m_appdata.url(AppStream::Component::UrlKindDonation);
}

QUrl AppPackageKitResource::contributeURL()
{
    return m_appdata.url(AppStream::Component::UrlKindContribute);
}

AbstractResource::Type AppPackageKitResource::type() const
{
    static QString desktop = QString::fromUtf8(qgetenv("XDG_CURRENT_DESKTOP"));
    const auto desktops = m_appdata.compulsoryForDesktops();
    if (kContainsValue(s_addonKinds, m_appdata.kind())) {
        return Addon;
    } else if (desktops.isEmpty() || !desktops.contains(desktop)) {
        return Application;
    } else {
        return System;
    }
}

void AppPackageKitResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched(AppStreamUtils::fetchScreenshots(m_appdata));
}

QStringList AppPackageKitResource::allPackageNames() const
{
    auto ret = m_appdata.packageNames();
    if (ret.isEmpty()) {
        ret = QStringList{PackageKit::Daemon::packageName(availablePackageId())};
    }
    return ret;
}

QList<PackageState> AppPackageKitResource::addonsInformation()
{
    const auto extendedBy = backend()->extendedBy(m_appdata.id());
    const auto allPackageNamesCached = allPackageNames();
    const auto resources = kFilter<QVector<AbstractResource *>>(extendedBy, [&](AbstractResource *r) {
        auto apkr = qobject_cast<AppPackageKitResource *>(r);
        return apkr && apkr->allPackageNames() != allPackageNamesCached;
    });
    return kTransform<QList<PackageState>>(resources, [](AbstractResource *r) {
        return PackageState(r->packageName(), r->name(), r->comment(), r->isInstalled());
    });
}

QStringList AppPackageKitResource::extends() const
{
    return m_appdata.extends();
}

QString AppPackageKitResource::changelog() const
{
    return PackageKitResource::changelog() + QLatin1String("<br />") + AppStreamUtils::changelogToHtml(m_appdata);
}

bool AppPackageKitResource::canExecute() const
{
    return !m_appdata.launchable(AppStream::Launchable::KindDesktopId).entries().isEmpty();
}

void AppPackageKitResource::invokeApplication() const
{
    const QString launchable = m_appdata.launchable(AppStream::Launchable::KindDesktopId).entries().constFirst();

    KService::Ptr service = KService::serviceByStorageId(launchable);

    if (!service) {
        Q_EMIT backend()->passiveMessage(i18n("Cannot launch %1", name()));
        return;
    }
    runService(service);
}

QString AppPackageKitResource::versionString()
{
    return isInstalled() ? installedVersion() : availableVersion();
}

QDate AppPackageKitResource::releaseDate() const
{
    if (const auto releases = m_appdata.releasesPlain(); !releases.isEmpty()) {
        auto release = releases.indexSafe(0).value();
        return release.timestamp().date();
    }

    return {};
}

QString AppPackageKitResource::author() const
{
    QString name = m_appdata.developer().name();

    if (name.isEmpty()) {
        name = m_appdata.projectGroup();
    }

    return name;
}

void AppPackageKitResource::fetchChangelog()
{
    Q_EMIT changelogFetched(changelog());
}

bool AppPackageKitResource::isCritical() const
{
    return m_appdata.isCompulsoryForDesktop(qEnvironmentVariable("XDG_CURRENT_DESKTOP"));
}

QString AppPackageKitResource::contentRatingDescription() const
{
    return AppStreamUtils::contentRatingDescription(m_appdata);
}

uint AppPackageKitResource::contentRatingMinimumAge() const
{
    return AppStreamUtils::contentRatingMinimumAge(m_appdata);
}

bool AppPackageKitResource::hasResolvedIcon() const
{
    return m_icon.has_value();
}

void AppPackageKitResource::resolveIcon()
{
    m_icon = componentIcon(m_appdata);
    Q_EMIT iconChanged();
}

#include "moc_AppPackageKitResource.cpp"
