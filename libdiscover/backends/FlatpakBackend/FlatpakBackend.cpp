/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakBackend.h"
#include "FlatpakFetchDataJob.h"
#include "FlatpakJobTransaction.h"
#include "FlatpakRefreshAppstreamMetadataJob.h"
#include "FlatpakSourcesBackend.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <ReviewsBackend/Rating.h>
#include <Transaction/Transaction.h>
#include <appstream/AppStreamConcurrentPool.h>
#include <appstream/AppStreamUtils.h>
#include <appstream/OdrsReviewsBackend.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>
#include <utils.h>
#include <utilscoro.h>

#include <AppStreamQt/bundle.h>
#include <AppStreamQt/icon.h>
#include <AppStreamQt/metadata.h>
#include <AppStreamQt/pool.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/version.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QCoroCore>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QtConcurrentRun>

#include <QRegularExpression>
#include <glib.h>

#include <Category/Category.h>
#include <optional>
#include <set>
#include <sys/stat.h>

DISCOVER_BACKEND_PLUGIN(FlatpakBackend)

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;
using namespace Utils;

namespace Utils
{

QString copyAndFree(char *str)
{
    const QString ret = QString::fromUtf8(str);
    g_free(str);
    return ret;
}

class ProgressCollector : public QObject
{
    Q_OBJECT
public:
    ProgressCollector(FlatpakBackend *backend)
        : m_backend(backend)
    {
        connect(m_backend->m_updater, &StandardBackendUpdater::settingUpChanged, this, &ProgressCollector::progressChanged);
    }

    void add(FlatpakRefreshAppstreamMetadataJob *job)
    {
        m_lastProgress = 0;
        m_jobs << job;
        connect(job, &FlatpakRefreshAppstreamMetadataJob::progressChanged, this, &ProgressCollector::progressChanged);
        connect(job, &FlatpakRefreshAppstreamMetadataJob::finished, this, [this]() {
            if (std::ranges::all_of(m_jobs, [](FlatpakRefreshAppstreamMetadataJob *j) {
                    return j->isFinished();
                })) {
                if (std::ranges::any_of(m_jobs, [](FlatpakRefreshAppstreamMetadataJob *j) -> bool {
                        return j->hasChanged();
                    })) {
                    m_backend->loadLocalUpdates();
                }
                qDeleteAll(m_jobs);
                m_jobs.clear();
                Q_EMIT progressChanged();
            }
        });
        Q_EMIT progressChanged();
    }

    uint progress()
    {
        if (!m_jobs.isEmpty()) {
            uint sum = 0;
            for (auto job : std::as_const(m_jobs)) {
                sum += job->progress();
            }
            static constexpr uint s_almostPercentage = 95; // We can't give 100% because we still have to process the updates locally and it's not trivial
            m_lastProgress = std::min<uint>(s_almostPercentage, std::max<uint>(m_lastProgress, sum / m_jobs.count()));
            return m_lastProgress;
        }
        return m_backend->m_updater->isSettingUp() ? m_lastProgress : 100;
    }

Q_SIGNALS:
    void progressChanged();

private:
    uint m_lastProgress = 0;
    FlatpakBackend *const m_backend;
    QList<FlatpakRefreshAppstreamMetadataJob *> m_jobs;
};
}

class FlatpakSource
{
public:
    FlatpakSource(FlatpakBackend *backend, FlatpakInstallation *installation)
        : m_remote(nullptr)
        , m_installation(installation)
        , m_backend(backend)
    {
        g_object_ref(m_installation);
    }

    FlatpakSource(FlatpakBackend *backend, FlatpakInstallation *installation, FlatpakRemote *remote)
        : m_remote(remote)
        , m_installation(installation)
        , m_backend(backend)
        , m_appstreamIconsDir(appstreamDir() + QLatin1String("/icons"))
    {
        g_object_ref(m_remote);
        g_object_ref(m_installation);
    }

    ~FlatpakSource()
    {
        if (m_remote) {
            g_object_unref(m_remote);
        }
        g_object_unref(m_installation);
    }

    QString url() const
    {
        return m_remote ? copyAndFree(flatpak_remote_get_url(m_remote)) : QString();
    }

    bool isEnabled() const
    {
        return m_remote && !flatpak_remote_get_disabled(m_remote);
    }

    QString appstreamIconsDir() const
    {
        return m_appstreamIconsDir;
    }
    QString appstreamDir() const
    {
        Q_ASSERT(m_remote);
        g_autoptr(GFile) appstreamDir = flatpak_remote_get_appstream_dir(m_remote, nullptr);
        if (!appstreamDir) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "No appstream dir for" << flatpak_remote_get_name(m_remote);
            return {};
        }
        g_autofree char *path_str = g_file_get_path(appstreamDir);
        return QString::fromUtf8(path_str);
    }

    QString name() const
    {
        return m_remote ? QString::fromUtf8(flatpak_remote_get_name(m_remote)) : QString();
    }

    QString title() const
    {
        auto ret = m_remote ? copyAndFree(flatpak_remote_get_title(m_remote)) : QString();
        if (flatpak_installation_get_is_user(m_installation)) {
            ret = i18nc("user denotes this as user-scoped flatpak repo", "%1 (user)", ret);
        }
        return ret;
    }

    FlatpakInstallation *installation() const
    {
        return m_installation;
    }

    void addResource(FlatpakResource *resource)
    {
        Q_ASSERT(!resource->packageName().isEmpty());
        m_backend->updateAppState(resource);

        Q_ASSERT(!m_resources.contains(resource->uniqueId()) || m_resources.value(resource->uniqueId()) == resource);
        m_resources.insert(resource->uniqueId(), resource);
    }

    FlatpakRemote *remote() const
    {
        return m_remote;
    }

    AppStream::ComponentBox componentsByFlatpakId(const QString &ref)
    {
        auto components = m_pool->componentsByBundleId(AppStream::Bundle::KindFlatpak, ref, false);
        if (!components.result().isEmpty())
            return components.result();

        components = m_pool->componentsByProvided(AppStream::Provided::KindId, ref.section('/'_L1, 1, 1));
        return components.result();
    }

    std::unique_ptr<AppStream::ConcurrentPool> m_pool;
    QHash<FlatpakResource::Id, FlatpakResource *> m_resources;

private:
    FlatpakRemote *const m_remote;
    FlatpakInstallation *const m_installation;
    FlatpakBackend *const m_backend;
    const QString m_appstreamIconsDir;
};

static void populateRemote(FlatpakRemote *remote, const QString &name, const QString &url, const QString &gpgKey)
{
    flatpak_remote_set_url(remote, url.toUtf8().constData());
    flatpak_remote_set_noenumerate(remote, false);
    flatpak_remote_set_title(remote, name.toUtf8().constData());

    if (!gpgKey.isEmpty()) {
        gsize dataLen = 0;
        g_autofree guchar *data = nullptr;
        g_autoptr(GBytes) bytes = nullptr;
        data = g_base64_decode(gpgKey.toUtf8().constData(), &dataLen);
        bytes = g_bytes_new(data, dataLen);
        flatpak_remote_set_gpg_verify(remote, true);
        flatpak_remote_set_gpg_key(remote, bytes);
    } else {
        flatpak_remote_set_gpg_verify(remote, false);
    }
}

QDebug operator<<(QDebug debug, const FlatpakResource::Id &id)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "FlatpakResource::Id(";
    debug.nospace() << "name:" << id.id << ',';
    debug.nospace() << "branch:" << id.branch;
    debug.nospace() << ')';
    return debug;
}

static FlatpakResource::Id idForComponent(const AppStream::Component &component)
{
    // app/app.getspace.Space/x86_64/stable
    const QString bundleId = component.bundle(AppStream::Bundle::KindFlatpak).id();
    auto parts = QStringView(bundleId).split('/'_L1);
    Q_ASSERT(parts.size() == 4);

    return {
        component.id(),
        parts[3].toString(),
        parts[2].toString(),
    };
}

static FlatpakResource::Id idForInstalledRef(FlatpakInstalledRef *ref, const QString &postfix = QString())
{
    const QString appId = QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref))) + postfix;
    const QString arch = QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(ref)));
    const QString branch = QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref)));

    return {appId, branch, arch};
}

static std::optional<AppStream::Metadata> metadataFromBytes(GBytes *appstreamGz, GCancellable *cancellable)
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GZlibDecompressor) decompressor = nullptr;
    g_autoptr(GInputStream) streamGz = nullptr;
    g_autoptr(GInputStream) streamData = nullptr;
    g_autoptr(GBytes) appstream = nullptr;

    /* decompress data */
    decompressor = g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_GZIP);
    streamGz = g_memory_input_stream_new_from_bytes(appstreamGz);
    if (!streamGz) {
        return {};
    }

    streamData = g_converter_input_stream_new(streamGz, G_CONVERTER(decompressor));

    appstream = g_input_stream_read_bytes(streamData, 0x100000, cancellable, &localError);
    if (!appstream) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to extract appstream metadata from bundle:" << localError->message;
        return {};
    }

    gsize len = 0;
    gconstpointer data = g_bytes_get_data(appstream, &len);

    AppStream::Metadata metadata;
    metadata.setFormatStyle(AppStream::Metadata::FormatStyleCatalog);
    AppStream::Metadata::MetadataError error = metadata.parse(QString::fromUtf8((char *)data, len), AppStream::Metadata::FormatKindXml);
    if (error != AppStream::Metadata::MetadataErrorNoError) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to parse appstream metadata: " << error;
        return {};
    }
    return metadata;
}

