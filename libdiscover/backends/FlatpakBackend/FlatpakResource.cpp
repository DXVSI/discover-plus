/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakResource.h"
#include "FlatpakBackend.h"
#include "FlatpakFetchDataJob.h"
#include "FlatpakSourcesBackend.h"
#include "LazyIconResolver.h"
#include "config-paths.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <Transaction/AddonList.h>

#include <AppStreamQt/developer.h>
#include <AppStreamQt/icon.h>
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/utils.h>
#include <AppStreamQt/version.h>
#include <appstream/AppStreamUtils.h>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KFormat>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>

#include <AppStreamQt/release.h>
#include <QCoroCore>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QQueue>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrlQuery>
#include <QtConcurrentRun>

using namespace Qt::StringLiterals;

static QString iconCachePath(const AppStream::Icon &icon)
{
    Q_ASSERT(icon.kind() == AppStream::Icon::KindRemote);
    return QStringLiteral("%1/icons/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), icon.url().fileName());
}

const QStringList FlatpakResource::s_topObjects({
    QStringLiteral("qrc:/qml/FlatpakAttention.qml"),
    QStringLiteral("qrc:/qml/FlatpakRemoveData.qml"),
    QStringLiteral("qrc:/qml/FlatpakOldBeta.qml"),
    QStringLiteral("qrc:/qml/FlatpakEolReason.qml"),
});
const QStringList FlatpakResource::s_bottomObjects({QStringLiteral("qrc:/qml/PermissionsList.qml")});

Q_GLOBAL_STATIC(QNetworkAccessManager, manager)

FlatpakResource::FlatpakResource(const AppStream::Component &component, FlatpakInstallation *installation, FlatpakBackend *parent)
    : AbstractResource(parent)
    , m_appdata(component)
    , m_id({component.id(), QString(), QString()})
    , m_downloadSize(0)
    , m_installedSize(0)
    , m_propertyStates({{DownloadSize, NotKnownYet}, {InstalledSize, NotKnownYet}, {RequiredRuntime, NotKnownYet}})
    , m_state(AbstractResource::None)
    , m_installation(installation)
{
    setObjectName(packageName());

    // Start fetching remote icons during initialization
    const auto icons = m_appdata.icons();
    m_stockIcon = std::ranges::any_of(icons, [](const AppStream::Icon &icon) {
        return icon.kind() == AppStream::Icon::KindStock && AppStreamUtils::kIconLoaderHasIcon(icon.name());
    });
    connect(this, &AbstractResource::iconChanged, this, [this] {
        Q_EMIT backend()->resourcesChanged(this, {"icon"});
    });
    if (!m_stockIcon && !icons.isEmpty() && !std::ranges::any_of(icons, [](const AppStream::Icon &icon) {
            return icon.kind() == AppStream::Icon::KindLocal || icon.kind() == AppStream::Icon::KindCached;
        })) {
        for (const auto &icon : icons) {
            if (icon.kind() != AppStream::Icon::KindRemote) {
                continue;
            }
            const QString fileName = iconCachePath(icon);
            if (!QFileInfo::exists(fileName)) {
                const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
                // Create $HOME/.cache/discover/icons folder
                cacheDir.mkdir(QStringLiteral("icons"));
                auto reply = manager->get(QNetworkRequest(icon.url()));
                connect(reply, &QNetworkReply::finished, this, [this, icon, fileName, reply] {
                    if (reply->error() == QNetworkReply::NoError) {
                        QByteArray iconData = reply->readAll();
                        QFile file(fileName);
                        if (file.open(QIODevice::WriteOnly)) {
                            file.write(iconData);
                        } else {
                            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not find icon for" << packageName() << reply->url();
                            QIcon::fromTheme(QStringLiteral("package-x-generic")).pixmap(32, 32).toImage().save(fileName);
                        }
                        file.close();
                        Q_EMIT iconChanged();
                        reply->deleteLater();
                    }
                });
            }
        }
    }

    connect(this, &FlatpakResource::stateChanged, this, &FlatpakResource::hasDataChanged);
}

FlatpakBackend *FlatpakResource::backend() const
{
    const auto backend = qobject_cast<FlatpakBackend *>(AbstractResource::backend());
    Q_ASSERT(backend);
    return backend;
}

AppStream::Component FlatpakResource::appstreamComponent() const
{
    return m_appdata;
}

QList<PackageState> FlatpakResource::addonsInformation()
{
    return {};
}

QString FlatpakResource::availableVersion() const
{
    if (m_availableVersion.isEmpty()) {
        const auto releases = m_appdata.releasesPlain().entries();
        if (!releases.isEmpty()) {
            auto latestVersion = releases.constFirst().version();
            for (const auto &release : releases) {
                if (AppStream::Utils::vercmpSimple(release.version(), latestVersion) > 0) {
                    latestVersion = release.version();
                }
            };
            m_availableVersion = latestVersion;
            return m_availableVersion;
        }
    } else {
        return m_availableVersion;
    }
    return branch();
}

QString FlatpakResource::appstreamId() const
{
    return m_id.id;
}

QString FlatpakResource::arch() const
{
    return m_id.arch;
}

QString FlatpakResource::branch() const
{
    return m_id.branch;
}

bool FlatpakResource::canExecute() const
{
    return (m_type == DesktopApp && (m_state == AbstractResource::Installed || m_state == AbstractResource::Upgradeable));
}

void FlatpakResource::updateFromRef(FlatpakRef *ref)
{
    setArch(QString::fromUtf8(flatpak_ref_get_arch(ref)));
    setBranch(QString::fromUtf8(flatpak_ref_get_branch(ref)));
    setFlatpakName(QString::fromUtf8(flatpak_ref_get_name(ref)));
    setType(flatpak_ref_get_kind(ref) == FLATPAK_REF_KIND_APP ? DesktopApp : extends().isEmpty() ? Runtime : Extension);
    setObjectName(packageName());
}

void FlatpakResource::updateFromAppStream()
{
    const QString refstr = m_appdata.bundle(AppStream::Bundle::KindFlatpak).id();
    g_autoptr(GError) localError = nullptr;
    g_autoptr(FlatpakRef) ref = flatpak_ref_parse(refstr.toUtf8().constData(), &localError);
    if (!ref) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed to obtain ref" << refstr << localError->message;
        return;
    }
    updateFromRef(ref);
}

