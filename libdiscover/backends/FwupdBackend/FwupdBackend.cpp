/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FwupdBackend.h"
#include "../DiscoverVersion.h"
#include "FwupdResource.h"
#include "FwupdSourcesBackend.h"
#include "FwupdTransaction.h"
#include <QtConcurrent/QtConcurrentRun>
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <QCoreApplication>

DISCOVER_BACKEND_PLUGIN(FwupdBackend)

FwupdBackend::FwupdBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , client(fwupd_client_new())
    , m_updater(new StandardBackendUpdater(this))
    , m_cancellable(g_cancellable_new())
{
    auto init = [this] {
        g_autoptr(GError) error = nullptr;
        if (!fwupd_client_connect(client, m_cancellable, &error)) {
            handleError(error);
            return false;
        }
        return true;
    };
    QtConcurrent::run(init).then(this, [this](bool correct) {
        if (!correct) {
            m_isValid = false;
            Q_EMIT invalidated();
            return;
        }
        fwupd_client_set_user_agent_for_package(client, "plasma-discover", version.data());
        connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FwupdBackend::updatesCountChanged);
        connect(TransactionModel::global(), &TransactionModel::lastTransactionFinished, this, &FwupdBackend::checkForUpdates);

        SourcesModel::global()->addSourcesBackend(new FwupdSourcesBackend(this));
        QTimer::singleShot(0, this, &FwupdBackend::checkForUpdates);
        Q_EMIT contentsChanged();
    });
}

QMap<GChecksumType, QCryptographicHash::Algorithm> FwupdBackend::gchecksumToQChryptographicHash()
{
    static QMap<GChecksumType, QCryptographicHash::Algorithm> map;
    if (map.isEmpty()) {
        map.insert(G_CHECKSUM_SHA1, QCryptographicHash::Sha1);
        map.insert(G_CHECKSUM_SHA256, QCryptographicHash::Sha256);
        map.insert(G_CHECKSUM_SHA512, QCryptographicHash::Sha512);
        map.insert(G_CHECKSUM_MD5, QCryptographicHash::Md5);
    }
    return map;
}

FwupdBackend::~FwupdBackend()
{
    g_cancellable_cancel(m_cancellable);
    g_object_unref(m_cancellable);

    g_object_unref(client);
}

void FwupdBackend::addResource(FwupdResource *res)
{
    res->setParent(this);
    auto &r = m_resources[res->packageName()];
    if (r) {
        Q_EMIT resourceRemoved(r);
        delete r;
    }
    r = res;
    Q_ASSERT(m_resources.value(res->packageName()) == res);
}

FwupdResource *FwupdBackend::createRelease(FwupdDevice *device)
{
    FwupdRelease *release = fwupd_device_get_release_default(device);
    FwupdResource *res = new FwupdResource(device, QString::fromUtf8(fwupd_release_get_appstream_id(release)), this);
    res->setReleaseDetails(release);

    /* the same as we have already */
    if (qstrcmp(fwupd_device_get_version(device), fwupd_release_get_version(release)) == 0) {
        qWarning() << "Fwupd Error: same firmware version as installed";
    }

    return res;
}