FlatpakBackend::FlatpakBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(OdrsReviewsBackend::global())
    , m_cancellable(g_cancellable_new())
    , m_checkForUpdatesTimer(new QTimer(this))
    , m_collector(new Utils::ProgressCollector(this))
{
    g_autoptr(GError) error = nullptr;

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FlatpakBackend::updatesCountChanged);
    connect(m_collector, &Utils::ProgressCollector::progressChanged, this, &FlatpakBackend::fetchingUpdatesProgressChanged);

    // Load flatpak installation
    if (!setupFlatpakInstallations(&error)) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to setup flatpak installations:" << error->message;
    } else {
        m_sources = new FlatpakSourcesBackend(m_installations, this);
        loadAppsFromAppstreamData();

        SourcesModel::global()->addSourcesBackend(m_sources);
    }

    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, [this] {
        m_reviews->emitRatingFetched(this, kAppend<QList<AbstractResource *>>(m_flatpakSources, [](const auto &source) {
                                         return kTransform<QList<AbstractResource *>>(source->m_resources.values());
                                     }));
    });

    m_checkForUpdatesTimer->setInterval(1000);
    m_checkForUpdatesTimer->setSingleShot(true);
    connect(m_checkForUpdatesTimer, &QTimer::timeout, this, &FlatpakBackend::checkForUpdates);

    /* Override the umask to 022 to make it possible to share files between
     * the plasma-discover process and flatpak system helper process.
     *
     * See https://github.com/flatpak/flatpak/pull/2856/
     */
    umask(022);
}

FlatpakBackend::~FlatpakBackend()
{
    g_cancellable_cancel(m_cancellable);
    if (!m_threadPool.waitForDone(200)) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not kill them all" << m_threadPool.activeThreadCount();
    }
    m_threadPool.clear();

    for (auto installation : std::as_const(m_installations)) {
        g_object_unref(installation);
    }
    m_installations.clear();
    g_object_unref(m_cancellable);
}

bool FlatpakBackend::isValid() const
{
    return m_sources && !m_installations.isEmpty();
}

class FlatpakFetchRemoteResourceJob : public QNetworkAccessManager
{
    Q_OBJECT
public:
    FlatpakFetchRemoteResourceJob(const QUrl &url, ResultsStream *stream, FlatpakBackend *backend)
        : QNetworkAccessManager(backend)
        , m_backend(backend)
        , m_stream(stream)
        , m_url(url)
    {
        connect(stream, &ResultsStream::destroyed, this, &QObject::deleteLater);
    }

    void start()
    {
        if (m_url.isLocalFile()) {
            QTimer::singleShot(0, m_stream, [this] {
                processFile(m_url);
            });
            return;
        }

        QNetworkRequest req(m_url);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        auto replyGet = get(req);
        connect(replyGet, &QNetworkReply::finished, this, [this, replyGet] {
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(replyGet);
            if (replyGet->error() != QNetworkReply::NoError) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "couldn't download" << m_url << replyGet->errorString();
                m_stream->finish();
                return;
            }
            const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) //
                                                     + QLatin1Char('/') + m_url.fileName());
            auto replyPut = put(QNetworkRequest(fileUrl), replyGet->readAll());
            connect(replyPut, &QNetworkReply::finished, this, [this, fileUrl, replyPut]() {
                QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(replyPut);
                if (replyPut->error() != QNetworkReply::NoError) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "couldn't save" << m_url << replyPut->errorString();
                    m_stream->finish();
                    return;
                }
                if (!fileUrl.isLocalFile()) {
                    m_stream->finish();
                    return;
                }

                processFile(fileUrl);
            });
        });
    }

private:
    void processFile(const QUrl &fileUrl)
    {
        const auto path = fileUrl.toLocalFile();
        if (path.endsWith(QLatin1String(".flatpak"))) {
            m_backend->addAppFromFlatpakBundle(fileUrl, m_stream);
        } else if (path.endsWith(QLatin1String(".flatpakref"))) {
            m_backend->addAppFromFlatpakRef(fileUrl, m_stream);
        } else if (path.endsWith(QLatin1String(".flatpakrepo"))) {
            m_backend->addSourceFromFlatpakRepo(fileUrl, m_stream);
        } else {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "unrecognized format" << fileUrl;
        }
    }

    FlatpakBackend *const m_backend;
    ResultsStream *const m_stream;
    const QUrl m_url;
};

FlatpakInstalledRef *FlatpakBackend::getInstalledRefForApp(const FlatpakResource *resource) const
{
    Q_ASSERT(resource->resourceType() != FlatpakResource::Source);
    g_autoptr(GError) localError = nullptr;

    const auto type = resource->resourceType() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME;

    auto ref = flatpak_installation_get_installed_ref(resource->installation(),
                                                      type,
                                                      resource->flatpakName().toUtf8().constData(),
                                                      resource->arch().toUtf8().constData(),
                                                      resource->branch().toUtf8().constData(),
                                                      m_cancellable,
                                                      &localError);
    return ref;
}

static QString refToBundleId(FlatpakRef *ref)
{
    const auto typeAsString = flatpak_ref_get_kind(ref) == FLATPAK_REF_KIND_APP ? QLatin1String("app") : QLatin1String("runtime");
    const QUtf8StringView flatpakName(flatpak_ref_get_name(ref));
    const QUtf8StringView arch(flatpak_ref_get_arch(ref));
    const QUtf8StringView branch(flatpak_ref_get_branch(ref));
    QString ret;
    ret.reserve(typeAsString.size() + flatpakName.size() + arch.size() + branch.size() + 3 /*slashes*/);
    ret += typeAsString;
    ret += QLatin1Char('/');
    ret += flatpakName;
    ret += QLatin1Char('/');
    ret += arch;
    ret += QLatin1Char('/');
    ret += branch;
    return ret;
}

FlatpakResource *FlatpakBackend::getAppForInstalledRef(FlatpakInstallation *installation, FlatpakInstalledRef *ref, bool *freshResource) const
{
    if (freshResource) {
        *freshResource = false;
    }
    if (!ref) {
        return nullptr;
    }
    const auto origin = QString::fromUtf8(flatpak_installed_ref_get_origin(ref));
    auto source = findSource(installation, origin);
    if (source) {
        if (auto resource = source->m_resources.value(idForInstalledRef(ref, {}))) {
            return resource;
        }
    }

    const QLatin1String name(flatpak_ref_get_name(FLATPAK_REF(ref)));
    const QLatin1String branch(flatpak_ref_get_branch(FLATPAK_REF(ref)));
    const QString pathExports = FlatpakResource::installationPath(installation) + QLatin1String("/exports/");
    const QString pathApps = pathExports + QLatin1String("share/applications/");
    const QString refId = refToBundleId(FLATPAK_REF(ref));
    AppStream::Component cid;
    if (source && source->m_pool) {
        auto components = source->componentsByFlatpakId(refId);
        if (components.isEmpty()) {
            g_autoptr(GBytes) metadata = flatpak_installed_ref_load_appdata(ref, m_cancellable, nullptr);
            if (metadata) {
                auto meta = metadataFromBytes(metadata, m_cancellable);
                components = meta->components();
            }
        }

        if (components.size() >= 1) {
            Q_ASSERT(components.size() == 1);
            cid = *components.indexSafe(0);
        }
    }

    if (!cid.isValid()) {
        AppStream::Metadata metadata;
        const QString fnDesktop = pathApps + name + QLatin1String(".desktop");
        AppStream::Metadata::MetadataError error = metadata.parseFile(fnDesktop, AppStream::Metadata::FormatKindDesktopEntry);
        if (error != AppStream::Metadata::MetadataErrorNoError) {
            if (QFile::exists(fnDesktop)) {
                qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to parse appstream metadata:" << error << fnDesktop;
            }

            cid.setId(name);
#if FLATPAK_CHECK_VERSION(1, 1, 2)
            cid.setName(QString::fromUtf8(flatpak_installed_ref_get_appdata_name(ref)));
#endif
        } else {
            cid = metadata.component();
        }
    }

    if (cid.bundle(AppStream::Bundle::KindFlatpak).isEmpty()) {
        AppStream::Bundle b;
        b.setKind(AppStream::Bundle::KindFlatpak);
        b.setId(refId);
        cid.addBundle(b);
    }

    if (source && cid.isValid()) {
        if (auto resource = source->m_resources.value(idForComponent(cid))) {
            return resource;
        }
    }

    if (!source) {
        return nullptr;
    }

    auto resource = new FlatpakResource(cid, source->installation(), const_cast<FlatpakBackend *>(this));
    resource->setOrigin(source->name());
    resource->setDisplayOrigin(source->title());
    resource->setIconPath(pathExports);
    resource->updateFromRef(FLATPAK_REF(ref));
    resource->setState(AbstractResource::Installed);
    source->addResource(resource);

    if (freshResource) {
        *freshResource = true;
    }

    Q_ASSERT_X(
        [&] {
            if (resource->uniqueId() == idForInstalledRef(ref) || resource->uniqueId() == idForInstalledRef(ref, QStringLiteral(".desktop"))) {
                return true;
            }
            // When an app changes appstream ID we may have the old installed but resolve the regular ids will not match.
            // What we need to do is collect all possible IDs for the component and see if any matches.
            QStringList cidIds = {cid.id()};
            const auto provided = cid.provided();
            for (const auto &provide : provided) {
                if (provide.kind() == AppStream::Provided::KindId) {
                    cidIds += provide.items();
                }
            }
            return std::ranges::any_of(cidIds, [&resource](const QString &cidId) {
                const auto id = resource->uniqueId().id;
                return id == cidId || id == cidId + ".desktop"_L1;
            });
        }(),
        "getAppForInstalledRef",
        flatpak_ref_format_ref_cached(FLATPAK_REF(ref)));
    return resource;
}

QSharedPointer<FlatpakSource> FlatpakBackend::findSource(FlatpakInstallation *installation, const QString &origin) const
{
    for (const auto &source : m_flatpakSources) {
        if (source->installation() == installation && source->name() == origin) {
            return source;
        }
    }
    for (const auto &source : m_flatpakLoadingSources) {
        if (source->installation() == installation && source->name() == origin) {
            return source;
        }
    }

    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Could not find source:" << installation << origin;
    return {};
}

QSharedPointer<FlatpakSource> FlatpakBackend::findSourceUrl(FlatpakInstallation *installation, const QString &url) const
{
    for (const auto &source : m_flatpakSources) {
        if (!source->remote()) {
            continue;
        }
        g_autofree char *remoteUrl = flatpak_remote_get_url(source->remote());
        if (source->installation() == installation && QUtf8StringView(remoteUrl) == url) {
            return source;
        }
    }
    for (const auto &source : m_flatpakLoadingSources) {
        if (!source->remote()) {
            continue;
        }
        g_autofree char *remoteUrl = flatpak_remote_get_url(source->remote());
        if (source->installation() == installation && QUtf8StringView(remoteUrl) == url) {
            return source;
        }
    }
    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Could not find source by url:" << url;
    return {};
}

