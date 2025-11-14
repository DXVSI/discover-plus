/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitBackend.h"
#include "AppPackageKitResource.h"
#include "LocalFilePKResource.h"
#include "PKResolveTransaction.h"
#include "PKTransaction.h"
#include "PackageKitSourcesBackend.h"
#include "PackageKitUpdater.h"
#include <AppStreamQt/release.h>
#include <AppStreamQt/systeminfo.h>
#include <AppStreamQt/utils.h>
#include <AppStreamQt/version.h>
#include <appstream/AppStreamIntegration.h>
#include <appstream/AppStreamUtils.h>
#include <appstream/OdrsReviewsBackend.h>
#include <resources/AbstractResource.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>

#include <QDebug>
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QHash>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QtConcurrentRun>

#include <QCoroCore>

#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <PackageKit/Offline>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KProtocolManager>

#include <optional>

#include "config-paths.h"
#include "libdiscover_backend_packagekit_debug.h"
#include "utils.h"
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <resources/ResourcesModel.h>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

DISCOVER_BACKEND_PLUGIN(PackageKitBackend)

QDebug operator<<(QDebug dbg, const PackageOrAppId &value)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "PackageOrAppId(";
    dbg << "id: " << value.id << ',';
    dbg << "isPkg: " << value.isPackageName;
    dbg << ')';
    return dbg;
}

bool operator==(const PackageOrAppId &a, const PackageOrAppId &b)
{
    return a.isPackageName == b.isPackageName && a.id == b.id;
}

uint qHash(const PackageOrAppId &id, uint seed)
{
    return qHash(id.id, seed) ^ qHash(id.isPackageName, seed);
}

PackageOrAppId makeAppId(const QString &id)
{
    return {id, false};
}

PackageOrAppId makePackageId(const QString &id)
{
    return {id, true};
}

PackageOrAppId makeResourceId(PackageKitResource *resource)
{
    auto appstreamResource = qobject_cast<AppPackageKitResource *>(resource);
    if (appstreamResource) {
        return {appstreamResource->appstreamId(), false};
    }
    return {resource->packageName(), true};
}

template<typename T, typename W>
static void setWhenAvailable(const QDBusPendingReply<T> &pending, W func, QObject *parent)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, parent);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, parent, [func](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<T> reply = *watcher;
        func(reply.value());
    });
}

QString PackageKitBackend::locateService(const QString &filename)
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1StringView("applications/") + filename);
}

Delay::Delay()
{
    m_delay.setSingleShot(true);
    m_delay.setInterval(100);

    connect(&m_delay, &QTimer::timeout, this, [this] {
        Q_EMIT perform(m_pkgids);
        m_pkgids.clear();
    });
}

PackageKitBackend::PackageKitBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_appdata(new AppStream::ConcurrentPool)
    , m_updater(new PackageKitUpdater(this))
    , m_refresher(nullptr)
    , m_isFetching(0)
    , m_reviews(OdrsReviewsBackend::global())
{
    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::checkForUpdates);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();

    connect(&m_details, &Delay::perform, this, &PackageKitBackend::performDetailsFetch);
    connect(&m_updateDetails, &Delay::perform, this, [this](const QSet<QString> &pkgids) {
        PackageKit::Transaction *t = PackageKit::Daemon::getUpdatesDetails(kSetToList(pkgids));
        connect(t,
                &PackageKit::Transaction::updateDetail,
                this,
                [this](const QString &packageID,
                       const QStringList &updates,
                       const QStringList &obsoletes,
                       const QStringList &vendorUrls,
                       const QStringList &bugzillaUrls,
                       const QStringList &cveUrls,
                       PackageKit::Transaction::Restart restart,
                       const QString &updateText,
                       const QString &changelog,
                       PackageKit::Transaction::UpdateState state,
                       const QDateTime &issued,
                       const QDateTime &updated) {
                    const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(packageID));
                    for (auto r : resources) {
                        PackageKitResource *resource = qobject_cast<PackageKitResource *>(r);
                        if (resource->containsPackageId(packageID)) {
                            resource->updateDetail(packageID,
                                                   updates,
                                                   obsoletes,
                                                   vendorUrls,
                                                   bugzillaUrls,
                                                   cveUrls,
                                                   restart,
                                                   updateText,
                                                   changelog,
                                                   state,
                                                   issued,
                                                   updated);
                        }
                    }
                });
        connect(t, &PackageKit::Transaction::errorCode, this, [this, pkgids](PackageKit::Transaction::Error err, const QString &error) {
            qWarning() << "PackageKitBackend: Error fetching updates:" << err << error;
            for (const QString &pkgid : pkgids) {
                const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(pkgid));
                for (auto r : resources) {
                    PackageKitResource *resource = qobject_cast<PackageKitResource *>(r);
                    if (resource->containsPackageId(pkgid)) {
                        Q_EMIT resource->changelogFetched(QString());
                    }
                }
            }
        });
    });

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::restartScheduled, this, [this] {
        m_updater->setNeedsReboot(true);
    });
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitBackend::checkDaemonRunning);
    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, [this] {
        const auto resources = m_packages.packages.values();
        m_reviews->emitRatingFetched(this, resources);
    });

    auto proxyWatch = new QFileSystemWatcher(this);
    proxyWatch->addPath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kioslaverc"));
    connect(proxyWatch, &QFileSystemWatcher::fileChanged, this, [this]() {
        KProtocolManager::reparseConfiguration();
        updateProxy();
    });

    SourcesModel::global()->addSourcesBackend(new PackageKitSourcesBackend(this));

    reloadPackageList();

    acquireFetching(true);
    setWhenAvailable(
        PackageKit::Daemon::getTimeSinceAction(PackageKit::Transaction::RoleRefreshCache),
        [this](uint timeSince) {
            if (timeSince > 3600) {
                checkForUpdates();
            } else if (!PackageKit::Daemon::global()->offline()->upgradeTriggered()) {
                fetchUpdates();
            }
            acquireFetching(false);
        },
        this);

    m_globalHints = QStringList() << QStringLiteral("interactive=true") << QStringLiteral("locale=%1").arg(qEnvironmentVariable("LANG"));
    PackageKit::Daemon::global()->setHints(m_globalHints);

    // Hide the drivers category if there's no drivers
    connect(CategoryModel::global(), &CategoryModel::rootCategoriesChanged, this, [this] {
        auto driversCategory = CategoryModel::global()->findCategoryByName(QStringLiteral("Hardware Drivers"));

        // Will be null on subsequent runs
        if (!driversCategory) {
            return;
        }

        AbstractResourcesBackend::Filters filter;
        filter.category = driversCategory;
        auto stream = search(filter);
        connect(stream, &ResultsStream::resourcesFound, stream, [stream] {
            stream->setProperty("foundDrivers", true);
        });
        connect(stream, &ResultsStream::destroyed, stream, [stream, driversCategory] {
            if (!stream->property("foundDrivers").toBool()) {
                driversCategory->hide();
            }
        });
    });
}