void FwupdBackend::addUpdates()
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) devices = fwupd_client_get_devices(client, m_cancellable, &error);

    if (!devices) {
        if (g_error_matches(error, FWUPD_ERROR, FWUPD_ERROR_NOTHING_TO_DO))
            qDebug() << "Fwupd Info: No Devices Found";
        else
            handleError(error);
        return;
    }

    for (uint i = 0; i < devices->len && !g_cancellable_is_cancelled(m_cancellable); i++) {
        FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, i);

        if (!fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_SUPPORTED))
            continue;

        if (fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_LOCKED))
            continue;

        if (!fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_UPDATABLE))
            continue;

        g_autoptr(GError) error2 = nullptr;
        g_autoptr(GPtrArray) rels = fwupd_client_get_upgrades(client, fwupd_device_get_id(device), m_cancellable, &error2);
        if (rels) {
            if ((fwupd_device_get_flags(device) & FWUPD_DEVICE_FLAG_NEEDS_REBOOT) && fwupd_device_get_update_state(device) == FWUPD_UPDATE_STATE_SUCCESS) {
                m_updater->setNeedsReboot(true);
                continue;
            }

            fwupd_device_add_release(device, (FwupdRelease *)g_ptr_array_index(rels, 0));
            auto res = createApp(device);
            if (!res) {
                qWarning() << "Fwupd Error: Cannot Create App From Device" << fwupd_device_get_name(device);
            } else {
                QString longdescription;
                for (uint j = 0; j < rels->len; j++) {
                    FwupdRelease *release = (FwupdRelease *)g_ptr_array_index(rels, j);
                    if (!fwupd_release_get_description(release))
                        continue;
                    if (rels->len > 1) {
                        longdescription += QStringLiteral("Version %1\n").arg(QString::fromUtf8(fwupd_release_get_version(release)));
                    }
                    longdescription += QString::fromUtf8(fwupd_release_get_description(release));
                    if (rels->len > 1) {
                        longdescription += QLatin1Char('\n');
                    }
                }
                res->setDescription(longdescription);

                // Make sure to set the installed version of the current thing so
                // they can both be shown in the update page UI
                auto installedResource = m_resources[res->packageName()];
                if (installedResource) {
                    res->setInstalledVersion(installedResource->availableVersion());
                }
                addResource(res);
            }
        } else {
            if (g_error_matches(error2, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED)) {
                qWarning() << "fwupd: Device not supported:" << fwupd_device_get_name(device);
            } else if (!g_error_matches(error2, FWUPD_ERROR, FWUPD_ERROR_NOTHING_TO_DO)) {
                handleError(error2);
            }
        }
    }
}

QByteArray FwupdBackend::getChecksum(const QString &filename, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(filename);
    if (!f.open(QFile::ReadOnly)) {
        qWarning() << "could not open to check" << filename;
        return {};
    }

    QCryptographicHash hash(hashAlgorithm);
    if (!hash.addData(&f)) {
        qWarning() << "could not read to check" << filename;
        return {};
    }

    return hash.result().toHex();
}

FwupdResource *FwupdBackend::createApp(FwupdDevice *device)
{
    FwupdRelease *release = fwupd_device_get_release_default(device);
    std::unique_ptr<FwupdResource> app(createRelease(device));

    if (!app->isLiveUpdatable()) {
        qWarning() << "Fwupd Error: " << app->name() << "[" << app->id() << "]"
                   << "cannot be updated";
        return nullptr;
    }

    if (app->id().isNull()) {
        qWarning() << "Fwupd Error: No id for firmware";
        return nullptr;
    }

    if (app->availableVersion().isNull()) {
        qWarning() << "Fwupd Error: No version! for " << app->id();
        return nullptr;
    }

    GPtrArray *checksums = fwupd_release_get_checksums(release);
    if (checksums->len == 0) {
        qWarning() << "Fwupd Error: " << app->name() << "[" << app->id() << "] has no checksums, ignoring as unsafe";
        return nullptr;
    }

#if FWUPD_CHECK_VERSION(1, 5, 6)
    GPtrArray *locations = fwupd_release_get_locations(release);
    const QUrl update_uri(locations->len == 0 ? QString::fromUtf8("") : QString::fromUtf8((const gchar *)g_ptr_array_index(locations, 0)));
#else
    const QUrl update_uri(QString::fromUtf8(fwupd_release_get_uri(release)));
#endif
    if (!update_uri.isValid()) {
        qWarning() << "Fwupd Error: No Update URI available for" << app->name() << "[" << app->id() << "]";
        return nullptr;
    }

    /* Checking for firmware in the cache? */
    const QString filename_cache = app->cacheFile();
    if (QFile::exists(filename_cache)) {
        /* Currently LVFS supports SHA1 only*/
        const QByteArray checksum_tmp(fwupd_checksum_get_by_kind(checksums, G_CHECKSUM_SHA1));
        const QByteArray checksum = getChecksum(filename_cache, QCryptographicHash::Sha1);
        if (checksum_tmp != checksum) {
            QFile::remove(filename_cache);
        }
    }

    app->setState(AbstractResource::Upgradeable);
    return app.release();
}