FlatpakResource *FlatpakBackend::getRuntimeForApp(FlatpakResource *resource) const
{
    FlatpakResource *runtime = nullptr;
    const QString runtimeName = resource->runtime();
    const auto runtimeInfo = QStringView(runtimeName).split(QLatin1Char('/'));

    if (runtimeInfo.count() != 3) {
        return runtime;
    }

    for (const auto &source : m_flatpakSources) {
        for (const auto &[id, resource] : source->m_resources.asKeyValueRange()) {
            if (resource->resourceType() == FlatpakResource::Runtime && id.id == runtimeInfo.at(0) && id.branch == runtimeInfo.at(2)) {
                runtime = resource;
                break;
            }
        }
    }

    for (auto installation : m_installations) {
        if (auto iref = flatpak_installation_get_installed_ref(installation,
                                                               FLATPAK_REF_KIND_RUNTIME,
                                                               runtimeInfo.at(0).toUtf8().constData(),
                                                               runtimeInfo.at(1).toUtf8().constData(),
                                                               runtimeInfo.at(2).toUtf8().constData(),
                                                               m_cancellable,
                                                               nullptr)) {
            return getAppForInstalledRef(installation, iref);
        }
    }

    // TODO if runtime wasn't found, create a new one from available info
    if (!runtime) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not find runtime" << runtimeName << resource;
    }

    return runtime;
}

void FlatpakBackend::addAppFromFlatpakBundle(const QUrl &url, ResultsStream *stream)
{
    auto x = qScopeGuard([stream] {
        stream->finish();
    });

    if (m_localSource) {
        const auto it = std::ranges::find_if(m_localSource->m_resources, [url](auto resource) {
            return resource->url() == url;
        });
        if (it != m_localSource->m_resources.end()) {
            Q_EMIT stream->resourcesFound({it.value()});
            return;
        }
    }

    g_autoptr(GBytes) appstreamGz = nullptr;
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GFile) file = nullptr;
    g_autoptr(FlatpakBundleRef) bundleRef = nullptr;
    AppStream::Component asComponent;

    file = g_file_new_for_path(url.toLocalFile().toUtf8().constData());
    bundleRef = flatpak_bundle_ref_new(file, &localError);

    if (!bundleRef) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to load bundle:" << localError->message;
        return;
    }

    gsize len = 0;
    g_autoptr(GBytes) metadata = flatpak_bundle_ref_get_metadata(bundleRef);
    const QByteArray metadataContent((char *)g_bytes_get_data(metadata, &len));

    appstreamGz = flatpak_bundle_ref_get_appstream(bundleRef);
    if (appstreamGz) {
        const auto metadata = metadataFromBytes(appstreamGz, m_cancellable);
        if (!metadata.has_value()) {
            return;
        }

        if (std::optional<AppStream::Component> firstComponent = metadata->components().indexSafe(0); firstComponent.has_value()) {
            asComponent = *firstComponent;
        } else {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to parse appstream metadata";
            return;
        }
    } else {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "No appstream metadata in bundle";

        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        if (!tempFile.open()) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get metadata file";
            return;
        }

        tempFile.write(metadataContent);
        tempFile.close();

        // Parse the temporary file
        QSettings setting(tempFile.fileName(), QSettings::NativeFormat);
        setting.beginGroup(QLatin1String("Application"));

        asComponent.setName(setting.value(QLatin1String("name")).toString());

        tempFile.remove();
    }

    g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs(preferredInstallation(), m_cancellable, &localError);
    if (!refs) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get list of installed refs for listing local updates:" << localError->message;
        return;
    }

    for (uint i = 0; i < refs->len; i++) {
        FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
        FlatpakInstalledRef *iref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));
        if (qstrcmp(flatpak_ref_get_commit(ref), flatpak_ref_get_commit(FLATPAK_REF(bundleRef))) == 0) {
            if (auto resource = getAppForInstalledRef(preferredInstallation(), iref, nullptr)) {
                Q_EMIT stream->resourcesFound({resource});
            }
            return;
        }
    }

    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    if (!updateAppMetadata(resource, metadataContent)) {
        delete resource;
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to update metadata from app bundle";
        return;
    }

    g_autoptr(GBytes) iconData = flatpak_bundle_ref_get_icon(bundleRef, 128);
    if (!iconData) {
        iconData = flatpak_bundle_ref_get_icon(bundleRef, 64);
    }

    if (iconData) {
        gsize len = 0;
        char *data = (char *)g_bytes_get_data(iconData, &len);

        QPixmap pixmap;
        pixmap.loadFromData(QByteArray(data, len), "PNG");
        resource->setBundledIcon(pixmap);
    }

    const auto origin = QString::fromUtf8(flatpak_bundle_ref_get_origin(bundleRef));
    resource->updateFromRef(FLATPAK_REF(bundleRef));
    resource->setDownloadSize(0);
    resource->setInstalledSize(flatpak_bundle_ref_get_installed_size(bundleRef));
    resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::AlreadyKnown);
    resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::AlreadyKnown);
    resource->setFlatpakFileType(FlatpakResource::FileFlatpak);
    resource->setOrigin(origin.isEmpty() ? i18n("Local bundle") : origin);
    resource->setResourceFile(url);
    resource->setState(FlatpakResource::None);

    if (!m_localSource) {
        m_localSource.reset(new FlatpakSource(this, preferredInstallation()));
        m_flatpakSources += m_localSource;
    }
    m_localSource->addResource(resource);
    Q_EMIT stream->resourcesFound({resource});
}

QString composeRef(bool isRuntime, const QString &name, const QString &branch)
{
    return (isRuntime ? "runtime/"_L1 : "app/"_L1) + name + '/'_L1 + QString::fromUtf8(flatpak_get_default_arch()) + '/'_L1 + branch;
}

AppStream::Component fetchComponentFromRemote(const QSettings &settings, GCancellable *cancellable)
{
    const QString name = settings.value(QLatin1StringView("Flatpak Ref/Name")).toString();
    const QString branch = settings.value(QLatin1StringView("Flatpak Ref/Branch")).toString();
    const QString remoteName = settings.value(QLatin1StringView("Flatpak Ref/SuggestRemoteName")).toString();
    const bool isRuntime = settings.value(QLatin1StringView("Flatpak Ref/IsRuntime")).toBool();

    AppStream::Component asComponent;
    asComponent.addUrl(AppStream::Component::UrlKindHomepage, settings.value(QLatin1StringView("Flatpak Ref/Homepage")).toString());
    asComponent.setDescription(settings.value(QLatin1StringView("Flatpak Ref/Description")).toString());
    asComponent.setName(settings.value(QLatin1StringView("Flatpak Ref/Title")).toString());
    asComponent.setSummary(settings.value(QLatin1StringView("Flatpak Ref/Comment")).toString());
    asComponent.setId(name);

    AppStream::Bundle b;
    b.setKind(AppStream::Bundle::KindFlatpak);
    b.setId(composeRef(isRuntime, asComponent.name(), branch));
    asComponent.addBundle(b);

    // We are going to create a temporary installation and add the remote to it.
    // There we will fetch the appstream metadata and then delete that temporary installation.

    g_autoptr(GError) localError = nullptr;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-temporary-") + remoteName;
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Creating temporary installation" << path;
    g_autoptr(GFile) file = g_file_new_for_path(QFile::encodeName(path).constData());
    g_autoptr(FlatpakInstallation) tempInstallation = flatpak_installation_new_for_path(file, true, cancellable, &localError);
    if (!tempInstallation) {
        return asComponent;
    }
    auto x = qScopeGuard([path] {
        QDir(path).removeRecursively();
    });

    g_autoptr(FlatpakRemote) tempRemote = flatpak_remote_new(remoteName.toUtf8().constData());
    populateRemote(tempRemote,
                   remoteName,
                   settings.value(QLatin1StringView("Flatpak Ref/Url")).toString(),
                   settings.value(QLatin1StringView("Flatpak Ref/GPGKey")).toString());
    if (!flatpak_installation_modify_remote(tempInstallation, tempRemote, cancellable, &localError)) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "error adding temporary remote" << localError->message;
        return asComponent;
    }

    auto cb = [](const char *status, guint progress, gboolean /*estimating*/, gpointer /*user_data*/) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Progress..." << status << progress;
    };

    gboolean changed;
    if (!flatpak_installation_update_appstream_full_sync(tempInstallation,
                                                         remoteName.toUtf8().constData(),
                                                         nullptr,
                                                         cb,
                                                         nullptr,
                                                         &changed,
                                                         cancellable,
                                                         &localError)) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "error fetching appstream" << localError->message;
        return asComponent;
    }
    Q_ASSERT(changed);
    const QString appstreamLocation = path + "/appstream/"_L1 + remoteName + '/'_L1 + QString::fromUtf8(flatpak_get_default_arch()) + "/active"_L1;

    AppStream::Pool pool;
    pool.setLoadStdDataLocations(false);
    pool.addExtraDataLocation(appstreamLocation, AppStream::Metadata::FormatStyleCatalog);

    if (!pool.load()) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "error loading pool" << pool.lastError();
        return asComponent;
    }

    // TODO optimise, this lookup should happen in libappstream
    auto comps = pool.components();
    kFilterInPlace<AppStream::ComponentBox>(comps, [name, branch](const AppStream::Component &component) {
        const QString id = component.bundle(AppStream::Bundle::KindFlatpak).id();
        // app/app.getspace.Space/x86_64/stable
        return id.section(QLatin1Char('/'), 1, 1) == name && (branch.isEmpty() || id.section(QLatin1Char('/'), 3, 3) == branch);
    });
    if (comps.isEmpty()) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not find" << name << "in" << remoteName;
        return asComponent;
    }
    return *comps.indexSafe(0);
}