bool FlatpakResource::hasCategory(const QString &category) const
{
    if (m_appdata.kind() != AppStream::Component::KindAddon && category == QStringLiteral("Application"))
        return true;
    return m_appdata.hasCategory(category);
}

QString FlatpakResource::comment()
{
    const auto summary = m_appdata.summary();
    if (!summary.isEmpty()) {
        return summary;
    }

    return QString();
}

quint64 FlatpakResource::downloadSize() const
{
    return m_downloadSize;
}

void FlatpakResource::resolveIcon()
{
    m_icon = QIcon();
    const auto icons = m_appdata.icons();

    if (!m_bundledIcon.isNull()) {
        m_icon = QIcon(m_bundledIcon);
    } else if (icons.isEmpty()) {
        m_icon = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else {
        for (const AppStream::Icon &icon : icons) {
            switch (icon.kind()) {
            case AppStream::Icon::KindLocal:
            case AppStream::Icon::KindCached: {
                const QString path = icon.url().toLocalFile();
                if (QDir::isRelativePath(path)) {
                    const QString appstreamLocation =
                        installationPath() + "/appstream/"_L1 + origin() + '/'_L1 + QString::fromUtf8(flatpak_get_default_arch()) + "/active/icons/"_L1;
                    QDirIterator dit(appstreamLocation, QDirIterator::Subdirectories);
                    while (dit.hasNext()) {
                        const auto currentPath = dit.next();
                        if (dit.fileName() == path) {
                            m_icon->addFile(currentPath, icon.size());
                        }
                    }
                } else {
                    m_icon->addFile(path, icon.size());
                }
                break;
            }
            case AppStream::Icon::KindStock: {
                if (m_stockIcon) {
                    m_icon = QIcon::fromTheme(icon.name());
                }
                break;
            }
            case AppStream::Icon::KindRemote: {
                const QString fileName = iconCachePath(icon);
                if (QFileInfo::exists(fileName)) {
                    m_icon->addFile(fileName, icon.size());
                }
                break;
            }
            case AppStream::Icon::KindUnknown:
                break;
            }
        }
    }

    if (m_icon->isNull()) {
        m_icon = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    }

    Q_EMIT iconChanged();
}

QVariant FlatpakResource::icon() const
{
    if (m_icon.has_value()) {
        return m_icon.value();
    }

    LazyIconResolver::instance()->queue(const_cast<FlatpakResource *>(this));
    return u"package-x-generic"_s;
}

QString FlatpakResource::installedVersion() const
{
    g_autoptr(FlatpakInstalledRef) ref = backend()->getInstalledRefForApp(this);
    if (ref) {
        const char *appdataVersion = flatpak_installed_ref_get_appdata_version(ref);

        if (appdataVersion) {
            return QString::fromUtf8(appdataVersion);
        }
    }
    return branch();
}

quint64 FlatpakResource::installedSize() const
{
    return m_installedSize;
}

AbstractResource::Type FlatpakResource::type() const
{
    switch (m_type) {
    case FlatpakResource::Runtime:
        return ApplicationSupport;
    case FlatpakResource::Extension:
        return Addon;
    default:
        return Application;
    }
}

QUrl FlatpakResource::homepage()
{
    return m_appdata.url(AppStream::Component::UrlKindHomepage);
}

QUrl FlatpakResource::helpURL()
{
    return m_appdata.url(AppStream::Component::UrlKindHelp);
}

QUrl FlatpakResource::bugURL()
{
    return m_appdata.url(AppStream::Component::UrlKindBugtracker);
}

QUrl FlatpakResource::donationURL()
{
    return m_appdata.url(AppStream::Component::UrlKindDonation);
}

QUrl FlatpakResource::contributeURL()
{
    return m_appdata.url(AppStream::Component::UrlKindContribute);
}

FlatpakResource::FlatpakFileType FlatpakResource::flatpakFileType() const
{
    return m_flatpakFileType;
}

QString FlatpakResource::flatpakName() const
{
    // If the flatpak name is not known (known only for installed apps), then use
    // appstream id instead;
    if (m_flatpakName.isEmpty()) {
        return m_id.id;
    }

    return m_flatpakName;
}

QJsonArray FlatpakResource::licenses()
{
    return AppStreamUtils::licenses(m_appdata);
}

QString FlatpakResource::longDescription()
{
    return m_appdata.description();
}

QString FlatpakResource::attentionText() const
{
    if (m_flatpakFileType == FlatpakResource::FileFlatpakRef) {
        QUrl loc = m_resourceLocation;
        loc.setPath({});
        loc.setQuery(QUrlQuery());
        return xi18nc("@info",
                      "<para>This application comes from \"%1\" (hosted at <link>%2</link>). Other software in this repository will also be made be available "
                      "in Discover "
                      "when the application is "
                      "installed.</para>",
                      m_origin,
                      loc.toDisplayString());
    }
    return {};
}

QAbstractListModel *FlatpakResource::permissionsModel()
{
    if (m_permissions.empty()) {
        loadPermissions();
    }
    return new FlatpakPermissionsModel(m_permissions);
}

QString FlatpakResource::name() const
{
    QString name = m_appdata.name();
    if (name.isEmpty()) {
        name = flatpakName();
    }

    if (name.startsWith(QLatin1String("(Nightly) "))) {
        return name.mid(10);
    }

    return name;
}

QString FlatpakResource::origin() const
{
    return m_origin;
}

QString FlatpakResource::displayOrigin() const
{
    const QString originName = m_origin.toLower();

    // Return nice display names for known sources
    if (originName.contains(QStringLiteral("flathub"))) {
        return QStringLiteral("Flathub");
    } else if (originName.contains(QStringLiteral("fedora"))) {
        return QStringLiteral("Fedora Flatpaks");
    }

    return !m_displayOrigin.isEmpty() ? m_displayOrigin : m_origin;
}

// Note: The following three methods are not using each other for optimization purposes:
// use string builder with arguments directly instead of temporary allocated strings.
QString FlatpakResource::packageName() const
{
    return flatpakName() + '/'_L1 + arch() + '/'_L1 + branch();
}

QString FlatpakResource::ref() const
{
    return typeAsString() + '/'_L1 + flatpakName() + '/'_L1 + arch() + '/'_L1 + branch();
}

QString FlatpakResource::installPath() const
{
    return installationPath() + '/'_L1 + typeAsString() + '/'_L1 + flatpakName() + '/'_L1 + arch() + '/'_L1 + branch() + "/active"_L1;
}

FlatpakResource::PropertyState FlatpakResource::propertyState(FlatpakResource::PropertyKind kind) const
{
    return m_propertyStates[kind];
}

QUrl FlatpakResource::resourceFile() const
{
    return m_resourceFile;
}

QString FlatpakResource::runtime() const
{
    return m_runtime;
}

QString FlatpakResource::section()
{
    return QString();
}

quint64 FlatpakResource::size()
{
    if (m_state == Installed) {
        return m_installedSize;
    } else {
        return m_downloadSize;
    }
}

QString FlatpakResource::sizeDescription()
{
    if (propertyState(InstalledSize) == NotKnownYet || propertyState(InstalledSize) == Fetching) {
        backend()->updateAppSize(this);
        return i18n("Retrieving size information");
    } else if (propertyState(InstalledSize) == UnknownOrFailed) {
        return i18nc("@label app size", "Unknown");
    } else {
        const KFormat f;
        return f.formatByteSize(installedSize());
    }
}

AbstractResource::State FlatpakResource::state()
{
    return m_state;
}

FlatpakResource::ResourceType FlatpakResource::resourceType() const
{
    return m_type;
}

QLatin1String FlatpakResource::typeAsString() const
{
    switch (m_type) {
    case FlatpakResource::Runtime:
    case FlatpakResource::Extension:
        return QLatin1String("runtime");
    case FlatpakResource::DesktopApp:
    case FlatpakResource::Source:
    default:
        return QLatin1String("app");
    }
}

FlatpakResource::Id FlatpakResource::uniqueId() const
{
    return m_id;
}

void FlatpakResource::invokeApplication() const
{
    QString desktopFileName;
    auto launchables = m_appdata.launchable(AppStream::Launchable::KindDesktopId).entries();
    if (!launchables.isEmpty()) {
        desktopFileName = launchables.constFirst();
    } else {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to find launchable for " << m_appdata.name() << ", using AppStream identifier instead";
        desktopFileName = appstreamId();
    }

    KService::Ptr service = KService::serviceByStorageId(desktopFileName);

    if (!service) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to find service" << desktopFileName;
        return;
    }

    auto job = new KIO::ApplicationLauncherJob(service);
    connect(job, &KJob::finished, this, [this, service](KJob *job) {
        if (job->error()) {
            Q_EMIT backend()->passiveMessage(i18n("Failed to start '%1': %2", service->name(), job->errorString()));
        }
    });

    job->start();
}