PackageKitBackend::~PackageKitBackend()
{
    m_threadPool.waitForDone(200);
    m_threadPool.clear();
}

QString proxyFor(KConfigGroup *config, const QString &protocol)
{
    const QString key = protocol + QLatin1String("Proxy");
    QString proxyStr(config->readEntry(key));
    const int index = proxyStr.lastIndexOf(QLatin1Char(' '));

    if (index > -1) {
        const QStringView portStr = QStringView(proxyStr).right(proxyStr.length() - index - 1);
        const bool isDigits = std::all_of(portStr.cbegin(), portStr.cend(), [](const QChar c) {
            return c.isDigit();
        });

        if (isDigits) {
            proxyStr = QStringView(proxyStr).left(index) + QLatin1Char(':') + portStr;
        } else {
            proxyStr.clear();
        }
    }

    return proxyStr;
}

void PackageKitBackend::updateProxy()
{
    if (PackageKit::Daemon::isRunning()) {
        KConfig kioSettings(QStringLiteral("kioslaverc"));
        KConfigGroup proxyConfig = kioSettings.group(u"Proxy Settings"_s);

        bool useProxy = proxyConfig.readEntry<int>("ProxyType", 0) != 0;

        static bool everHad = useProxy;
        if (!everHad && !useProxy) {
            return;
        }

        everHad = useProxy;

        PackageKit::Daemon::global()->setProxy(proxyFor(&proxyConfig, QStringLiteral("http")),
                                               proxyFor(&proxyConfig, QStringLiteral("https")),
                                               proxyFor(&proxyConfig, QStringLiteral("ftp")),
                                               proxyFor(&proxyConfig, QStringLiteral("socks")),
                                               {},
                                               {});
    }
}

void PackageKitBackend::acquireFetching(bool f)
{
    if (f) {
        m_isFetching++;
    } else {
        m_isFetching--;
    }

    if (!f && m_isFetching == 0) {
        Q_EMIT contentsChanged();
        Q_EMIT available();
        Q_EMIT updatesCountChanged();
    }
    Q_ASSERT(m_isFetching >= 0);
}

void PackageKitBackend::reloadPackageList()
{
    acquireFetching(true);

    auto *pool = new AppStream::Pool;
    pool->setFlags(AppStream::Pool::Flags(AppStream::Pool::Flag::FlagLoadOsCatalog
                                          | AppStream::Pool::Flag::FlagLoadOsDesktopFiles
                                          | AppStream::Pool::Flag::FlagLoadOsMetainfo));
    m_appdata->reset(pool, &m_threadPool);

    const auto loadDone = [this](bool correct) {
        if (!correct && m_packages.packages.isEmpty()) {
            QTimer::singleShot(0, this, [this]() {
                Q_EMIT passiveMessage(i18n("Please make sure that Appstream is properly set up on your system"));
            });
        }

        // TODO
        // if (data.components.isEmpty()) {
        //     qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "empty appstream db";
        //     if (PackageKit::Daemon::backendName() == QLatin1String("aptcc") || PackageKit::Daemon::backendName().isEmpty()) {
        //         checkForUpdates();
        //     }
        // }
        if (!m_appstreamInitialized) {
            m_appstreamInitialized = true;
            Q_EMIT loadedAppStream();
        }
        acquireFetching(false);

        const auto distroComponents = m_appdata->componentsById(AppStream::SystemInfo::currentDistroComponentId());
        if (distroComponents.result().isEmpty()) {
            qWarning() << "PackageKitBackend: No distro component found for" << AppStream::SystemInfo::currentDistroComponentId();
        }
        for (const AppStream::Component &dc : distroComponents.result()) {
            const auto releases = dc.releasesPlain().entries();
            for (const auto &r : releases) {
                int cmp = AppStream::Utils::vercmpSimple(r.version(), AppStreamIntegration::global()->osRelease()->versionId());
                if (cmp == 0) {
                    // Ignore (likely) empty date_eol entries that are parsed as the UNIX Epoch
                    if (r.timestampEol().isNull() || r.timestampEol().toSecsSinceEpoch() == 0) {
                        continue;
                    }
                    if (r.timestampEol() < QDateTime::currentDateTime()) {
                        const QString releaseDate = QLocale().toString(r.timestampEol());
                        Q_EMIT inlineMessageChanged(
                            QSharedPointer<InlineMessage>::create(InlineMessage::Warning,
                                                                  QStringLiteral("dialog-warning"),
                                                                  i18nc("%1 is the date as formatted by the locale",
                                                                        "Your operating system ended support on %1. Consider upgrading to a supported version.",
                                                                        releaseDate)));
                    }
                }
            }
        }
    };

    connect(m_appdata.get(), &AppStream::ConcurrentPool::loadFinished, this, [this, loadDone](bool success) {
        m_appdataLoaded = true;
        if (!success) {
            qWarning() << "PackageKitBackend: Could not open the AppStream metadata pool" << m_appdata->lastError();
        }
        // We do not want to block the GUI for very long, so we load appstream pools via queued
        // invocation for the post-load state changes.
        QMetaObject::invokeMethod(
            this,
            [loadDone, success] {
                loadDone(success);
            },
            Qt::QueuedConnection);
    });
    m_appdata->loadAsync();
}