void FlatpakBackend::addAppFromFlatpakRef(const QUrl &url, ResultsStream *stream)
{
    Q_ASSERT(url.isLocalFile());
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);
    const QString refurl = settings.value(QLatin1StringView("Flatpak Ref/Url")).toString();
    const QString name = settings.value(QLatin1StringView("Flatpak Ref/Name")).toString();
    const QString remoteName = settings.value(QLatin1StringView("Flatpak Ref/SuggestRemoteName")).toString();
    const QString branch = settings.value(QLatin1StringView("Flatpak Ref/Branch")).toString();
    const bool isRuntime = settings.value(QLatin1StringView("Flatpak Ref/IsRuntime")).toBool();
    g_autoptr(GError) error = nullptr;

    // If we already added the remote, just go with it
    if (auto source = findSourceUrl(preferredInstallation(), refurl)) {
        const QString ref = composeRef(isRuntime, name, branch);
        auto searchComponent = [this, stream, source, ref] {
            auto components = source->componentsByFlatpakId(ref);
            auto resources = kTransform<QVector<StreamResult>>(components, [this, source](const auto &component) {
                return resourceForComponent(component, source);
            });
            Q_EMIT stream->resourcesFound(resources);
            stream->finish();
        };
        if (source->m_pool) {
            QTimer::singleShot(0, this, searchComponent);
        } else {
            connect(this, &FlatpakBackend::initialized, stream, searchComponent);
        }
        return;
    }

    AppStream::Component asComponent = fetchComponentFromRemote(settings, m_cancellable);
    const QString iconUrl = settings.value(QLatin1StringView("Flatpak Ref/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AppStream::Icon icon;
        icon.setKind(AppStream::Icon::KindRemote);
        icon.setUrl(QUrl(iconUrl));
        asComponent.addIcon(icon);
    }

    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    resource->setFlatpakFileType(FlatpakResource::FileFlatpakRef);
    resource->setResourceFile(url);
    resource->setResourceLocation(QUrl(refurl));
    resource->setOrigin(remoteName);
    resource->setDisplayOrigin(remoteName);
    resource->setFlatpakName(name);
    resource->setArch(QString::fromUtf8(flatpak_get_default_arch()));
    resource->setBranch(branch);
    resource->setType(isRuntime ? FlatpakResource::Runtime : FlatpakResource::DesktopApp);

    QUrl runtimeUrl = QUrl(settings.value(QLatin1StringView("Flatpak Ref/RuntimeRepo")).toString());
    auto refSource = QSharedPointer<FlatpakSource>::create(this, preferredInstallation());
    resource->setTemporarySource(refSource);
    m_flatpakSources += refSource;
    if (!runtimeUrl.isEmpty()) {
        // We need to fetch metadata to find information about required runtime
        auto fw = new QFutureWatcher<QByteArray>(this);
        connect(fw, &QFutureWatcher<QByteArray>::finished, this, [this, resource, fw, runtimeUrl, stream, refSource]() {
            fw->deleteLater();
            const auto metadata = fw->result();
            // Even when we failed to fetch information about runtime we still want to show the application
            if (metadata.isEmpty()) {
                onFetchMetadataFinished(resource, metadata);
            } else {
                updateAppMetadata(resource, metadata);

                auto runtime = getRuntimeForApp(resource);
                if (!runtime || (runtime && !runtime->isInstalled())) {
                    auto repoStream = new ResultsStream(QLatin1String("FlatpakStream-searchrepo-") + runtimeUrl.toString());
                    connect(repoStream, &ResultsStream::resourcesFound, this, [this, resource, stream, refSource](const QVector<StreamResult> &resources) {
                        for (auto res : resources) {
                            installApplication(res.resource);
                        }
                        refSource->addResource(resource);
                        Q_EMIT stream->resourcesFound({resource});
                        stream->finish();
                    });

                    auto fetchRemoteResource = new FlatpakFetchRemoteResourceJob(runtimeUrl, repoStream, this);
                    fetchRemoteResource->start();
                    return;
                } else {
                    refSource->addResource(resource);
                }
            }
            Q_EMIT stream->resourcesFound({resource});
            stream->finish();
        });
        fw->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchMetadata, resource, m_cancellable));
    } else {
        refSource->addResource(resource);
        Q_EMIT stream->resourcesFound({resource});
        stream->finish();
    }
}

void FlatpakBackend::addSourceFromFlatpakRepo(const QUrl &url, ResultsStream *stream)
{
    auto x = qScopeGuard([stream] {
        stream->finish();
    });
    Q_ASSERT(url.isLocalFile());
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);

    const QString gpgKey = settings.value(QLatin1StringView("Flatpak Repo/GPGKey")).toString();
    const QString title = settings.value(QLatin1StringView("Flatpak Repo/Title")).toString();
    const QString repoUrl = settings.value(QLatin1StringView("Flatpak Repo/Url")).toString();

    if (gpgKey.isEmpty() || title.isEmpty() || repoUrl.isEmpty()) {
        return;
    }

    if (gpgKey.startsWith(QLatin1String("http://")) || gpgKey.startsWith(QLatin1String("https://"))) {
        return;
    }

    AppStream::Component asComponent;
    asComponent.addUrl(AppStream::Component::UrlKindHomepage, settings.value(QLatin1StringView("Flatpak Repo/Homepage")).toString());
    asComponent.setSummary(settings.value(QLatin1StringView("Flatpak Repo/Comment")).toString());
    asComponent.setDescription(settings.value(QLatin1StringView("Flatpak Repo/Description")).toString());
    asComponent.setName(title);
    asComponent.setId(settings.value(QLatin1StringView("Flatpak Repo/Title")).toString());

    const QString iconUrl = settings.value(QLatin1StringView("Flatpak Repo/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AppStream::Icon icon;
        icon.setKind(AppStream::Icon::KindRemote);
        icon.setUrl(QUrl(iconUrl));
        asComponent.addIcon(icon);
    }

    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    // Use metadata only for stuff which are not common for all resources
    resource->addMetadata(QLatin1StringView("gpg-key"), gpgKey);
    resource->addMetadata(QLatin1StringView("repo-url"), repoUrl);
    resource->setBranch(settings.value(QLatin1StringView("Flatpak Repo/DefaultBranch")).toString());
    resource->setFlatpakName(url.fileName().remove(QLatin1StringView(".flatpakrepo")));
    resource->setType(FlatpakResource::Source);

    g_autoptr(FlatpakRemote) repo =
        flatpak_installation_get_remote_by_name(preferredInstallation(), resource->flatpakName().toUtf8().constData(), m_cancellable, nullptr);
    if (!repo) {
        resource->setState(AbstractResource::State::None);
    } else {
        resource->setState(AbstractResource::State::Installed);
    }

    Q_EMIT stream->resourcesFound({resource});
}

void FlatpakBackend::loadAppsFromAppstreamData()
{
    for (auto installation : std::as_const(m_installations)) {
        // Load applications from appstream metadata
        if (g_cancellable_is_cancelled(m_cancellable)) {
            break;
        }

        if (!loadAppsFromAppstreamData(installation)) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to load packages from appstream data from installation" << installation;
        }
    }
}

bool FlatpakBackend::loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation)
{
    Q_ASSERT(flatpakInstallation);

    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) remotes = flatpak_installation_list_remotes(flatpakInstallation, m_cancellable, &error);
    if (!remotes) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed to list remotes" << error->message;
        return false;
    }

    for (uint i = 0; i < remotes->len; i++) {
        auto remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));
        loadRemote(flatpakInstallation, remote);
    }
    return true;
}

void FlatpakBackend::loadRemote(FlatpakInstallation *installation, FlatpakRemote *remote)
{
    g_autoptr(GFile) fileTimestamp = flatpak_remote_get_appstream_timestamp(remote, flatpak_get_default_arch());

    Q_ASSERT(!m_refreshAppstreamMetadataJobs.contains(remote));
    m_refreshAppstreamMetadataJobs.insert(remote);

    g_autofree char *path_str = g_file_get_path(fileTimestamp);
    QFileInfo fileInfo(QFile::decodeName(path_str));
    if (!fileInfo.exists() || fileInfo.lastModified().toUTC().secsTo(QDateTime::currentDateTimeUtc()) > 21600) {
        // Refresh appstream metadata in case they have never been refreshed or the cache is older than 6 hours
        checkForRemoteUpdates(installation, remote);
    } else {
        auto source = integrateRemote(installation, remote);
        Q_ASSERT(findSource(installation, QString::fromUtf8(flatpak_remote_get_name(remote))) == source);
    }
}

void FlatpakBackend::unloadRemote(FlatpakInstallation *installation, FlatpakRemote *remote)
{
    acquireFetching(true);
    for (auto it = m_flatpakSources.begin(); it != m_flatpakSources.end();) {
        if ((*it)->url() == copyAndFree(flatpak_remote_get_url(remote)) && (*it)->installation() == installation) {
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "unloading remote" << (*it) << remote;
            it = m_flatpakSources.erase(it);
        } else {
            ++it;
        }
    }
    acquireFetching(false);
}

void FlatpakBackend::metadataRefreshed(FlatpakRemote *remote)
{
    const bool removed = m_refreshAppstreamMetadataJobs.remove(remote);
    Q_ASSERT(removed);
}

void FlatpakBackend::createPool(QSharedPointer<FlatpakSource> source)
{
    if (source->m_pool) {
        if (m_refreshAppstreamMetadataJobs.contains(source->remote())) {
            metadataRefreshed(source->remote());
        }
        return;
    }

    const QString appstreamDirPath = source->appstreamDir();
    if (!QFile::exists(appstreamDirPath)) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "No" << appstreamDirPath << "appstream metadata found for" << source->name();
        metadataRefreshed(source->remote());
        return;
    }

    AppStream::Pool *pool = new AppStream::Pool;
    acquireFetching(true);

    pool->setLoadStdDataLocations(false);
    pool->addExtraDataLocation(appstreamDirPath, AppStream::Metadata::FormatStyleCatalog);

    const auto loadDone = [this, source, pool](bool result) {
        source->m_pool.reset(new AppStream::ConcurrentPool);
        source->m_pool->reset(pool, &m_threadPool);
        m_flatpakLoadingSources.removeAll(source);
        if (result) {
            m_flatpakSources += source;
        } else {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Could not open the AppStream metadata pool" << pool->lastError();
        }
        metadataRefreshed(source->remote());
        acquireFetching(false);
    };

    connect(pool, &AppStream::Pool::loadFinished, this, [this, loadDone](bool success) {
        // We do not want to block the GUI for very long, so we load appstream pools via queued
        // invocation for the post-load state changes.
        QMetaObject::invokeMethod(
            this,
            [loadDone, success] {
                loadDone(success);
            },
            Qt::QueuedConnection);
    });
    pool->loadAsync();
}