void FlatpakResource::fetchChangelog()
{
    if (m_permissions.isEmpty()) {
        loadPermissions();
    }
    QString changelog = AppStreamUtils::changelogToHtml(m_appdata);
    if (!m_newPermissions.isEmpty()) {
        changelog += "<h3>New Permissions</h3>\n<ul>"_L1 + m_newPermissions + "</ul>"_L1;
    }
    Q_EMIT changelogFetched(changelog);
}

void FlatpakResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched(AppStreamUtils::fetchScreenshots(m_appdata));
}

void FlatpakResource::setArch(const QString &arch)
{
    m_id.arch = arch;
}

void FlatpakResource::setBranch(const QString &branch)
{
    m_id.branch = branch;
}

void FlatpakResource::setBundledIcon(const QPixmap &pixmap)
{
    m_bundledIcon = pixmap;
}

void FlatpakResource::reportSizeChange()
{
    Q_EMIT sizeChanged();

    if (!qobject_cast<FlatpakBackend *>(backend())->isFetching()) {
        static const QVector<QByteArray> properties = {
            "size",
            "sizeDescription",
        };
        Q_EMIT backend()->resourcesChanged(this, properties);
    }
}

void FlatpakResource::setDownloadSize(quint64 size)
{
    m_downloadSize = size;

    setPropertyState(DownloadSize, AlreadyKnown);

    reportSizeChange();
}