AppPackageKitResource *PackageKitBackend::addComponent(const AppStream::Component &component) const
{
    const QStringList pkgNames = component.packageNames();
    Q_ASSERT(!pkgNames.isEmpty());
    auto appId = makeAppId(component.id());
    auto resource = qobject_cast<AppPackageKitResource *>(m_packages.packages.value(appId));
    if (!resource) {
        resource = qobject_cast<AppPackageKitResource *>(m_packagesToAdd.value(appId));
    }
    if (!resource) {
        resource = new AppPackageKitResource(component, pkgNames.at(0), const_cast<PackageKitBackend *>(this));
        m_packagesToAdd.insert(appId, resource);
    }

    for (const auto &pkg : pkgNames) {
        m_packages.packageToApp[pkg] += component.id();
    }
    return resource;
}

PKResolveTransaction *PackageKitBackend::resolvePackages(const QStringList &packageNames)
{
    if (packageNames.isEmpty()) {
        return nullptr;
    }

    if (!m_resolveTransaction) {
        m_resolveTransaction = new PKResolveTransaction(this);
        connect(m_resolveTransaction, &PKResolveTransaction::allFinished, this, &PackageKitBackend::getPackagesFinished);
        connect(m_resolveTransaction, &PKResolveTransaction::started, this, [this] {
            m_resolveTransaction = nullptr;
        });
    }

    m_resolveTransaction->addPackageNames(packageNames);
    return m_resolveTransaction;
}

void PackageKitBackend::fetchUpdates()
{
    if (m_updater->isProgressing()) {
        return;
    }

    PackageKit::Transaction *tUpdates = PackageKit::Daemon::getUpdates();
    connect(tUpdates, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesFinished);
    connect(tUpdates, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
    connect(tUpdates, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
    m_updatesPackageId.clear();
    m_hasSecurityUpdates = false;

    m_updater->setProgressing(true);

    Q_EMIT fetchingUpdatesProgressChanged();
}

void PackageKitBackend::addPackageArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    addPackage(info, packageId, summary, true);
}

void PackageKitBackend::addPackageNotArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    addPackage(info, packageId, summary, false);
}

void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary, bool arch)
{
    if (PackageKit::Daemon::packageArch(packageId) == QLatin1String("source")) {
        // We do not add source packages, they make little sense here. If source is needed,
        // we are going to have to consider that in some other way, some other time
        // If we do not ignore them here, e.g. openSuse entirely fails at installing applications
        return;
    }
    const QString packageName = PackageKit::Daemon::packageName(packageId);
    QSet<AbstractResource *> r = resourcesByPackageName(packageName);
    if (r.isEmpty()) {
        auto pk = new PackageKitResource(packageName, summary, this);
        r = {pk};
        m_packagesToAdd.insert(makePackageId(packageName), pk);
    }
    for (auto resource : std::as_const(r)) {
        static_cast<PackageKitResource *>(resource)->addPackageId(info, packageId, arch);
    }
}

void PackageKitBackend::getPackagesFinished()
{
    includePackagesToAdd();
}

void PackageKitBackend::includePackagesToAdd()
{
    if (m_packagesToAdd.isEmpty() && m_packagesToDelete.isEmpty()) {
        return;
    }

    acquireFetching(true);
    for (auto [id, resource] : KeyValueRange(std::as_const(m_packagesToAdd))) {
        m_packages.packages[id] = resource;
    }
    m_packagesToAdd.clear();
    for (auto pkResource : std::as_const(m_packagesToDelete)) {
        const auto pkgs = m_packages.packageToApp.value(pkResource->packageName(), {pkResource->packageName()});
        for (const auto &pkg : pkgs) {
            auto resource = m_packages.packages.take(makePackageId(pkg));
            if (resource) {
                Q_EMIT resourceRemoved(resource);
                resource->deleteLater();
            }
        }
    }
    m_packagesToDelete.clear();
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString &message)
{
    qWarning() << "Transaction error:" << message << sender();
    Q_EMIT passiveMessage(message);
}

void PackageKitBackend::packageDetails(const PackageKit::Details &details)
{
    const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(details.packageId()));
    if (resources.isEmpty()) {
        qWarning() << "PackageKitBackend: Couldn't find package for" << details.packageId();
        return;
    }

    for (auto resource : resources) {
        qobject_cast<PackageKitResource *>(resource)->setDetails(details);
    }
}

QSet<AbstractResource *> PackageKitBackend::resourcesByPackageName(const QString &name) const
{
    return resourcesByPackageNames<QSet<AbstractResource *>>(QStringList{name});
}

template<typename T, typename W>
T PackageKitBackend::resourcesByAppNames(const W &appNames) const
{
    T ret;
    ret.reserve(appNames.size());
    for (const QString &name : appNames) {
        auto resource = m_packages.packages.value(makeAppId(name));
        if (resource) {
            ret += resource;
        }
    }
    return ret;
}

template<typename T, typename W>
T PackageKitBackend::resourcesByPackageNames(const W &pkgnames) const
{
    T ret;
    ret.reserve(pkgnames.size());
    for (const QString &pkg_name : pkgnames) {
        const QStringList app_names = m_packages.packageToApp.value(pkg_name, QStringList());
        if (app_names.isEmpty()) {
            auto pkgId = makePackageId(pkg_name);
            auto resource = qobject_cast<PackageKitResource *>(m_packages.packages.value(pkgId));
            if (!resource) {
                resource = m_packagesToAdd.value(pkgId);
            }
            if (resource) {
                ret += resource;
            }
        } else {
            for (const QString &app_id : app_names) {
                const auto appId = makeAppId(app_id);
                auto resource = m_packages.packages.value(appId);
                if (!resource) {
                    resource = m_packagesToAdd.value(appId);
                }
                if (resource) {
                    ret += resource;
                } else {
                    ret += resourcesByComponents<T>(m_appdata->componentsByBundleId(AppStream::Bundle::KindPackage, pkg_name, false).result());
                }
            }
        }
    }
    return ret;
}