void FwupdBackend::handleError(GError *perror)
{
    // TODO: localise the error message
    if (perror && !g_error_matches(perror, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE)
        && !g_error_matches(perror, FWUPD_ERROR, FWUPD_ERROR_NOTHING_TO_DO)
        // "Systemd service is masked" error, which is not really an error, but has no clean GError string,
        // so we have to compare codes and strings. Thankfully it's unlocalized!
        && !(perror->code == 3 && QString::fromUtf8(perror->message).contains(QStringLiteral("unit is masked")))) {
        const QString msg = QString::fromUtf8(perror->message);
        QTimer::singleShot(0, this, [this, msg]() {
            Q_EMIT passiveMessage(msg);
        });
        qWarning() << "Fwupd Error" << perror->code << perror->message;
    }
    // else
    //     qDebug() << "Fwupd skipped" << perror->code << perror->message;
}

QString FwupdBackend::cacheFile(const QString &kind, const QString &basename)
{
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
    const QString cacheDirFile = cacheDir.filePath(kind);

    if (!QFileInfo::exists(cacheDirFile) && !cacheDir.mkpath(kind)) {
        qWarning() << "Fwupd Error: cannot make  cache directory!";
        return {};
    }

    return cacheDir.filePath(kind + QLatin1Char('/') + basename);
}

static void fwupd_client_get_devices_cb(GObject * /*source*/, GAsyncResult *res, gpointer user_data)
{
    FwupdBackend *helper = (FwupdBackend *)user_data;
    g_autoptr(GError) error = nullptr;
    auto array = fwupd_client_get_devices_finish(helper->client, res, &error);
    if (!error)
        helper->setDevices(array);
    else
        helper->handleError(error);
}

void FwupdBackend::setDevices(GPtrArray *devices)
{
    for (uint i = 0; devices && i < devices->len; i++) {
        FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, i);

        if (!fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_SUPPORTED))
            continue;

        g_autoptr(GError) error = nullptr;
        g_autoptr(GPtrArray) releases = fwupd_client_get_releases(client, fwupd_device_get_id(device), m_cancellable, &error);

        if (error) {
            if (g_error_matches(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED)) {
                qWarning() << "fwupd: Device not supported:" << fwupd_device_get_name(device) << error->message;
                continue;
            }
            if (g_error_matches(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE)) {
                continue;
            }

            handleError(error);
        }

        auto res = new FwupdResource(device, this);
        for (uint i = 0; releases && i < releases->len; ++i) {
            FwupdRelease *release = (FwupdRelease *)g_ptr_array_index(releases, i);
            if (res->installedVersion().toUtf8() == fwupd_release_get_version(release)) {
                res->setReleaseDetails(release);
                break;
            }
        }
        addResource(res);
    }
    g_ptr_array_unref(devices);

    addUpdates();

    m_fetching = false;
    Q_EMIT contentsChanged();
    Q_EMIT initialized();
}

static void fwupd_client_get_remotes_cb(GObject * /*source*/, GAsyncResult *res, gpointer user_data)
{
    FwupdBackend *helper = (FwupdBackend *)user_data;
    g_autoptr(GError) error = nullptr;
    auto array = fwupd_client_get_remotes_finish(helper->client, res, &error);
    if (!error)
        helper->setRemotes(array);
    else
        helper->handleError(error);
}

static void fwupd_client_refresh_remote_cb(GObject * /*source*/, GAsyncResult *res, gpointer user_data)
{
    FwupdBackend *helper = (FwupdBackend *)user_data;
    g_autoptr(GError) error = nullptr;
    const bool successful = fwupd_client_refresh_remote_finish(helper->client, res, &error);
    if (!successful)
        helper->handleError(error);
}

void FwupdBackend::setRemotes(GPtrArray *remotes)
{
    for (uint i = 0; remotes && i < remotes->len; i++) {
        FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index(remotes, i);
        if (!fwupd_remote_has_flag(remote, FWUPD_REMOTE_FLAG_ENABLED))
            continue;

        if (fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_LOCAL || fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_DIRECTORY) {
            continue;
        }

#if FWUPD_CHECK_VERSION(2, 0, 0)
        fwupd_client_refresh_remote_async(client, remote, FWUPD_CLIENT_DOWNLOAD_FLAG_NONE, m_cancellable, fwupd_client_refresh_remote_cb, this);
#else
        fwupd_client_refresh_remote2_async(client, remote, FWUPD_CLIENT_DOWNLOAD_FLAG_NONE, m_cancellable, fwupd_client_refresh_remote_cb, this);
#endif
    }
}