void FlatpakResource::setFlatpakFileType(FlatpakFileType fileType)
{
    m_flatpakFileType = fileType;
}

void FlatpakResource::setFlatpakName(const QString &name)
{
    m_flatpakName = name;
}

void FlatpakResource::setIconPath(const QString &path)
{
    m_iconPath = path;
}

void FlatpakResource::setInstalledSize(quint64 size)
{
    m_installedSize = size;

    setPropertyState(InstalledSize, AlreadyKnown);

    reportSizeChange();
}

void FlatpakResource::setOrigin(const QString &origin)
{
    m_origin = origin;
}

void FlatpakResource::setDisplayOrigin(const QString &displayOrigin)
{
    m_displayOrigin = displayOrigin;
}

void FlatpakResource::setPropertyState(FlatpakResource::PropertyKind kind, FlatpakResource::PropertyState newState)
{
    auto &state = m_propertyStates[kind];
    if (state != newState) {
        state = newState;

        Q_EMIT propertyStateChanged(kind, newState);
    }
}

void FlatpakResource::setResourceFile(const QUrl &url)
{
    m_resourceFile = url;
}

void FlatpakResource::setRuntime(const QString &runtime)
{
    m_runtime = runtime;

    setPropertyState(RequiredRuntime, AlreadyKnown);
}