QSharedPointer<FlatpakSource> FlatpakBackend::integrateRemote(GLibHolder<FlatpakInstallation> flatpakInstallation, GLibHolder<FlatpakRemote> remote)
{
    Q_ASSERT(m_refreshAppstreamMetadataJobs.contains(remote.get()));
    m_sources->addRemote(remote.get(), flatpakInstallation.get());
    const auto matchRemote = [this, flatpakInstallation, remote](const auto &sources) -> QSharedPointer<FlatpakSource> {
        for (auto source : sources) {
            if (source->url() == copyAndFree(flatpak_remote_get_url(remote.get())) && source->installation() == flatpakInstallation.get()
                && source->name() == QString::fromUtf8(flatpak_remote_get_name(remote.get()))) {
                createPool(source);
                if (source->remote() != remote.get()) {
                    m_refreshAppstreamMetadataJobs.remove(remote.get());
                }
                return source;
            }
        }
        return {};
    };
    if (auto source = matchRemote(m_flatpakSources)) {
        return source;
    }
    if (auto source = matchRemote(m_flatpakLoadingSources)) {
        return source;
    }

    auto source = QSharedPointer<FlatpakSource>::create(this, flatpakInstallation.get(), remote.get());
    if (!source->isEnabled() || flatpak_remote_get_noenumerate(remote.get())) {
        m_flatpakSources += source;
        metadataRefreshed(remote.get());
        return source;
    }

    createPool(source);
    m_flatpakLoadingSources << source;
    return source;
}

void FlatpakBackend::loadLocalUpdates()
{
    acquireFetching(true);
    for (auto installation : std::as_const(m_installations)) {
        if (g_cancellable_is_cancelled(m_cancellable)) {
            break;
        }
        loadLocalUpdates(installation);
    }
    acquireFetching(false);
}
void FlatpakBackend::loadLocalUpdates(FlatpakInstallation *flatpakInstallation)
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs(flatpakInstallation, m_cancellable, &localError);
    if (!refs) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get list of installed refs for listing local updates:" << localError->message;
        return;
    }

    for (uint i = 0; i < refs->len && !g_cancellable_is_cancelled(m_cancellable); i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));

        const gchar *latestCommit = flatpak_installed_ref_get_latest_commit(ref);

        if (!latestCommit) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Couldn't get latest commit for" << flatpak_ref_get_name(FLATPAK_REF(ref));
            continue;
        }

        const gchar *commit = flatpak_ref_get_commit(FLATPAK_REF(ref));
        if (g_strcmp0(commit, latestCommit) == 0) {
            continue;
        }

        auto resource = getAppForInstalledRef(flatpakInstallation, ref);
        if (resource) {
            resource->setState(AbstractResource::Upgradeable);
            updateAppSize(resource);
        }
        Q_ASSERT(!resource->temporarySource());
    }
}

bool FlatpakBackend::setupFlatpakInstallations(GError **error)
{
    if (qEnvironmentVariableIsSet("FLATPAK_TEST_MODE")) {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test");
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "running flatpak backend on test mode" << path;
        g_autoptr(GFile) file = g_file_new_for_path(QFile::encodeName(path).constData());
        m_installations << flatpak_installation_new_for_path(file, true, m_cancellable, error);
        return m_installations.constLast() != nullptr;
    }

    g_autoptr(GPtrArray) installations = flatpak_get_system_installations(m_cancellable, error);
    if (*error) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to call flatpak_get_system_installations:" << (*error)->message;
    }
    for (uint i = 0; installations && i < installations->len; i++) {
        auto installation = FLATPAK_INSTALLATION(g_ptr_array_index(installations, i));
        g_object_ref(installation);
        m_installations << installation;
    }

    if (auto user = flatpak_installation_new_user(m_cancellable, error)) {
        m_installations << user;
    }

    return !m_installations.isEmpty();
}

void FlatpakBackend::updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource)
{
    // Update the rest
    resource->updateFromRef(FLATPAK_REF(installedRef));
    resource->setInstalledSize(flatpak_installed_ref_get_installed_size(installedRef));
    resource->setOrigin(QString::fromUtf8(flatpak_installed_ref_get_origin(installedRef)));
    if (resource->state() < AbstractResource::Installed) {
        resource->setState(AbstractResource::Installed);
    }
}

bool FlatpakBackend::updateAppMetadata(FlatpakResource *resource)
{
    if (resource->resourceType() != FlatpakResource::DesktopApp) {
        return true;
    }

    const QString path = resource->installPath() + QLatin1StringView("/metadata");

    if (QFile::exists(path)) {
        return updateAppMetadata(resource, path);
    } else {
        auto fw = new QFutureWatcher<QByteArray>(this);
        connect(fw, &QFutureWatcher<QByteArray>::finished, this, [this, resource, fw]() {
            const auto metadata = fw->result();
            if (!metadata.isEmpty()) {
                onFetchMetadataFinished(resource, metadata);
            }
            fw->deleteLater();
        });
        fw->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchMetadata, resource, m_cancellable));

        // Return false to indicate we cannot continue (right now used only in updateAppSize())
        return false;
    }
}

void FlatpakBackend::onFetchMetadataFinished(FlatpakResource *resource, const QByteArray &metadata)
{
    updateAppMetadata(resource, metadata);

    // Right now we attempt to update metadata for calculating the size so call updateSizeFromRemote()
    // as it's what we want. In future if there are other reason to update metadata we will need to somehow
    // distinguish between these calls
    updateAppSizeFromRemote(resource);
}

bool FlatpakBackend::updateAppMetadata(FlatpakResource *resource, const QString &path)
{
    // Parse the temporary file
    QSettings setting(path, QSettings::NativeFormat);
    setting.beginGroup(QLatin1String("Application"));
    // Set the runtime in form of name/arch/version which can be later easily parsed
    resource->setRuntime(setting.value(QLatin1String("runtime")).toString());
    // TODO get more information?
    return true;
}

bool FlatpakBackend::updateAppMetadata(FlatpakResource *resource, const QByteArray &data)
{
    // We just find the runtime with a regex, QSettings only can read from disk (and so does KConfig)
    static const QRegularExpression rx(QStringLiteral("runtime=(.*)"));
    const auto match = rx.match(QString::fromUtf8(data));
    if (!match.isValid()) {
        return false;
    }

    resource->setRuntime(match.captured(1));
    return true;
}

bool FlatpakBackend::updateAppSize(FlatpakResource *resource)
{
    // Check if the size is already set, we should also distinguish between download and installed size,
    // right now it doesn't matter whether we get size for installed or not installed app, but if we
    // start making difference then for not installed app check download and install size separately

    if (resource->state() == AbstractResource::Installed) {
        // The size appears to be already set (from updateAppInstalledMetadata() apparently)
        if (resource->installedSize() > 0) {
            return true;
        }
    } else {
        if (resource->installedSize() > 0 && resource->downloadSize() > 0) {
            return true;
        }
    }

    // Check if we know the needed runtime which is needed for calculating the size
    if (resource->runtime().isEmpty()) {
        if (!updateAppMetadata(resource)) {
            return false;
        }
    }

    return updateAppSizeFromRemote(resource);
}

bool FlatpakBackend::updateAppSizeFromRemote(FlatpakResource *resource)
{
    // Calculate the runtime size
    if (resource->state() == AbstractResource::None && resource->resourceType() == FlatpakResource::DesktopApp) {
        auto runtime = getRuntimeForApp(resource);
        if (runtime) {
            // Re-check runtime state if case a new one was created
            updateAppState(runtime);

            if (!runtime->isInstalled()) {
                if (!updateAppSize(runtime)) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get runtime size needed for total size of" << resource->name();
                    return false;
                }
                // Set required download size to include runtime size even now, in case we fail to
                // get the app size (e.g. when installing bundles where download size is 0)
                resource->setDownloadSize(runtime->downloadSize());
            }
        }
    }

    if (resource->state() == AbstractResource::Installed) {
        g_autoptr(FlatpakInstalledRef) ref = nullptr;
        ref = getInstalledRefForApp(resource);
        if (!ref) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get installed size of" << resource->name();
            return false;
        }
        resource->setInstalledSize(flatpak_installed_ref_get_installed_size(ref));
    } else if (resource->resourceType() != FlatpakResource::Source) {
        if (resource->origin().isEmpty()) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get size of" << resource->name() << " because of missing origin";
            return false;
        }

        if (resource->propertyState(FlatpakResource::DownloadSize) == FlatpakResource::Fetching) {
            return true;
        }

        auto futureWatcher = new QFutureWatcher<FlatpakRemoteRef *>(this);
        connect(futureWatcher, &QFutureWatcher<FlatpakRemoteRef *>::finished, this, [this, resource, futureWatcher]() {
            g_autoptr(FlatpakRemoteRef) remoteRef = futureWatcher->result();
            if (remoteRef) {
                onFetchSizeFinished(resource, flatpak_remote_ref_get_download_size(remoteRef), flatpak_remote_ref_get_installed_size(remoteRef));
            } else {
                resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::UnknownOrFailed);
                resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::UnknownOrFailed);
            }
            futureWatcher->deleteLater();
        });
        resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::Fetching);
        resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::Fetching);

        futureWatcher->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::findRemoteRef, resource, m_cancellable));
    }

    return true;
}

void FlatpakBackend::onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize)
{
    FlatpakResource *runtime = nullptr;
    if (resource->state() == AbstractResource::None && resource->resourceType() == FlatpakResource::DesktopApp) {
        runtime = getRuntimeForApp(resource);
    }

    if (runtime && !runtime->isInstalled()) {
        resource->setDownloadSize(runtime->downloadSize() + downloadSize);
    } else {
        resource->setDownloadSize(downloadSize);
    }
    resource->setInstalledSize(installedSize);
}