void FwupdBackend::checkForUpdates()
{
    if (m_fetching)
        return;

    m_fetching = true;
    Q_EMIT fetchingUpdatesProgressChanged();

    fwupd_client_get_devices_async(client, m_cancellable, fwupd_client_get_devices_cb, this);
    fwupd_client_get_remotes_async(client, m_cancellable, fwupd_client_get_remotes_cb, this);
}

int FwupdBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream *FwupdBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (!filter.resourceUrl.isEmpty()) {
        if (filter.resourceUrl.scheme() == QLatin1String("fwupd")) {
            return findResourceByPackageName(filter.resourceUrl);
        } else if (filter.resourceUrl.isLocalFile()) {
            return resourceForFile(filter.resourceUrl);
        }
        return new ResultsStream(QStringLiteral("FwupdStream-empty"), {});
    }

    auto stream = new ResultsStream(QStringLiteral("FwupdStream"));
    auto f = [this, stream, filter]() {
        QVector<StreamResult> ret;
        for (AbstractResource *r : std::as_const(m_resources)) {
            if (r->state() < filter.state)
                continue;

            if (filter.search.isEmpty() || r->name().contains(filter.search, Qt::CaseInsensitive)
                || r->comment().contains(filter.search, Qt::CaseInsensitive)) {
                ret += r;
            }
        }
        if (!ret.isEmpty())
            Q_EMIT stream->resourcesFound(ret);
        stream->finish();
    };
    if (m_fetching) {
        connect(this, &FwupdBackend::initialized, stream, f);
    } else {
        QTimer::singleShot(0, this, f);
    }
    return stream;
}

ResultsStream *FwupdBackend::findResourceByPackageName(const QUrl &search)
{
    auto res = search.scheme() == QLatin1String("fwupd") ? m_resources.value(search.host().replace(QLatin1Char('.'), QLatin1Char(' '))) : nullptr;
    if (!res) {
        return new ResultsStream(QStringLiteral("FwupdStream"), {});
    } else
        return new ResultsStream(QStringLiteral("FwupdStream"), {res});
}

AbstractBackendUpdater *FwupdBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *FwupdBackend::reviewsBackend() const
{
    return nullptr;
}

Transaction *FwupdBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_ASSERT(addons.isEmpty());
    return installApplication(app);
}

Transaction *FwupdBackend::installApplication(AbstractResource *app)
{
    return new FwupdTransaction(qobject_cast<FwupdResource *>(app), this);
}

Transaction *FwupdBackend::removeApplication(AbstractResource * /*app*/)
{
    qWarning() << "should not have reached here, it's not possible to uninstall a firmware";
    return nullptr;
}

ResultsStream *FwupdBackend::resourceForFile(const QUrl &path)
{
    if (!path.isLocalFile())
        return new ResultsStream(QStringLiteral("FwupdStream-void"), {});

    g_autoptr(GError) error = nullptr;

    const QString fileName = path.fileName();
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(fileName);
    FwupdResource *app = nullptr;

    if (type.isValid() && type.inherits(QStringLiteral("application/vnd.ms-cab-compressed"))) {
        g_autofree gchar *filename = fileName.toUtf8().data();
        g_autoptr(GPtrArray) devices = fwupd_client_get_details(client, filename, nullptr, &error);

        if (devices) {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, 0);
            app = createRelease(device);
            app->setState(AbstractResource::None);
            for (uint i = 1; i < devices->len; i++) {
                FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, i);
                FwupdResource *app_ = createRelease(device);
                app_->setState(AbstractResource::None);
            }
            addResource(app);
            connect(app, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
            return new ResultsStream(QStringLiteral("FwupdStream-file"), {app});
        } else {
            handleError(error);
        }
    }
    return new ResultsStream(QStringLiteral("FwupdStream-void"), {});
}

QString FwupdBackend::displayName() const
{
    return i18n("Firmware Updates");
}

bool FwupdBackend::hasApplications() const
{
    return false;
}

#include "FwupdBackend.moc"
#include "moc_FwupdBackend.cpp"