void FlatpakResource::setState(AbstractResource::State state, bool shouldEmit)
{
    if (m_state != state) {
        m_state = state;

        if (shouldEmit && backend()->isTracked(this)) {
            Q_EMIT stateChanged();
        }
    }
}

void FlatpakResource::setType(FlatpakResource::ResourceType type)
{
    m_type = type;
}

QString FlatpakResource::installationPath() const
{
    return installationPath(m_installation);
}

QString FlatpakResource::installationPath(FlatpakInstallation *flatpakInstallation)
{
    g_autoptr(GFile) path = flatpak_installation_get_path(flatpakInstallation);
    g_autofree char *path_str = g_file_get_path(path);
    return QString::fromUtf8(path_str);
}

QUrl FlatpakResource::url() const
{
    if (!m_resourceFile.isEmpty()) {
        return m_resourceFile;
    }

    QUrl ret(QStringLiteral("appstream:") + appstreamId());
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    const auto provided = m_appdata.provided(AppStream_Provided_KindId).items();
    if (!provided.isEmpty()) {
        QUrlQuery qq;
        qq.addQueryItem(u"alt"_s, provided.join(QLatin1Char(',')));
        ret.setQuery(qq);
    }
    return ret;
}

QDate FlatpakResource::releaseDate() const
{
    if (const auto releases = m_appdata.releasesPlain(); !releases.isEmpty()) {
        auto release = releases.indexSafe(0);
        if (release) {
            return release->timestamp().date();
        }
    }

    return {};
}

QString FlatpakResource::sourceIcon() const
{
    const QString originName = origin().toLower();

    // Different icons for different Flatpak sources
    if (originName.contains(QStringLiteral("flathub"))) {
        return QStringLiteral("flatpak-discover");
    } else if (originName.contains(QStringLiteral("fedora"))) {
        return QStringLiteral("org.fedoraproject.AnacondaInstaller");
    }

    const auto sourceItem = backend()->sources()->sourceById(origin());
    if (!sourceItem) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Could not find source " << origin();
        return QStringLiteral("flatpak-discover");
    }

    const auto iconUrl = sourceItem->data(FlatpakSourcesBackend::IconUrlRole).toString();
    if (iconUrl.isEmpty()) {
        return QStringLiteral("flatpak-discover");
    }
    return iconUrl;
}

QString FlatpakResource::author() const
{
    QString name = m_appdata.developer().name();

    if (name.isEmpty()) {
        name = m_appdata.projectGroup();
    }

    return name;
}

QStringList FlatpakResource::extends() const
{
    return m_appdata.extends();
}

QSet<QString> FlatpakResource::alternativeAppstreamIds() const
{
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    const auto ret = m_appdata.provided(AppStream_Provided_KindId).items();

    return QSet<QString>(ret.begin(), ret.end());
}

QStringList FlatpakResource::mimetypes() const
{
    return m_appdata.provided(AppStream::Provided::KindMimetype).items();
}

QString FlatpakResource::versionString()
{
    QString version;
    if (resourceType() == Source) {
        return {};
    }
    if (isInstalled()) {
        auto ref = backend()->getInstalledRefForApp(this);
        if (ref) {
            version = QString::fromUtf8(flatpak_installed_ref_get_appdata_version(ref));
        }
    } else if (!m_appdata.releasesPlain().isEmpty()) {
        const auto release = m_appdata.releasesPlain().indexSafe(0).value();
        version = release.version();
    } else {
        version = m_id.branch;
    }
    return version;
}

QString translateSymbolicName(const QStringView &name)
{
    if (name == QLatin1String("host")) {
        return i18n("All Files");
    } else if (name == QLatin1String("home")) {
        return i18n("Home");
    } else if (name == QLatin1String("xdg-download")) {
        return i18n("Downloads");
    } else if (name == QLatin1String("xdg-music")) {
        return i18n("Music");
    }
    return name.toString();
}