void FlatpakBackend::updateAppState(FlatpakResource *resource)
{
    g_autoptr(FlatpakInstalledRef) ref = getInstalledRefForApp(resource);
    if (ref) {
        // If the app is installed, we can set information about commit, arch etc.
        updateAppInstalledMetadata(ref, resource);
    } else {
        // TODO check if the app is actually still available
        resource->setState(AbstractResource::None);
    }
}

void FlatpakBackend::acquireFetching(bool f)
{
    if (f) {
        m_isFetching++;
    } else {
        m_isFetching--;
    }

    if (!f && m_isFetching == 0) {
        Q_EMIT contentsChanged();
        Q_EMIT initialized();
    }
}

int FlatpakBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool FlatpakBackend::flatpakResourceLessThan(const StreamResult &left, const StreamResult &right) const
{
    if (left.sortScore == right.sortScore) {
        return flatpakResourceLessThan(left.resource, right.resource);
    }
    return left.sortScore < right.sortScore;
}

bool FlatpakBackend::flatpakResourceLessThan(AbstractResource *left, AbstractResource *right) const
{
    if (left->isInstalled() != right->isInstalled()) {
        return left->isInstalled();
    }
    if (left->origin() != right->origin()) {
        return m_sources->originIndex(left->origin()) < m_sources->originIndex(right->origin());
    }

    const auto leftPoints = left->rating().ratingPoints();
    const auto rightPoints = right->rating().ratingPoints();
    if (leftPoints != rightPoints) {
        return leftPoints > rightPoints;
    }

    return left < right;
}

ResultsStream *FlatpakBackend::deferredResultStream(const QString &streamName, std::function<QCoro::Task<>(ResultsStream *)> callback)
{
    ResultsStream *stream = new ResultsStream(streamName);
    stream->setParent(this);

    // Don't capture variables into a coroutine lambda, pass them in as arguments instead
    // See https://devblogs.microsoft.com/oldnewthing/20211103-00/?p=105870
    [](FlatpakBackend *self, ResultsStream *stream, std::function<QCoro::Task<>(ResultsStream *)> callback) -> QCoro::Task<> {
        QPointer<ResultsStream> guard = stream;
        if (self->m_isFetching > 0) {
            co_await qCoro(self, &FlatpakBackend::initialized);
        } else {
            co_await QCoro::sleepFor(0ms);
        }
        if (guard.isNull()) {
            co_return;
        }
        co_await callback(stream);
        if (guard.isNull()) {
            co_return;
        }
        stream->finish();
    }(this, stream, std::move(callback));

    return stream;
}

ResultsStream *FlatpakBackend::deferredResultStreamNoFinish(const QString &streamName, std::function<QCoro::Task<>(ResultsStream *)> callback)
{
    ResultsStream *stream = new ResultsStream(streamName);
    stream->setParent(this);

    // Don't capture variables into a coroutine lambda, pass them in as arguments instead
    // See https://devblogs.microsoft.com/oldnewthing/20211103-00/?p=105870
    [](FlatpakBackend *self, ResultsStream *stream, std::function<QCoro::Task<>(ResultsStream *)> callback) -> QCoro::Task<> {
        QPointer<ResultsStream> guard = stream;
        if (self->m_isFetching > 0) {
            co_await qCoro(self, &FlatpakBackend::initialized);
        } else {
            co_await QCoro::sleepFor(0ms);
        }
        if (guard.isNull()) {
            co_return;
        }
        co_await callback(stream);
        if (guard.isNull()) {
            co_return;
        }
    }(this, stream, std::move(callback));

    return stream;
}

#define FLATPAK_BACKEND_GUARD                                                                                                                                  \
    QPointer<ResultsStream> guardStream(stream);                                                                                                               \
    g_autoptr(GCancellable) cancellable = g_object_ref(self->m_cancellable);                                                                                   \
    CoroutineSplitter coroutineSplitter;

#define FLATPAK_BACKEND_CHECK                                                                                                                                  \
    if (guardStream.isNull() || g_cancellable_is_cancelled(cancellable)) {                                                                                     \
        co_return;                                                                                                                                             \
    }

#define FLATPAK_BACKEND_YIELD                                                                                                                                  \
    co_await coroutineSplitter();                                                                                                                              \
    FLATPAK_BACKEND_CHECK

static bool isFlatpakSubRef(const QLatin1String &name)
{
    return name.endsWith(QLatin1String(".Debug")) || name.endsWith(QLatin1String(".Locale")) || name.endsWith(QLatin1String(".Docs"));
}

void triage(FlatpakResource *resource,
            QVector<StreamResult> &prioritary,
            QVector<StreamResult> &rest,
            const AbstractResourcesBackend::Filters &filter,
            bool filtered)
{
    const bool matchById = resource->appstreamId().compare(filter.search, Qt::CaseInsensitive) == 0;
    // Note: FlatpakResource can not have type == System
    if (resource->type() == AbstractResource::ApplicationSupport && filter.state != AbstractResource::Upgradeable && !matchById) {
        return;
    }

    if (resource->state() < filter.state) {
        return;
    }

    if (!filter.extends.isEmpty() && !resource->extends().contains(filter.extends)) {
        return;
    }

    if (!filter.mimetype.isEmpty() && !resource->mimetypes().contains(filter.mimetype)) {
        return;
    }

    if (filter.search.isEmpty() || matchById) {
        rest += resource;
    } else if (resource->name().contains(filter.search, Qt::CaseInsensitive)) {
        prioritary += resource;
    } else if (filtered || resource->comment().contains(filter.search, Qt::CaseInsensitive)) {
        rest += resource;
        // trust The search terms provided by appstream are relevant, this makes possible finding "gimp"
        // since the name() is "GNU Image Manipulation Program"
    } else if (resource->appstreamId().contains(filter.search, Qt::CaseInsensitive)) {
        rest += resource;
    }
}

int FlatpakBackend::fetchingUpdatesProgress() const
{
    return m_collector->progress();
}