void PackageKitBackend::checkForUpdates()
{
    if (auto offline = PackageKit::Daemon::global()->offline(); offline->updateTriggered() || offline->upgradeTriggered()) {
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Won't be checking for updates again, the system needs a reboot to apply the fetched offline updates.";
        return;
    }

    if (!m_refresher) {
        acquireFetching(true);
        Q_EMIT m_updater->fetchingChanged();
        m_updater->clearDistroUpgrade();
        m_refresher = PackageKit::Daemon::refreshCache(false);
        // Limit the cache-age so that we actually download new caches if necessary
        m_refresher->setHints(globalHints() << QStringLiteral("cache-age=300" /* 5 minutes */));

        connect(m_refresher.data(), &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(m_refresher.data(), &PackageKit::Transaction::percentageChanged, this, &PackageKitBackend::fetchingUpdatesProgressChanged);
        connect(m_refresher.data(), &PackageKit::Transaction::finished, this, [this]() {
            m_refresher = nullptr;
            fetchUpdates();
            acquireFetching(false);
            Q_EMIT m_updater->fetchingChanged();
        });
    } else {
        qWarning() << "PackageKitBackend: Already resetting";
    }

    Q_EMIT fetchingUpdatesProgressChanged();
}

AppStream::ComponentBox PackageKitBackend::componentsById(const QString &id) const
{
    Q_ASSERT(m_appstreamInitialized);
    auto comps = m_appdata->componentsById(id);
    if (comps.result().isEmpty()) {
        comps = m_appdata->componentsByProvided(AppStream::Provided::KindId, id);
    }
    return comps.result();
}

static bool needsResolveFilter(const StreamResult &result)
{
    return result.resource->state() == AbstractResource::Broken;
};

class PKResultsStream : public ResultsStream
{
private:
    PKResultsStream(PackageKitBackend *backend, const QString &name)
        : ResultsStream(name)
        , backend(backend)
    {
        Q_ASSERT(QThread::currentThread() == backend->thread());
    }

    PKResultsStream(PackageKitBackend *backend, const QString &name, const QVector<StreamResult> &resources)
        : ResultsStream(name)
        , backend(backend)
    {
        QTimer::singleShot(0, this, [resources, this]() {
            sendResources(resources);
        });
    }

public:
    template<typename... Args>
    [[nodiscard]] static QPointer<PKResultsStream> create(Args &&...args)
    {
        return new PKResultsStream(std::forward<Args>(args)...);
    }

    void sendResources(const QVector<StreamResult> &resources, bool waitForResolved = false)
    {
        if (resources.isEmpty()) {
            finish();
            return;
        }

        Q_ASSERT(resources.size() == QSet(resources.constBegin(), resources.constEnd()).size());
        Q_ASSERT(QThread::currentThread() == backend->thread());
        const auto toResolve = kFilter<QVector<StreamResult>>(resources, needsResolveFilter);
        if (!toResolve.isEmpty()) {
            auto transaction = backend->resolvePackages(kTransform<QStringList>(toResolve, [](const StreamResult &result) {
                return result.resource->packageName();
            }));
            if (waitForResolved) {
                Q_ASSERT(transaction);
                connect(transaction, &QObject::destroyed, this, [this, resources] {
                    Q_EMIT resourcesFound(resources);
                    finish();
                });
                return;
            }
        }

        Q_EMIT resourcesFound(resources);
        finish();
    }

private:
    PackageKitBackend *const backend;
};

PKResultsStream *PackageKitBackend::deferredResultStream(const QString &streamName, std::function<void(PKResultsStream *)> callback)
{
    // NOTE: stream is a child of `this` so we don't have guard this backend object separately.
    auto stream = PKResultsStream::create(this, streamName);

    // Don't capture variables into a coroutine lambda, pass them in as arguments instead
    // See https://devblogs.microsoft.com/oldnewthing/20211103-00/?p=105870
    [](PackageKitBackend *self, QPointer<PKResultsStream> stream, std::function<void(PKResultsStream *)> callback) -> QCoro::Task<> {
        if (self->m_appstreamInitialized) {
            co_await QCoro::sleepFor(0ms);
        } else {
            co_await qCoro(self, &PackageKitBackend::loadedAppStream);
        }
        if (stream.isNull()) {
            co_return;
        }
        callback(stream);
    }(this, stream, std::move(callback));

    return stream;
}

ResultsStream *PackageKitBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    // In this method we are copying filters by value into capturing lambdas
    // to avoid lifetime issues. Also coroutines should not be capturing, so
    // we use nested lambdas with arguments instead of captures.
    if (!filter.resourceUrl.isEmpty()) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (!filter.extends.isEmpty()) {
        return deferredResultStream(u"PackageKitStream-extends"_s, [this, filter = filter](PKResultsStream *stream) {
            m_appdata->componentsByExtends(filter.extends).then(this, [this, stream, filter](const QFuture<AppStream::ComponentBox> &extendingComponents) {
                auto resources = resultsByComponents(extendingComponents.result());
                stream->sendResources(resources, filter.state != AbstractResource::Broken);
            });
        });
    } else if (filter.state == AbstractResource::Upgradeable) {
        return new ResultsStream(QStringLiteral("PackageKitStream-upgradeable"),
                                 kTransform<QVector<StreamResult>>(upgradeablePackages())); // No need for it to be a PKResultsStream
    } else if (filter.state == AbstractResource::Installed) {
        return deferredResultStream(u"PackageKitStream-installed"_s, [this, filter = filter](PKResultsStream *stream) {
            loadAllPackages();

            const auto toResolve = kFilter<QVector<AbstractResource *>>(m_packages.packages, needsResolveFilter);

            auto installedAndNameFilter = [filter](AbstractResource *resource) {
                return resource->state() >= AbstractResource::Installed && !qobject_cast<PackageKitResource *>(resource)->isCritical()
                    && (resource->name().contains(filter.search, Qt::CaseInsensitive)
                        || resource->packageName().compare(filter.search, Qt::CaseInsensitive) == 0);
            };
            bool furtherSearch = false;
            if (!toResolve.isEmpty()) {
                resolvePackages(kTransform<QStringList>(toResolve, [](AbstractResource *resource) {
                    return resource->packageName();
                }));
                connect(m_resolveTransaction, &PKResolveTransaction::allFinished, this, [stream, toResolve, installedAndNameFilter] {
                    const auto resolved = kFilter<QVector<AbstractResource *>>(toResolve, installedAndNameFilter);
                    if (!resolved.isEmpty()) {
                        Q_EMIT stream->resourcesFound(kTransform<QVector<StreamResult>>(resolved, [](auto resource) {
                            return StreamResult(resource, 0);
                        }));
                    }
                    stream->finish();
                });
                furtherSearch = true;
            }

            const auto resolved = kFilter<QVector<AbstractResource *>>(m_packages.packages, installedAndNameFilter);
            if (!resolved.isEmpty()) {
                QTimer::singleShot(0, stream, [resolved, toResolve, stream]() {
                    if (!resolved.isEmpty()) {
                        Q_EMIT stream->resourcesFound(kTransform<QVector<StreamResult>>(resolved, [](auto resource) {
                            return StreamResult(resource, 0);
                        }));
                    }

                    if (toResolve.isEmpty()) {
                        stream->finish();
                    }
                });
                furtherSearch = true;
            }

            if (!furtherSearch) {
                stream->finish();
            }
        });
    } else if (filter.search.isEmpty() && !filter.category) {
        return deferredResultStream(u"PackageKitStream-all"_s, [this](PKResultsStream *stream) {
            loadAllPackages();

            auto resources = kFilter<QVector<AbstractResource *>>(m_packages.packages, [](AbstractResource *resource) {
                auto pkResource = qobject_cast<PackageKitResource *>(resource);
                // Neither PackageKitResource or its subclass AppPackageKitResource can have type == ApplicationSupport
                return resource->type() != AbstractResource::System && pkResource && !pkResource->isCritical() && !pkResource->extendsItself();
            });
            stream->sendResources(kTransform<QVector<StreamResult>>(resources, [](auto resource) {
                return StreamResult(resource, 0);
            }));
        });
    } else {
        return deferredResultStream(u"PackageKitStream-search"_s, [this, filter = filter](PKResultsStream *stream) {
            qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "PackageKitBackend search called with query:" << filter.search;
            auto loadComponents = [](const auto &filter, const auto &appdata) -> QFuture<AppStream::ComponentBox> {
                QFuture<AppStream::ComponentBox> components;
                if (!filter.search.isEmpty()) {
                    components = appdata->search(filter.search);
                } else if (filter.category) {
                    components = AppStreamUtils::componentsByCategoriesTask(appdata.get(), filter.category, AppStream::Bundle::KindUnknown);
                } else {
                    components = appdata->components();
                }
                return components;
            };

            auto watcher = new QFutureWatcher<AppStream::ComponentBox>();
            auto futureComponents = loadComponents(filter, m_appdata);
            watcher->setFuture(futureComponents);
            connect(watcher, &QFutureWatcher<AppStream::ComponentBox>::finished, watcher, &QObject::deleteLater);
            connect(watcher, &QFutureWatcher<AppStream::ComponentBox>::finished, this, [this, stream, filter, futureComponents]() {
                QSet<QString> ids;
                AppStream::ComponentBox components = futureComponents.result();
                kFilterInPlace<AppStream::ComponentBox>(components, [&ids](const AppStream::Component &component) {
                    if (ids.contains(component.id())) {
                        return false;
                    }
                    ids.insert(component.id());
                    return true;
                });

                QVector<StreamResult> appstreamResults;
                if (!ids.isEmpty()) {
                    auto allResults = resultsByComponents(components);

                    // Filter out results that don't match the search query well enough
                    appstreamResults = kFilter<QVector<StreamResult>>(allResults, [&filter](const StreamResult &res) {
                        auto pkResource = qobject_cast<PackageKitResource *>(res.resource);
                        if (!pkResource || pkResource->extendsItself()) {
                            return false;
                        }

                        const QString resName = pkResource->name().toLower();
                        const QString searchLower = filter.search.toLower();

                        // Check if the name has ANY relevance to the search query
                        bool nameMatches = resName.contains(searchLower) || searchLower.contains(resName);

                        // Check if any word from search matches the name
                        bool wordMatches = false;
                        const QStringList searchWords = filter.search.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                        for (const QString &word : searchWords) {
                            if (word.length() >= 3 && resName.contains(word.toLower())) {
                                wordMatches = true;
                                break;
                            }
                        }

                        // If sortScore from AppStream is low (< 50), require name match
                        if (res.sortScore < 50) {
                            return nameMatches || wordMatches;
                        }

                        // For medium scores (50-70), still prefer name matches
                        if (res.sortScore < 70) {
                            return nameMatches || wordMatches;
                        }

                        // High score results (>= 70) are kept only if there's at least word match
                        return nameMatches || wordMatches || res.sortScore >= 90;
                    });
                }

                // Search via PackageKit when:
                // 1. There's a search query, OR
                // 2. Browsing a top-level category (to show all packages from all repos)
                bool isTopLevelCategory = filter.category && !filter.category->parentCategory();
                bool shouldSearchPackageKit = !filter.search.isEmpty() || isTopLevelCategory;

                if (shouldSearchPackageKit) {
                    PackageKit::Transaction *pkTransaction;
                    if (!filter.search.isEmpty()) {
                        pkTransaction = PackageKit::Daemon::searchNames(filter.search, PackageKit::Transaction::FilterNotSource);
                    } else {
                        // For top-level categories without search, get all GUI applications only
                        pkTransaction = PackageKit::Daemon::getPackages(PackageKit::Transaction::FilterApplication | PackageKit::Transaction::FilterNotSource);
                    }
                    QPointer<PKResultsStream> streamPtr(stream);

                    // Collect found packages in a local set to avoid relying on m_packagesToAdd
                    auto foundPackages = std::make_shared<QSet<AbstractResource*>>();

                    connect(pkTransaction, &PackageKit::Transaction::package, this, [this, foundPackages](PackageKit::Transaction::Info info, const QString &packageId, const QString &summary) {
                        addPackageNotArch(info, packageId, summary);

                        // Also track the resource directly
                        const QString packageName = PackageKit::Daemon::packageName(packageId);
                        QSet<AbstractResource *> resources = resourcesByPackageName(packageName);
                        foundPackages->unite(resources);
                    });
                    connect(pkTransaction, &PackageKit::Transaction::errorCode, this, [](PackageKit::Transaction::Error, const QString &) {
                        // Silently ignore errors
                    });
                    connect(pkTransaction, &PackageKit::Transaction::finished, this, [this, streamPtr, filter, pkTransaction, appstreamResults, foundPackages]() {
                        Q_UNUSED(pkTransaction); // Keep transaction alive until finished

                        if (!streamPtr) {
                            includePackagesToAdd();
                            return;
                        }

                        // Collect PackageKit results - use foundPackages which includes both new and existing
                        QVector<StreamResult> pkResults;
                        for (auto resource : std::as_const(*foundPackages)) {
                            auto pkResource = qobject_cast<PackageKitResource *>(resource);
                            if (pkResource && !pkResource->extendsItself()) {
                                // Calculate relevance score based on search query match
                                uint sortScore = 0;
                                const QString resName = pkResource->name().toLower();
                                const QString searchLower = filter.search.toLower();

                                // Split package name by dash/underscore to check each part
                                const QStringList nameParts = resName.split(QRegularExpression(QStringLiteral("[-_]")), Qt::SkipEmptyParts);

                                // Exact match gets highest priority (100)
                                if (resName == searchLower) {
                                    sortScore = 100;
                                }
                                // Check if search query exactly matches any part of the package name (95)
                                // e.g., "chrome" matches "google-chrome-stable"
                                else {
                                    bool exactPartMatch = false;
                                    for (const QString &part : nameParts) {
                                        if (part == searchLower) {
                                            exactPartMatch = true;
                                            sortScore = 95;
                                            break;
                                        }
                                    }

                                    if (!exactPartMatch) {
                                        // Starts with search query gets high priority (80)
                                        if (resName.startsWith(searchLower)) {
                                            sortScore = 80;
                                        }
                                        // Any part starts with search query (75)
                                        else {
                                            bool partStartsWith = false;
                                            for (const QString &part : nameParts) {
                                                if (part.startsWith(searchLower)) {
                                                    partStartsWith = true;
                                                    sortScore = 75;
                                                    break;
                                                }
                                            }

                                            if (!partStartsWith) {
                                                // Contains search query gets medium priority (60)
                                                if (resName.contains(searchLower)) {
                                                    sortScore = 60;
                                                }
                                                // Otherwise check if search words match (40)
                                                else {
                                                    const QStringList searchWords = filter.search.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                                                    bool hasMatch = false;
                                                    for (const QString &word : searchWords) {
                                                        if (resName.contains(word.toLower())) {
                                                            hasMatch = true;
                                                            break;
                                                        }
                                                    }
                                                    if (hasMatch) {
                                                        sortScore = 40;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                // Only add if there's some relevance
                                if (sortScore > 0) {
                                    pkResults << StreamResult(pkResource, sortScore);
                                }
                            }
                        }

                        // Merge AppStream and PackageKit results
                        QVector<StreamResult> allResults = appstreamResults;

                        // Add PackageKit results, avoiding duplicates
                        for (const auto &pkResult : pkResults) {
                            bool isDuplicate = false;
                            for (const auto &asResult : appstreamResults) {
                                if (asResult.resource == pkResult.resource) {
                                    isDuplicate = true;
                                    break;
                                }
                            }
                            if (!isDuplicate) {
                                allResults.append(pkResult);
                            }
                        }

                        // Sort by sortScore (descending) - higher score = more relevant
                        std::sort(allResults.begin(), allResults.end(), [](const StreamResult &a, const StreamResult &b) {
                            return a.sortScore > b.sortScore;
                        });

                        // Send all results sorted by relevance
                        if (!allResults.isEmpty()) {
                            streamPtr->sendResources(allResults, false);
                        }

                        includePackagesToAdd();
                        streamPtr->finish();
                    });
                } else {
                    // No search, just send AppStream results
                    if (!appstreamResults.isEmpty()) {
                        stream->sendResources(appstreamResults, filter.state != AbstractResource::Broken);
                    }
                    stream->finish();
                }
            });
        });
    }
}

PKResultsStream *PackageKitBackend::findResourceByPackageName(const QUrl &url)
{
    if (url.isLocalFile()) {
        QMimeDatabase db;
        const auto mime = db.mimeTypeForUrl(url);
        if (mime.inherits(QStringLiteral("application/vnd.debian.binary-package")) //
            || mime.inherits(QStringLiteral("application/x-rpm")) //
            || mime.inherits(QStringLiteral("application/x-tar")) //
            || mime.inherits(QStringLiteral("application/x-zstd-compressed-tar")) //
            || mime.inherits(QStringLiteral("application/x-xz-compressed-tar"))) {
            return PKResultsStream::create(this, QStringLiteral("PackageKitStream-localpkg"), QVector<StreamResult>{new LocalFilePKResource(url, this)}).data();
        }
    } else if (url.scheme() == QLatin1String("appstream")) {
        const auto appstreamIds = AppStreamUtils::appstreamIds(url);
        if (appstreamIds.isEmpty()) {
            Q_EMIT passiveMessage(i18n("Malformed appstream url '%1'", url.toDisplayString()));
        } else {
            return deferredResultStream(u"PackageKitStream-appstream-url"_s, [this, appstreamIds](PKResultsStream *stream) {
                auto toSend = QSet<StreamResult>();
                toSend.reserve(appstreamIds.size());
                for (const auto &appstreamId : appstreamIds) {
                    const auto components = componentsById(appstreamId);
                    if (components.isEmpty()) {
                        continue;
                    }
                    const auto resources = resultsByComponents(components);
                    for (const auto &resource : resources) {
                        toSend.insert(resource);
                    }
                }
                stream->sendResources(QList(toSend.constBegin(), toSend.constEnd()));
            });
        }
    }
    return PKResultsStream::create(this, QStringLiteral("PackageKitStream-unknown-url"), QVector<StreamResult>{}).data();
}

template<typename T>
T PackageKitBackend::resourcesByComponents(const AppStream::ComponentBox &components) const
{
    T ret;
    ret.reserve(components.size());
    QSet<QString> done;
    for (const auto &component : components) {
        if (component.packageNames().isEmpty() || done.contains(component.id())) {
            continue;
        }
        done += component.id();
        auto r = addComponent(component);
        Q_ASSERT(r);
        ret << r;
    }
    return ret;
}

QVector<StreamResult> PackageKitBackend::resultsByComponents(const AppStream::ComponentBox &components) const
{
    QVector<StreamResult> ret;
    ret.reserve(components.size());
    QSet<QString> done;
    for (const auto &component : components) {
        if (component.packageNames().isEmpty() || component.kind() == AppStream::Component::KindFirmware || done.contains(component.id())) {
            continue;
        }
        done += component.id();
        auto r = addComponent(component);
        Q_ASSERT(r);
        ret << StreamResult(r, component.sortScore());
    }
    return ret;
}

bool PackageKitBackend::hasSecurityUpdates() const
{
    return m_hasSecurityUpdates;
}

int PackageKitBackend::updatesCount() const
{
    auto offline = PackageKit::Daemon::global()->offline();
    if (offline->updateTriggered() || offline->upgradeTriggered()) {
        return 0;
    }

    QSet<QString> packages;
    const auto toUpgrade = upgradeablePackages();
    for (auto resource : toUpgrade) {
        packages.insert(resource->packageName());
    }
    return packages.count();
}

Transaction *PackageKitBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Transaction *transaction = nullptr;
    if (!addons.addonsToInstall().isEmpty()) {
        QVector<AbstractResource *> appsToInstall = resourcesByPackageNames<QVector<AbstractResource *>>(addons.addonsToInstall());
        if (!app->isInstalled()) {
            appsToInstall << app;
        }
        transaction = new PKTransaction(appsToInstall, Transaction::ChangeAddonsRole);
    } else if (!app->isInstalled()) {
        transaction = installApplication(app);
    }

    if (!addons.addonsToRemove().isEmpty()) {
        const auto appsToRemove = resourcesByPackageNames<QVector<AbstractResource *>>(addons.addonsToRemove());
        transaction = new PKTransaction(appsToRemove, Transaction::RemoveRole);
    }

    return transaction;
}

Transaction *PackageKitBackend::installApplication(AbstractResource *app)
{
    return new PKTransaction({app}, Transaction::InstallRole);
}

Transaction *PackageKitBackend::removeApplication(AbstractResource *app)
{
    if (!qobject_cast<PackageKitResource *>(app)) {
        Q_EMIT passiveMessage(i18n("Cannot remove '%1'", app->name()));
        return nullptr;
    }
    return new PKTransaction({app}, Transaction::RemoveRole);
}

QSet<AbstractResource *> PackageKitBackend::upgradeablePackages() const
{
    if (!m_packagesToAdd.isEmpty()) {
        return {};
    }

    QSet<AbstractResource *> ret;
    ret.reserve(m_updatesPackageId.size());
    for (const QString &pkgid : std::as_const(m_updatesPackageId)) {
        const QString pkgname = PackageKit::Daemon::packageName(pkgid);
        const auto pkgs = resourcesByPackageName(pkgname);
        if (pkgs.isEmpty()) {
            qWarning() << "PackageKitBackend: Couldn't find resource for" << pkgid;
        }
        ret.unite(pkgs);
    }
    return kFilter<QSet<AbstractResource *>>(ret, [](AbstractResource *resource) {
        return !static_cast<PackageKitResource *>(resource)->extendsItself();
    });
}

void PackageKitBackend::addPackageToUpdate(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    if (info == PackageKit::Transaction::InfoBlocked) {
        return;
    }

    if (info == PackageKit::Transaction::InfoRemoving || info == PackageKit::Transaction::InfoObsoleting) {
        // Don't try updating packages which need to be removed
        return;
    }

    if (info == PackageKit::Transaction::InfoSecurity) {
        m_hasSecurityUpdates = true;
    }

    m_updatesPackageId += packageId;
    addPackage(info, packageId, summary, true);
}

void PackageKitBackend::getUpdatesFinished(PackageKit::Transaction::Exit, uint)
{
    if (!m_updatesPackageId.isEmpty()) {
        resolvePackages(kTransform<QStringList>(m_updatesPackageId, [](const QString &pkgid) {
            return PackageKit::Daemon::packageName(pkgid);
        }));
    }

    m_updater->setProgressing(false);

    includePackagesToAdd();
    if (!m_isFetching) {
        Q_EMIT updatesCountChanged();
    }

    if (!m_updater->isDistroUpgrade() && !PackageKit::Daemon::global()->offline()->upgradeTriggered()) {
        const auto loadDone = [this] {
            auto nextRelease = AppStreamIntegration::global()->getDistroUpgrade(m_appdata->get());
            if (nextRelease) {
                foundNewMajorVersion(*nextRelease);
            }
        };
        if (m_appdataLoaded) {
            loadDone();
        } else {
            connect(m_appdata.get(), &AppStream::ConcurrentPool::loadFinished, this, loadDone);
        }
    }
}

void PackageKitBackend::foundNewMajorVersion(const AppStream::Release &release)
{
    const QString upgradeVersion = release.version();
    const QString newDistroVersionText = AppStreamIntegration::global()->osRelease()->name() + QLatin1Char(' ') + upgradeVersion;

    QString info;
    // Message to display when:
    // - A new major version is available
    // - An update to the current version is available or pending a reboot
    info = i18nc("@info:status %1 is a new major version of the user's distro",
                 "<b>%1 is now available.</b>\n"
                 "To be able to upgrade to this new version, first apply all available updates, and then restart the system.",
                 newDistroVersionText);
    QSharedPointer<InlineMessage> updateBeforeMajorUpgradeMessage =
        QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("system-software-update"), info);

    // Message to display when:
    // - A new major version is available
    // - No update to the current version are available or pending a reboot
    DiscoverAction *majorUpgrade = new DiscoverAction(QStringLiteral("system-upgrade-symbolic"), i18nc("@action: button", "Upgrade Now"), this);
    connect(majorUpgrade, &DiscoverAction::triggered, this, [this, release, upgradeVersion] {
        if (m_updater->isProgressing()) {
            return;
        }

        m_updatesPackageId.clear();
        m_updater->setProgressing(true);
        m_refresher = PackageKit::Daemon::upgradeSystem(upgradeVersion,
                                                        PackageKit::Transaction::UpgradeKind::UpgradeKindComplete,
                                                        PackageKit::Transaction::TransactionFlagSimulate);
        m_refresher->setHints(globalHints() << QStringLiteral("cache-age=86400" /* 24*60*60 */));
        connect(m_refresher, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
        connect(m_refresher, &PackageKit::Transaction::percentageChanged, this, &PackageKitBackend::fetchingUpdatesProgressChanged);
        connect(m_refresher, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(m_refresher, &PackageKit::Transaction::finished, this, [this, release](PackageKit::Transaction::Exit e, uint x) {
            m_updater->setDistroUpgrade(release);
            getUpdatesFinished(e, x);
        });
        Q_EMIT inlineMessageChanged({});
        Q_EMIT ResourcesModel::global()->switchToUpdates();
    });

    info = i18nc("@info:status %1 is a new major version of the user's distro", "%1 is now available.", newDistroVersionText);
    QSharedPointer<InlineMessage> majorUpgradeAvailableMessage =
        QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("system-software-update"), info, majorUpgrade);

    // Allow upgrade only if up to date on the current release
    if (!m_updatesPackageId.isEmpty()) {
        Q_EMIT inlineMessageChanged(updateBeforeMajorUpgradeMessage);
        return;
    }

    // No updates pending or available. We are good to offer the upgrade to the
    // next major version!
    Q_EMIT inlineMessageChanged(majorUpgradeAvailableMessage);
}

// Copy of Transaction::packageName that doesn't create a copy but just pass a reference
// It's an optimisation as there's a bunch of allocations that happen from packageName
// Having packageName return a QStringRef or a QStringView would fix this issue.
// TODO Qt 6: Have packageName and similars return a QStringView
static QStringView TransactionPackageName(const QString &packageID)
{
    return QStringView(packageID).left(packageID.indexOf(QLatin1Char(';')));
}

bool PackageKitBackend::isPackageNameUpgradeable(const PackageKitResource *resource) const
{
    const QString name = resource->packageName();
    for (const QString &pkgid : m_updatesPackageId) {
        if (TransactionPackageName(pkgid) == name) {
            return true;
        }
    }
    return false;
}

QSet<QString> PackageKitBackend::upgradeablePackageId(const PackageKitResource *resource) const
{
    QSet<QString> ids;
    const QString name = resource->packageName();
    for (const QString &pkgid : m_updatesPackageId) {
        if (TransactionPackageName(pkgid) == name) {
            ids.insert(pkgid);
        }
    }
    return ids;
}

void PackageKitBackend::fetchDetails(const QString &pkgid)
{
    m_details.add(pkgid);
}

void PackageKitBackend::fetchDetails(const QSet<QString> &pkgid)
{
    m_details.add(pkgid);
}

void PackageKitBackend::performDetailsFetch(const QSet<QString> &pkgids)
{
    Q_ASSERT(!pkgids.isEmpty());
    const auto ids = pkgids.values();

    PackageKit::Transaction *transaction = PackageKit::Daemon::getDetails(ids);
    connect(transaction, &PackageKit::Transaction::details, this, &PackageKitBackend::packageDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
}

void PackageKitBackend::checkDaemonRunning()
{
    if (!PackageKit::Daemon::isRunning()) {
        qWarning() << "PackageKit stopped running!";
    } else {
        updateProxy();
    }
}

AbstractBackendUpdater *PackageKitBackend::backendUpdater() const
{
    return m_updater;
}

QVector<AbstractResource *> PackageKitBackend::extendedBy(const QString &id) const
{
    const auto components = m_appdata->componentsByExtends(id);
    return resourcesByComponents<QVector<AbstractResource *>>(components.result());
}

AbstractReviewsBackend *PackageKitBackend::reviewsBackend() const
{
    return m_reviews.data();
}

QString PackageKitBackend::displayName() const
{
    auto osRelease = AppStreamIntegration::global()->osRelease();
    return osRelease->name();
}

int PackageKitBackend::fetchingUpdatesProgress() const
{
    if (!m_refresher) {
        return 100;
    }

    int percentage = m_refresher->percentage();
    if (percentage > 100) {
        return m_refresher->property("lastPercentage").toInt();
    }
    m_refresher->setProperty("lastPercentage", percentage);
    return percentage;
}

uint PackageKitBackend::fetchingUpdatesProgressWeight() const
{
    return 50;
}

InlineMessage *PackageKitBackend::explainDysfunction() const
{
    const auto error = m_appdata->lastError();
    if (!error.isEmpty()) {
        return new InlineMessage(InlineMessage::Error, QStringLiteral("network-disconnect"), error);
    }

    if (!PackageKit::Daemon::isRunning()) {
        return new InlineMessage(InlineMessage::Error,
                                 u"run-build-prune-symbolic"_s,
                                 i18nc("@info %2 is the name of the operating system",
                                       "The PackageKit background service stopped unexpectedly.<nl/></nl/>Please <link url='%1'>report this issue to %2</link>",
                                       KOSRelease().bugReportUrl(),
                                       KOSRelease().name()));
    }

    return AbstractResourcesBackend::explainDysfunction();
}

void PackageKitBackend::loadAllPackages()
{
    if (m_allPackagesLoaded) {
        return;
    }
    const auto components = m_appdata->components().result();
    for (const auto &component : components) {
        if (!component.packageNames().isEmpty()) {
            addComponent(component);
        }
    }
    includePackagesToAdd();
    m_allPackagesLoaded = true;
}

void PackageKitBackend::aboutTo(AboutToAction action)
{
    PackageKit::Offline::Action packageKitAction;
    switch (action) {
    case Reboot:
        packageKitAction = PackageKit::Offline::ActionReboot;
        break;
    case PowerOff:
        packageKitAction = PackageKit::Offline::ActionPowerOff;
        break;
    }
    m_updater->setOfflineUpdateAction(packageKitAction);
}

#include "PackageKitBackend.moc"
#include "moc_PackageKitBackend.cpp"