QString FlatpakResource::eolReason()
{
    if (!m_eolReason.has_value() && !m_eolReasonTask.has_value()) {
        QPointer<FlatpakResource> self = this;

        m_eolReasonTask = [](FlatpakResource *self) -> QCoro::Task<std::optional<QString>> {
            QPointer<FlatpakResource> guard = self;
            auto backend = qobject_cast<FlatpakBackend *>(self->backend());
            auto cancellable = backend->cancellable();

            g_autoptr(FlatpakRemoteRef) rref = co_await QtConcurrent::run(backend->threadPool(), &FlatpakRunnables::findRemoteRef, self, cancellable);

            if (guard && !g_cancellable_is_cancelled(cancellable) && rref) {
                co_return QString::fromUtf8(flatpak_remote_ref_get_eol(rref));
            }

            co_return std::nullopt;
        }(this)
                                                           .then([guard = QPointer(this)](std::optional<QString> reason) {
                                                               if (!guard) {
                                                                   return;
                                                               }
                                                               guard->m_eolReasonTask.reset();
                                                               if (reason.has_value()) {
                                                                   guard->setEolReason(reason.value());
                                                               }
                                                           });
    }
    return m_eolReason.value_or(QString());
}

void FlatpakResource::setEolReason(const QString &reason)
{
    if (m_eolReason != reason) {
        m_eolReason = reason;
        Q_EMIT eolReasonChanged();
    }
}

QString createHtmlList(const QStringList &itemList)
{
    QString str = QStringLiteral("<ul>");
    for (const QString &itemText : std::as_const(itemList)) {
        str += QStringLiteral("<li>%1</li>").arg(itemText.toHtmlEscaped());
    }
    str += QStringLiteral("</ul>");
    return str;
}

bool FlatpakResource::updateNeedsAttention()
{
    if (m_permissions.isEmpty()) {
        loadPermissions();
    }
    return !m_newPermissions.isEmpty();
}

void FlatpakResource::loadPermissions()
{
    auto b = qobject_cast<FlatpakBackend *>(backend());
    QByteArray metaDataBytes = FlatpakRunnables::fetchMetadata(this, b->cancellable());

    QTemporaryFile f;
    if (!f.open()) {
        return;
    }
    f.write(metaDataBytes);
    f.close();
    m_permissions = readPermissions(f.fileName());

    auto currentPermissions = readPermissions(installPath() + "/metadata"_L1);
    for (const auto &permission : std::as_const(m_permissions)) {
        if (!currentPermissions.contains(permission)) {
            m_newPermissions += "<li>"_L1 + i18n("%1: %2", permission.brief(), permission.description()) + "</li>\n"_L1;
        }
    }
}