ResultsStream *FlatpakBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    const auto fileName = filter.resourceUrl.fileName();
    if (filter.resourceUrl.scheme() == QLatin1String("flatpak+https")) {
        QUrl newUrl = filter.resourceUrl;
        newUrl.setScheme(QStringLiteral("https"));
        auto stream = new ResultsStream(QLatin1String("FlatpakStream-http-") + fileName);
        FlatpakFetchRemoteResourceJob *fetchResourceJob = new FlatpakFetchRemoteResourceJob(newUrl, stream, this);
        fetchResourceJob->start();
        return stream;
    } else if (fileName.endsWith(QLatin1String(".flatpakrepo")) || fileName.endsWith(QLatin1String(".flatpakref"))
               || fileName.endsWith(QLatin1String(".flatpak"))) {
        auto stream = new ResultsStream(QLatin1String("FlatpakStream-http-") + fileName);
        auto fetchResourceJob = new FlatpakFetchRemoteResourceJob(filter.resourceUrl, stream, this);
        fetchResourceJob->start();
        return stream;
    } else if (filter.resourceUrl.scheme() == QLatin1String("flatpak")) {
        return deferredResultStream(u"FlatpakStream-flatpak-ref"_s, [this, filter](ResultsStream *stream) -> QCoro::Task<> {
            return [](FlatpakBackend *self, ResultsStream *stream, const AbstractResourcesBackend::Filters filter) -> QCoro::Task<> {
                FLATPAK_BACKEND_GUARD
                const auto installations = self->m_installations;
                QVector<StreamResult> resources;
                g_autoptr(GError) error = nullptr;

                g_autoptr(FlatpakRef) ref = flatpak_ref_parse(filter.resourceUrl.path().toUtf8().constData(), &error);
                if (!ref) {
                    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed to parse ref:" << error->message;
                    co_return;
                }
                const auto refKind = flatpak_ref_get_kind(ref);
                const auto refName = flatpak_ref_get_name(ref);
                const auto refArch = flatpak_ref_get_arch(ref);
                const auto refBranch = flatpak_ref_get_branch(ref);

                // need to issue the search of installed refs too
                // it can happen that a ref is present because it's installed but not part of the appstream metadata anymore
                for (auto installation : std::as_const(installations)) {
                    g_autoptr(GError) localError = nullptr;
                    g_autoptr(FlatpakInstalledRef) installedRef =
                        flatpak_installation_get_installed_ref(installation, refKind, refName, refArch, refBranch, cancellable, &localError);
                    FLATPAK_BACKEND_YIELD
                    const auto resource = self->getAppForInstalledRef(installation, installedRef);
                    if (resource) {
                        resources.append(StreamResult(resource));
                    }
                }
                if (filter.state < AbstractResource::Installed) {
                    for (const auto &source : std::as_const(self->m_flatpakSources)) {
                        if (source->m_pool) {
                            auto components = source->componentsByFlatpakId(filter.resourceUrl.path());
                            resources.append(kTransform<QVector<StreamResult>>(components, [self, source](const AppStream::Component &comp) {
                                return StreamResult(self->resourceForComponent(comp, source), comp.sortScore());
                            }));
                        }
                    }

                    // make sure we are not adding duplicates from the installed results
                    kRemoveDuplicates(resources);
                }
                FLATPAK_BACKEND_YIELD

                if (!resources.isEmpty()) {
                    Q_EMIT stream->resourcesFound(resources);
                }
            }(this, stream, filter);
        });
    } else if (filter.resourceUrl.scheme() == QLatin1String("appstream")) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (!filter.resourceUrl.isEmpty()) {
        return new ResultsStream(QStringLiteral("FlatpakStream-void"), {});
    } else if (filter.state == AbstractResource::Upgradeable) {
        return deferredResultStream(u"FlatpakStream-upgradeable"_s, [this](ResultsStream *stream) -> QCoro::Task<> {
            return [](FlatpakBackend *self, ResultsStream *stream) -> QCoro::Task<> {
                FLATPAK_BACKEND_GUARD

                const auto ret = co_await self->listInstalledRefsForUpdate();

                FLATPAK_BACKEND_CHECK

                QVector<StreamResult> resources;
                for (const auto &[installation, refs] : ret.asKeyValueRange()) {
                    resources.reserve(resources.size() + refs.size());
                    for (const auto ref : refs) {
                        bool fresh = false;
                        const QLatin1String id(flatpak_ref_get_name(FLATPAK_REF(ref)));
                        if (isFlatpakSubRef(id)) {
                            const QByteArray parentId(id.constData(), id.lastIndexOf(QLatin1Char('.')));

                            g_autoptr(GError) appError = nullptr;
                            // We need to bruteforce the API as we don't know from here if ref's parent is an app or a runtime 🙄
                            auto parentRef = flatpak_installation_get_installed_ref(installation,
                                                                                    FLATPAK_REF_KIND_APP,
                                                                                    parentId.constData(),
                                                                                    flatpak_ref_get_arch(FLATPAK_REF(ref)),
                                                                                    flatpak_ref_get_branch(FLATPAK_REF(ref)),
                                                                                    cancellable,
                                                                                    &appError);

                            g_autoptr(GError) runtimeError = nullptr;
                            if (!parentRef) {
                                parentRef = flatpak_installation_get_installed_ref(installation,
                                                                                   FLATPAK_REF_KIND_RUNTIME,
                                                                                   parentId.constData(),
                                                                                   flatpak_ref_get_arch(FLATPAK_REF(ref)),
                                                                                   flatpak_ref_get_branch(FLATPAK_REF(ref)),
                                                                                   cancellable,
                                                                                   &runtimeError);
                            }

                            if (parentRef) {
                                if (auto resource = self->getAppForInstalledRef(installation, parentRef)) {
                                    resource->addRefToUpdate(flatpak_ref_format_ref_cached(FLATPAK_REF(parentRef)));
                                    if (!kContains(refs, [&parentId](auto ref) {
                                            return parentId == flatpak_ref_get_name(FLATPAK_REF(ref));
                                        })) {
                                        resources.append(resource);
                                    }
                                    continue;
                                }
                            } else {
                                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get parent ref" << parentId << "for subref" << id //
                                                                           << "\n  AppError:" << appError->message //
                                                                           << "\n  RuntimeError:" << runtimeError->message;
                            }
                        }
                        auto resource = self->getAppForInstalledRef(installation, ref, &fresh);
                        if (resource) {
                            resource->setState(AbstractResource::Upgradeable, !fresh);
                            self->updateAppSize(resource);
                            if (resource->resourceType() == FlatpakResource::Runtime) {
                                resources.prepend(resource);
                            } else {
                                resources.append(resource);
                            }
                        }

                        FLATPAK_BACKEND_YIELD
                    }

                    g_object_unref(installation);
                    for (const auto &ref : refs) {
                        g_object_unref(ref);
                    }
                }

                FLATPAK_BACKEND_CHECK

                if (!resources.isEmpty()) {
                    Q_EMIT stream->resourcesFound(resources);
                }
            }(this, stream);
        });
    } else if (filter.state == AbstractResource::Installed) {
        return deferredResultStream(u"FlatpakStream-installed"_s, [this, filter](ResultsStream *stream) -> QCoro::Task<> {
            return [](FlatpakBackend *self, ResultsStream *stream, const AbstractResourcesBackend::Filters filter) -> QCoro::Task<> {
                FLATPAK_BACKEND_GUARD
                const auto installations = self->m_installations;
                QVector<StreamResult> resources;

                for (auto installation : std::as_const(installations)) {
                    FLATPAK_BACKEND_YIELD

                    g_autoptr(GError) localError = nullptr;
                    g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs(installation, cancellable, &localError);
                    if (!refs) {
                        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get list of installed refs for listing installed:" << localError->message;
                        continue;
                    }

                    resources.reserve(resources.size() + refs->len);
                    for (uint i = 0; i < refs->len; i++) {
                        FLATPAK_BACKEND_YIELD;

                        FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
                        if (isFlatpakSubRef(QLatin1String(flatpak_ref_get_name(ref)))) {
                            continue;
                        }

                        FlatpakInstalledRef *iref = FLATPAK_INSTALLED_REF(ref);
                        auto resource = self->getAppForInstalledRef(installation, iref);
                        if (!resource) {
                            continue;
                        }
                        if (!filter.search.isEmpty() && !resource->name().contains(filter.search, Qt::CaseInsensitive)
                            && !resource->appstreamId().contains(filter.search, Qt::CaseInsensitive)) {
                            continue;
                        }
                        if (resource->resourceType() == FlatpakResource::Runtime) {
                            resources.prepend(resource);
                        } else {
                            resources.append(resource);
                        }
                    }
                }

                FLATPAK_BACKEND_CHECK

                if (!resources.isEmpty()) {
                    Q_EMIT stream->resourcesFound(resources);
                }
            }(this, stream, filter);
        });
    } else {
        // Multithreading is nearly impossible for this stream, since child
        // objects are created and interact with this backend object. So the
        // task is split into hunks, interleaved by zero timers.
        return deferredResultStreamNoFinish(u"FlatpakStream"_s, [this, filter](ResultsStream *stream) -> QCoro::Task<> {
            return [](FlatpakBackend *self, ResultsStream *stream, const AbstractResourcesBackend::Filters filter) -> QCoro::Task<> {
                FLATPAK_BACKEND_GUARD
                const auto flatpakSources = self->m_flatpakSources;
                QVector<StreamResult> prioritary, rest;
                QMap<QSharedPointer<FlatpakSource>, QFuture<AppStream::ComponentBox>> futures;
                QList<FlatpakResource *> unpooled;

                for (const auto &source : flatpakSources) {
                    if (source->m_pool) {
                        if (!filter.search.isEmpty()) {
                            futures.insert(source, source->m_pool->search(filter.search));
                        } else if (filter.category) {
                            futures.insert(source,
                                           AppStreamUtils::componentsByCategoriesTask(source->m_pool.get(), filter.category, AppStream::Bundle::KindFlatpak));
                        } else {
                            futures.insert(source, source->m_pool->components());
                        }
                    } else {
                        unpooled += source->m_resources.values();
                    }
                }
                using QFS = QFutureWatcher<QList<QFuture<AppStream::ComponentBox>>>;
                auto *fw = new QFS(stream);
                fw->setFuture(QtFuture::whenAll(futures.begin(), futures.end()));
                FLATPAK_BACKEND_YIELD
                for (auto r : std::as_const(unpooled)) {
                    triage(r, prioritary, rest, filter, false);
                }
                connect(fw, &QFS::finished, stream, [self, futures, stream, _rest = rest, _prioritary = prioritary, filter, fw]() {
                    QVector<StreamResult> rest(_rest), prioritary(_prioritary);
                    for (const auto [source, future] : futures.asKeyValueRange()) {
                        for (const auto &component : future.result()) {
                            triage(self->resourceForComponent(component, source), prioritary, rest, filter, true);
                        }
                    }

                    auto f = [self](auto left, auto right) {
                        return self->flatpakResourceLessThan(left, right);
                    };

                    // Even sorting can not be performed in other thread, and it can take a while
                    std::sort(rest.begin(), rest.end(), f);
                    std::sort(prioritary.begin(), prioritary.end(), f);

                    QList<StreamResult> resources;
                    resources.reserve(prioritary.size() + rest.size());
                    resources.append(std::move(prioritary));
                    resources.append(std::move(rest));

                    if (!resources.isEmpty()) {
                        Q_EMIT stream->resourcesFound(resources);
                    }
                    stream->finish();
                    fw->deleteLater();
                });
            }(this, stream, filter);
        });
    }
}

QCoro::Task<QHash<FlatpakInstallation *, QList<FlatpakInstalledRef *>>> FlatpakBackend::listInstalledRefsForUpdate()
{
    g_autoptr(GCancellable) cancellable = g_object_ref(m_cancellable);

    // Passing installations between threads and using them concurrently most
    // likely isn't safe. But at least let's make sure they are ref'ed.
    const auto installations = m_installations;
    for (const auto &installation : installations) {
        g_object_ref(installation);
    };

    co_return co_await QtConcurrent::run(
        &m_threadPool,
        [](GCancellable *cancellable, QList<FlatpakInstallation *> installations) {
            QHash<FlatpakInstallation *, QVector<FlatpakInstalledRef *>> ret;
            if (g_cancellable_is_cancelled(cancellable)) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Job cancelled";
                return ret;
            }

            for (auto installation : std::as_const(installations)) {
                g_autoptr(GError) localError = nullptr;
                g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs_for_update(installation, cancellable, &localError);
                if (!refs) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get list of installed refs for listing updates:" << localError->message;
                    continue;
                }
                if (g_cancellable_is_cancelled(cancellable)) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Job cancelled";
                    ret.clear();
                    break;
                }

                if (refs->len == 0) {
                    continue;
                }

                auto &current = ret[installation];
                current.reserve(refs->len);
                for (uint i = 0; i < refs->len; i++) {
                    FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));
                    g_object_ref(ref);
                    current.append(ref);
                }
            }
            return ret;
        },
        cancellable,
        installations);
}

bool FlatpakBackend::isTracked(FlatpakResource *resource) const
{
    const auto uid = resource->uniqueId();
    return std::any_of(m_flatpakSources.constBegin(), m_flatpakSources.constEnd(), [uid](const auto &source) {
        return source->m_resources.contains(uid);
    });
}