QVector<FlatpakPermission> FlatpakResource::readPermissions(const QString &file) const
{
    QVector<FlatpakPermission> permissions;
    KDesktopFile parser(file);

    QString brief, description;

    bool fullSessionBusAccess = false;
    bool fullSystemBusAccess = false;

    const KConfigGroup contextGroup = parser.group(u"Context"_s);
    const QString shared = contextGroup.readEntry("shared", QString());
    if (shared.contains("network"_L1)) {
        brief = i18n("Network Access");
        description = i18n("Can access the internet");
        permissions.append(FlatpakPermission(brief, description, u"network-wireless-symbolic"_s));
    }

    const QString sockets = contextGroup.readEntry("sockets", QString());
    const QString filesystems = contextGroup.readEntry("filesystems", QString());
    const auto dirs = QStringView(filesystems).split(';'_L1, Qt::SkipEmptyParts);

    if (sockets.contains("pulseaudio"_L1) || filesystems.contains("xdg-run/pipewire-0"_L1)) {
        brief = i18n("Sound system access");
        description = i18n("Can play and record audio");
        permissions.append(FlatpakPermission(brief, description, u"audio-speakers-symbolic"_s));
    }
    if (sockets.contains("session-bus"_L1)) {
        brief = i18n("Session Bus Access");
        description = i18n("Can communicate with all other applications and processes run in this user account");
        permissions.append(FlatpakPermission(brief, description, u"security-medium-symbolic"_s));
        fullSessionBusAccess = true;
    }
    if (sockets.contains("system-bus"_L1)) {
        brief = i18n("System Bus Access");
        description = i18n("Can communicate with all other applications and processes on the system");
        permissions.append(FlatpakPermission(brief, description, u"security-medium-symbolic"_s));
        fullSystemBusAccess = true;
    }
    if (sockets.contains("ssh-auth"_L1)) {
        brief = i18n("Remote Login Access");
        description = i18n("Can initiate remote login requests using the SSH protocol");
        permissions.append(FlatpakPermission(brief, description, u"x-shape-connection-symbolic"_s));
    }
    if (sockets.contains("pcsc"_L1)) {
        brief = i18n("Smart Card Access");
        description = i18n("Can integrate and communicate with smart cards");
        permissions.append(FlatpakPermission(brief, description, u"auth-sim-symbolic"_s));
    }
    if (sockets.contains("cups"_L1)) {
        brief = i18n("Printer Access");
        description = i18n("Can integrate and communicate with printers");
        permissions.append(FlatpakPermission(brief, description, u"printer-symbolic"_s));
    }
    if (sockets.contains("gpg-agent"_L1)) {
        brief = i18n("GPG Agent");
        description = i18n("Allows access to the GPG cryptography service, generally used for signing and reading signed documents");
        permissions.append(FlatpakPermission(brief, description, u"document-edit-sign-encrypt"_s));
    }

    const QString features = contextGroup.readEntry("features", QString());
    if (features.contains("bluetooth"_L1)) {
        brief = i18n("Bluetooth Access");
        description = i18n("Can integrate and communicate with Bluetooth devices");
        permissions.append(FlatpakPermission(brief, description, u"network-bluetooth-symbolic"_s));
    }
    if (features.contains("devel"_L1)) {
        brief = i18n("Low-Level System Access");
        description = i18n("Can make low-level system calls (e.g. ptrace)");
        permissions.append(FlatpakPermission(brief, description, u"run-build-symbolic"_s));
    }

    const QString devices = contextGroup.readEntry("devices", QString());
    if (devices.contains("all"_L1)) {
        brief = i18n("Device Access");
        description = i18n("Can communicate with and control built-in or connected hardware devices");
        permissions.append(FlatpakPermission(brief, description, u"device-notifier-symbolic"_s));
    }
    if (devices.contains("kvm"_L1)) {
        brief = i18n("Kernel-based Virtual Machine Access");
        description = i18n("Allows running other operating systems as guests in virtual machines");
        permissions.append(FlatpakPermission(brief, description, u"virt-manager-symbolic"_s));
    }

    QStringList homeList, systemList;
    bool home_ro = false, home_rw = false, home_cr = false, homeAccess = false;
    bool system_ro = false, system_rw = false, system_cr = false, systemAccess = false;

    bool hasHostRW = false;

    for (const QStringView &dir : dirs) {
        if (dir == QLatin1String("xdg-config/kdeglobals:ro")) {
            // Ignore notifying about the global file being visible, since it's intended by design
            continue;
        }

        if (dir.startsWith("xdg-run/pipewire-0"_L1)) {
            // This is already handled as sound system above
            continue;
        }

        int separator = dir.lastIndexOf(':'_L1);
        const QStringView postfix = separator > 0 ? dir.mid(separator) : QStringView();
        const QStringView symbolicName = dir.left(separator);
        const QString id = translateSymbolicName(symbolicName);
        if ((dir.contains(QLatin1String("home")) || dir.contains(QLatin1Char('~')))) {
            if (postfix == QLatin1String(":ro")) {
                homeList << i18n("%1 (read-only)", id);
                home_ro = true;
            } else if (postfix == QLatin1String(":create")) {
                homeList << i18n("%1 (can create files)", id);
                home_cr = true;
            } else {
                homeList << i18n("%1 (read & write) ", id);
                home_rw = true;
            }
            homeAccess = true;
        } else if (!hasHostRW) {
            if (postfix == QLatin1String(":ro")) {
                systemList << i18n("%1 (read-only)", id);
                system_ro = true;
            } else if (postfix == QLatin1String(":create")) {
                systemList << i18n("%1 (can create files)", id);
                system_cr = true;
            } else {
                // Once we have host in rw, no need to list the rest
                if (symbolicName == QLatin1String("host")) {
                    hasHostRW = true;
                    systemList.clear();
                }

                systemList << i18n("%1 (read & write) ", id);
                system_rw = true;
            }
            systemAccess = true;
        }
    }

    QString appendText = createHtmlList(homeList);
    if (homeAccess) {
        brief = i18n("Home Folder Access");
        if (home_rw && home_ro && home_cr) {
            description =
                i18n("Can read, write, and create files in the following locations in your home folder without asking permission first: %1", appendText);
        } else if (home_rw && !home_cr) {
            description = i18n("Can read and write files in the following locations in your home folder without asking permission first: %1", appendText);
        } else if (home_ro && !home_cr && !home_rw) {
            description = i18n("Can read files in the following locations in your home folder without asking permission first: %1", appendText);
        } else {
            description = i18n("Can access files in the following locations in your home folder without asking permission first: %1", appendText);
        }
        permissions.append(FlatpakPermission(brief, description, u"user-home-symbolic"_s));
    }
    appendText = createHtmlList(systemList);
    if (systemAccess) {
        brief = i18n("System Folder Access");
        if (system_rw && system_ro && system_cr) {
            description = i18n("Can read, write, and create system files in the following locations without asking permission first: %1", appendText);
        } else if (system_rw && !system_cr) {
            description = i18n("Can read and write system files in the following locations without asking permission first: %1", appendText);
        } else if (system_ro && !system_cr && !system_rw) {
            description = i18n("Can read system files in the following locations without asking permission first: %1", appendText);
        } else {
            description = i18n("Can access system files in the following locations without asking permission first: %1", appendText);
        }
        permissions.append(FlatpakPermission(brief, description, u"drive-harddisk-root-symbolic"_s));
    }

    if (!fullSessionBusAccess) {
        const KConfigGroup sessionBusGroup = parser.group(u"Session Bus Policy"_s);
        if (sessionBusGroup.exists()) {
            const QStringList busList = sessionBusGroup.keyList();
            brief = i18n("Session Bus Access");
            description =
                i18n("Can communicate with other applications and processes in the same desktop session using the following communication protocols: %1",
                     createHtmlList(busList));
            permissions.append(FlatpakPermission(brief, description, "plugins-symbolic"_L1));
        }
    }

    if (!fullSystemBusAccess) {
        const KConfigGroup systemBusGroup = parser.group(u"System Bus Policy"_s);
        if (systemBusGroup.exists()) {
            const QStringList busList = systemBusGroup.keyList();
            brief = i18n("System Bus Access");
            description =
                i18n("Can communicate with all applications and system services using the following communication protocols: %1", createHtmlList(busList));
            permissions.append(FlatpakPermission(brief, description, "plugins-symbolic"_L1));
        }
    }
    return permissions;
}