ResultsStream *FlatpakBackend::findResourceByPackageName(const QUrl &url)
{
    if (url.scheme() == QLatin1String("appstream")) {
        const auto appstreamIds = AppStreamUtils::appstreamIds(url);
        if (appstreamIds.isEmpty()) {
            Q_EMIT passiveMessage(i18n("Malformed appstream url '%1'", url.toDisplayString()));
        } else {
            auto stream = new ResultsStream(QStringLiteral("FlatpakStream-AppStreamUrl"));
            auto f = [this, stream, appstreamIds] {
                const auto pools = kTransform<QList<AppStream::ConcurrentPool *>>(m_flatpakSources, [](const auto &x) {
                    return x->m_pool.get();
                });
                AppStream::ConcurrentPool::componentsByNames(&m_threadPool, pools, appstreamIds)
                    .then(this, [this, stream](const QMap<AppStream::ConcurrentPool *, QList<AppStream::Component>> &componentsList) {
                        QList<StreamResult> resourcesFound;
                        std::set<AbstractResource *> resources;
                        QVector<StreamResult> resourcesVector;
                        auto sorter = [this](auto left, auto right) {
                            return flatpakResourceLessThan(left, right);
                        };

                        for (const auto &[pool, components] : componentsList.asKeyValueRange()) {
                            if (!pool) {
                                continue;
                            }
                            const auto sourceIt = std::ranges::find_if(m_flatpakSources, [pool](const QSharedPointer<FlatpakSource> &s) -> bool {
                                return s->m_pool.get() == pool;
                            });
                            Q_ASSERT(sourceIt != m_flatpakSources.end());
                            const QSharedPointer<FlatpakSource> source = *sourceIt;
                            resourcesFound << kTransform<QVector<StreamResult>>(components, [this, source](const AppStream::Component &comp) -> StreamResult {
                                return {resourceForComponent(comp, source), comp.sortScore()};
                            });
                        }
                        std::sort(resourcesFound.begin(), resourcesFound.end(), sorter);
                        for (auto result : resourcesFound) {
                            auto [x, inserted] = resources.insert(result.resource);
                            if (inserted) {
                                resourcesVector.append(result);
                            }
                        }

                        if (!resourcesVector.isEmpty()) {
                            Q_EMIT stream->resourcesFound(resourcesVector);
                        }
                        stream->finish();
                    });
            };

            if (m_isFetching > 0) {
                connect(this, &FlatpakBackend::initialized, stream, f);
            } else {
                QTimer::singleShot(0, this, f);
            }
            return stream;
        }
    }
    return new ResultsStream(QStringLiteral("FlatpakStream-packageName-void"), {});
}

FlatpakResource *FlatpakBackend::resourceForComponent(const AppStream::Component &component, const QSharedPointer<FlatpakSource> &source) const
{
    const auto ref = idForComponent(component);

    if (auto resource = source->m_resources.value(ref)) {
        return resource;
    }

    auto resource = new FlatpakResource(component, source->installation(), const_cast<FlatpakBackend *>(this));
    resource->setOrigin(source->name());
    resource->setDisplayOrigin(source->title());
    resource->setIconPath(source->appstreamIconsDir());
    resource->updateFromAppStream();
    source->addResource(resource);
    Q_ASSERT(ref == resource->uniqueId());
    return resource;
}

AbstractBackendUpdater *FlatpakBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *FlatpakBackend::reviewsBackend() const
{
    return m_reviews.data();
}

void FlatpakBackend::checkRepositories(const FlatpakJobTransaction::Repositories &repositories)
{
    auto flatpakInstallationByPath = [this](const QString &installationPath) -> FlatpakInstallation * {
        for (auto installation : std::as_const(m_installations)) {
            if (FlatpakResource::installationPath(installation) == installationPath) {
                return installation;
            }
        }
        return nullptr;
    };

    g_autoptr(GError) localError = nullptr;
    for (const auto &[installationPath, names] : repositories.asKeyValueRange()) {
        auto installation = flatpakInstallationByPath(installationPath);
        for (const auto &name : names) {
            if (g_autoptr(FlatpakRemote) remote =
                    flatpak_installation_get_remote_by_name(installation, name.toUtf8().constData(), m_cancellable, &localError)) {
                loadRemote(installation, remote);
            } else {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Could not find remote" << name << "in" << installationPath;
            }
        }
    }
}

FlatpakRemote *FlatpakBackend::installSource(FlatpakResource *resource)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();

    if (g_autoptr(FlatpakRemote) remote =
            flatpak_installation_get_remote_by_name(preferredInstallation(), resource->flatpakName().toUtf8().constData(), cancellable, nullptr)) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Source" << resource->flatpakName() << "already exists in"
                                                   << flatpak_installation_get_path(preferredInstallation());
        return nullptr;
    }

    auto remote = flatpak_remote_new(resource->flatpakName().toUtf8().constData());
    populateRemote(remote,
                   resource->comment(),
                   resource->getMetadata(QLatin1StringView("repo-url")).toString(),
                   resource->getMetadata(QLatin1StringView("gpg-key")).toString());
    if (!resource->branch().isEmpty()) {
        flatpak_remote_set_default_branch(remote, resource->branch().toUtf8().constData());
    }

    g_autoptr(GError) error = nullptr;
    if (!flatpak_installation_add_remote(preferredInstallation(), remote, false, cancellable, &error)) {
        Q_EMIT passiveMessage(i18n("Failed to add source '%1': %2", resource->flatpakName(), QString::fromUtf8(error->message)));
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to add source" << resource->flatpakName() << error->message;
        return nullptr;
    }
    return remote;
}

Transaction *FlatpakBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);

    auto resource = qobject_cast<FlatpakResource *>(app);

    if (resource->resourceType() == FlatpakResource::Source) {
        // Let source backend handle this
        g_autoptr(FlatpakRemote) remote = installSource(resource);
        if (remote) {
            resource->setState(AbstractResource::Installed);
            auto name = flatpak_remote_get_name(remote);
            g_autoptr(FlatpakRemote) remote = flatpak_installation_get_remote_by_name(resource->installation(), name, m_cancellable, nullptr);
            loadRemote(resource->installation(), remote);
        }
        return nullptr;
    }

    auto transaction = new FlatpakJobTransaction(resource, Transaction::InstallRole);
    connect(transaction, &FlatpakJobTransaction::repositoriesAdded, this, &FlatpakBackend::checkRepositories);
    connect(transaction, &FlatpakJobTransaction::statusChanged, this, [this, resource](Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            if (auto tempSource = resource->temporarySource()) {
                auto source = findSource(resource->installation(), resource->origin());
                if (!source) {
                    // It could mean that it's still integrating after checkRepositories It should update itself
                    loadAppsFromAppstreamData(resource->installation());
                } else {
                    resource->setTemporarySource({});
                    const auto id = resource->uniqueId();
                    source->m_resources.insert(id, resource);

                    tempSource->m_resources.remove(id);
                    if (tempSource->m_resources.isEmpty()) {
                        const bool removed = m_flatpakSources.removeAll(tempSource) || m_flatpakLoadingSources.removeAll(tempSource);
                        Q_ASSERT(removed);
                    }
                }
            }
            updateAppState(resource);
        }
    });
    return transaction;
}

Transaction *FlatpakBackend::installApplication(AbstractResource *app)
{
    return installApplication(app, {});
}

Transaction *FlatpakBackend::removeApplication(AbstractResource *app)
{
    auto resource = qobject_cast<FlatpakResource *>(app);

    if (resource->resourceType() == FlatpakResource::Source) {
        // Let source backend handle this
        if (m_sources->removeSource(resource->flatpakName())) {
            resource->setState(AbstractResource::None);
        }
        return nullptr;
    }

    auto transaction = new FlatpakJobTransaction(resource, Transaction::RemoveRole);
    connect(transaction, &FlatpakJobTransaction::repositoriesAdded, this, &FlatpakBackend::checkRepositories);
    connect(transaction, &FlatpakJobTransaction::statusChanged, this, [this, resource](Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppState(resource);
            updateAppSize(resource);
        }
    });
    return transaction;
}

void FlatpakBackend::checkForUpdates()
{
    disconnect(this, &FlatpakBackend::initialized, m_checkForUpdatesTimer, qOverload<>(&QTimer::start));
    for (const auto &source : std::as_const(m_flatpakSources)) {
        if (source->remote()) {
            if (m_refreshAppstreamMetadataJobs.contains(source->remote())) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Already checking for updates" << source->name();
                continue;
            }
            m_refreshAppstreamMetadataJobs.insert(source->remote());
            checkForRemoteUpdates(source->installation(), source->remote());
        }
    }
}

void FlatpakBackend::checkForRemoteUpdates(FlatpakInstallation *installation, FlatpakRemote *remote)
{
    Q_ASSERT(remote);
    const bool needsIntegration = m_refreshAppstreamMetadataJobs.contains(remote);
    if (flatpak_remote_get_disabled(remote) || flatpak_remote_get_noenumerate(remote)) {
        if (needsIntegration) {
            integrateRemote(GLibHolder<FlatpakInstallation>(installation), GLibHolder<FlatpakRemote>(remote));
        }
        return;
    }

    auto job = new FlatpakRefreshAppstreamMetadataJob(installation, remote);
    if (needsIntegration) {
        connect(job, &FlatpakRefreshAppstreamMetadataJob::jobRefreshAppstreamMetadataFinished, this, &FlatpakBackend::integrateRemote);
    }
    connect(job, &FlatpakRefreshAppstreamMetadataJob::finished, this, [this] {
        acquireFetching(false);
    });

    m_collector->add(job);
    acquireFetching(true);
    job->start();
}

QString FlatpakBackend::displayName() const
{
    return QStringLiteral("Flatpak");
}

InlineMessage *FlatpakBackend::explainDysfunction() const
{
    if (m_flatpakSources.isEmpty()) {
        return new InlineMessage(InlineMessage::Error, QStringLiteral("emblem-error"), i18n("There are no Flatpak sources."), m_sources->actions());
    }
    for (const auto &source : m_flatpakSources) {
        if (source->m_pool && !source->m_pool->lastError().isEmpty()) {
            return new InlineMessage(InlineMessage::Error, QStringLiteral("emblem-error"), i18n("Failed to load \"%1\" source", source->name()));
        }
    }
    return AbstractResourcesBackend::explainDysfunction();
}

bool FlatpakBackend::extends(const QString &extends) const
{
    return std::any_of(m_flatpakSources.constBegin(), m_flatpakSources.constEnd(), [extends](const auto &source) {
        return source->m_pool && source->m_pool->lastError().isEmpty() && !source->m_pool->componentsByExtends(extends).result().isEmpty();
    });
}

#include "FlatpakBackend.moc"
#include "moc_FlatpakBackend.cpp"