QString FlatpakResource::dataLocation() const
{
    auto id = m_appdata.bundle(AppStream::Bundle::KindFlatpak).id().section('/'_L1, 0, 1);
    if (id.isEmpty()) {
        return {};
    }
    return QDir::homePath() + QLatin1String("/.var/") + id;
}

bool FlatpakResource::hasData() const
{
    return !dataLocation().isEmpty() && QDir(dataLocation()).exists();
}

void FlatpakResource::clearUserData()
{
    const auto location = dataLocation();
    if (location.isEmpty()) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed find location for" << name();
        return;
    }

    if (!QDir(location).removeRecursively()) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to remove location" << location;
    }
    Q_EMIT hasDataChanged();
}

int FlatpakResource::versionCompare(FlatpakResource *resource) const
{
    // This is getting called from QML, we better sanitize the input
    if (!resource) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Cannot compare with a null resource";
        return -1;
    }

    const QString other = resource->availableVersion();
    return AppStream::Utils::vercmpSimple(availableVersion(), other);
}

QString FlatpakResource::contentRatingDescription() const
{
    return AppStreamUtils::contentRatingDescription(m_appdata);
}

uint FlatpakResource::contentRatingMinimumAge() const
{
    return AppStreamUtils::contentRatingMinimumAge(m_appdata);
}

QStringList FlatpakResource::topObjects() const
{
    return s_topObjects;
}

QStringList FlatpakResource::bottomObjects() const
{
    // Don't need permissions for non-apps; it doesn't make any sense
    if (isDesktopApp()) {
        return s_bottomObjects;
    }
    return {};
}

void FlatpakResource::addRefToUpdate(const QByteArray &toUpdate)
{
    m_toUpdate += toUpdate;
}

void FlatpakResource::clearToUpdate()
{
    m_toUpdate.clear();
}

bool FlatpakResource::hasResolvedIcon() const
{
    return m_icon.has_value();
}

QString FlatpakResource::verifiedMessage() const
{
    const bool flathubVerified = m_appdata.customValue(QStringLiteral("flathub::verification::verified")) == QLatin1String("true");
    if (flathubVerified) {
        bool ok;
        const qint64 time = m_appdata.customValue(QStringLiteral("flathub::verification::timestamp")).toUInt(&ok);
        return i18n("App Verified on %1 as shipped by its author %2",
                    KFormat().formatRelativeDateTime(QDateTime::fromSecsSinceEpoch(time), QLocale::ShortFormat),
                    author());
    }
    return {};
}

#include "moc_FlatpakResource.cpp"
