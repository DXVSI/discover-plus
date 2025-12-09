/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DiscoverObject.h"
#include "CachedNetworkAccessManager.h"
#include "DiscoverBackendsFactory.h"
#include "DiscoverDeclarativePlugin.h"
#include "FeaturedModel.h"
#include "LimitedRowCountProxyModel.h"
#include "OdrsAppsModel.h"
#include "UnityLauncher.h"
#include <Transaction/TransactionModel.h>

// Qt includes
#include "discover_debug.h"
#include <QClipboard>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSessionManager>
#include <QTimer>
#include <QtQuick/QQuickItem>
#include <qqml.h>

// KDE includes
#include <KAboutData>
#include <KAuthorized>
#include <KConfigGroup>
#include <KCrash>
#include <KLocalizedContext>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KOSRelease>
#include <KSharedConfig>
#include <KStatusNotifierItem>
#include <KUiServerV2JobTracker>
#include <KUriFilter>
#include <kcoreaddons_version.h>
// #include <KSwitchLanguageDialog>

// DiscoverCommon includes
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>

#include <QMimeDatabase>
#include <cmath>
#include <functional>
#include <resources/StoredResultsStream.h>
#include <unistd.h>
#include <utils.h>

#ifdef WITH_FEEDBACK
#include "plasmauserfeedback.h"
#endif
#include "FedoraRepoManager.h"
#include "PowerManagementInterface.h"
#include "RefreshNotifier.h"
#include "discoversettings.h"
#include <resources/ResourcesUpdatesModel.h>

using namespace Qt::StringLiterals;

class CachedNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
    virtual QNetworkAccessManager *create(QObject *parent) override
    {
        return new CachedNetworkAccessManager(QStringLiteral("images"), parent);
    }
};

static void scheduleShutdownWithErrorCode()
{
    QTimer::singleShot(0, QCoreApplication::instance(), []() {
        QCoreApplication::instance()->exit(1);
    });
}

DiscoverObject::DiscoverObject(const QVariantMap &initialProperties)
    : QObject()
    , m_engine(new QQmlApplicationEngine)
    , m_networkAccessManagerFactory(std::make_unique<CachedNetworkAccessManagerFactory>())
{
    setObjectName(QStringLiteral("DiscoverMain"));
    m_engine->rootContext()->setContextObject(new KLocalizedQmlContext(m_engine.get()));
    auto factory = m_engine->networkAccessManagerFactory();
    m_engine->setNetworkAccessManagerFactory(nullptr);
    delete factory;
    m_engine->setNetworkAccessManagerFactory(m_networkAccessManagerFactory.get());

    new RefreshNotifier(this);

    const auto uriApp = "org.kde.discover.app";

    qmlRegisterType<UnityLauncher>(uriApp, 1, 0, "UnityLauncher");
    qmlRegisterType<LimitedRowCountProxyModel>(uriApp, 1, 0, "LimitedRowCountProxyModel");
    qmlRegisterType<FeaturedModel>(uriApp, 1, 0, "FeaturedModel");
    qmlRegisterType<OdrsAppsModel>(uriApp, 1, 0, "OdrsAppsModel");
    qmlRegisterType<PowerManagementInterface>(uriApp, 1, 0, "PowerManagementInterface");
#ifdef WITH_FEEDBACK
    qmlRegisterSingletonType<PlasmaUserFeedback>(uriApp, 1, 0, "UserFeedbackSettings", [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        auto r = new PlasmaUserFeedback(KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback"), KConfig::NoGlobals));
        r->setParent(engine);
        return r;
    });
#endif
    qmlRegisterSingletonType<DiscoverSettings>(uriApp, 1, 0, "DiscoverSettings", [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        auto r = new DiscoverSettings;
        r->setParent(engine);
        connect(r, &DiscoverSettings::installedPageSortingChanged, r, &DiscoverSettings::save);
        connect(r, &DiscoverSettings::appsListPageSortingChanged, r, &DiscoverSettings::save);
        return r;
    });
    qmlRegisterSingletonType<FedoraRepoManager>(uriApp, 1, 0, "FedoraRepoManager", FedoraRepoManager::create);
    qmlRegisterAnonymousType<QQuickView>(uriApp, 1);

    qmlRegisterAnonymousType<KAboutData>(uriApp, 1);
    qmlRegisterAnonymousType<KAboutLicense>(uriApp, 1);
    qmlRegisterAnonymousType<KAboutPerson>(uriApp, 1);
    qmlRegisterAnonymousType<DiscoverObject>(uriApp, 1);

    auto uri = "org.kde.discover";
    DiscoverDeclarativePlugin *plugin = new DiscoverDeclarativePlugin;
    plugin->setParent(this);
    plugin->initializeEngine(m_engine.get(), uri);
    plugin->registerTypes(uri);

    m_engine->rootContext()->setContextProperty(QStringLiteral("app"), this);

    const auto discoverQmlUri = u"org.kde.discover.qml"_s;

    auto navigation = m_engine->singletonInstance<QObject *>(discoverQmlUri, u"Navigation"_s);
    if (!navigation) {
        qCWarning(DISCOVER_LOG) << "Failed to import Navigation singleton";
        scheduleShutdownWithErrorCode();
        return;
    }

    {
        QQmlComponent component(m_engine.get(), discoverQmlUri, u"DiscoverWindow"_s);
        auto object = component.beginCreate(m_engine->rootContext());
        auto window = qobject_cast<QQuickWindow *>(object);
        if (!object || !window) {
            qCWarning(DISCOVER_LOG) << "Failed to create main window:" << component.errorString();
            scheduleShutdownWithErrorCode();
            component.completeCreate();
            if (object) {
                delete object;
            }
            return;
        }
        navigation->setProperty("window", QVariant::fromValue(object));
        component.setInitialProperties(object, initialProperties);
        component.completeCreate();
        initMainWindow(window);
    }

    auto action = new OneTimeAction(
        [this]() {
            bool found = DiscoverBackendsFactory::hasRequestedBackends();
            bool hasPackageKit = false;
            const auto backends = ResourcesModel::global()->backends();
            for (auto b : backends) {
                found |= b->hasApplications();
                hasPackageKit = hasPackageKit || (b->name() == "packagekit-backend"_L1 && b->isValid());
            }

            const KOSRelease osRelease;
            const QString distroName = osRelease.name();
            const bool isArch = osRelease.id() == u"arch"_s || osRelease.idLike().contains(u"arch"_s);
            if (!found) {
                QString errorText = i18nc("@title %1 is the name", "%1 is not configured for installing apps through Discover—only app add-ons", distroName);
                QString errorExplanation = xi18nc("@info:usagetip %2 is the name of the operating system",
                                                  "To use Discover for apps, install your preferred module on the <interface>Settings"
                                                  "</interface> page, under <interface>Missing Backends</interface>."
                                                  "<nl/><nl/>Please <link url='%1'>report this issue to %2.</link>",
                                                  osRelease.bugReportUrl(),
                                                  distroName);
                if (isArch) {
                    errorExplanation = xi18nc("@info:usagetip %1 is the distro name; in this case it always contains 'Arch Linux'",
                                              "To use Discover for apps, install"
                                              " <link url='https://wiki.archlinux.org/title/Flatpak#Installation'>Flatpak</link>"
                                              " using the <command>pacman</command> package manager.<nl/><nl/>"
                                              " Review <link url='https://archlinux.org/packages/extra/x86_64/discover/'>%1's packaging for Discover</link>",
                                              distroName);
                }

                Q_EMIT openErrorPage(errorText, errorExplanation, QString(), QString(), QString());
            } else if (hasPackageKit && isArch) {
                const QString errorText = xi18nc("@info:usagetip %1 is the distro name",
                                                 "Support for managing packages from %1 is incomplete; you may experience any number of problems."
                                                 " Do not report bugs to KDE. It is highly recommended to uninstall the"
                                                 " <resource>packagekit-qt6</resource> package and use Discover only to manage Flatpaks, Snaps,"
                                                 " and Add-Ons."
                                                 "<para>%1 maintainers recommended instead using the <command>pacman</command> command-line tool"
                                                 " to manage packages.</para>",
                                                 distroName);
                m_homePageMessage = std::make_unique<InlineMessage>(InlineMessage::Warning, QString(), errorText);
                Q_EMIT homeMessageChanged();
            }
        },
        this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

DiscoverObject::~DiscoverObject()
{
    m_isDeleting = true;
    m_engine->deleteLater();
}

bool DiscoverObject::isRoot()
{
    return ::getuid() == 0;
}

QStringList DiscoverObject::modes() const
{
    QStringList ret;
    QObject *obj = mainWindow();
    for (int i = obj->metaObject()->propertyOffset(); i < obj->metaObject()->propertyCount(); i++) {
        QMetaProperty p = obj->metaObject()->property(i);
        QByteArray compName = p.name();
        if (compName.startsWith("top") && compName.endsWith("Comp")) {
            ret += QString::fromLatin1(compName.mid(3, compName.length() - 7));
        }
    }
    return ret;
}

void DiscoverObject::openMode(const QString &_mode)
{
    if (!m_mainWindow) {
        qCWarning(DISCOVER_LOG) << "could not get the main object";
        return;
    }

    if (!modes().contains(_mode, Qt::CaseInsensitive))
        qCWarning(DISCOVER_LOG) << "unknown mode" << _mode << modes();

    QString mode = _mode;
    mode[0] = mode[0].toUpper();

    const QByteArray propertyName = "top" + mode.toLatin1() + "Comp";
    const QVariant modeComp = m_mainWindow->property(propertyName.constData());
    if (!modeComp.isValid()) {
        qCWarning(DISCOVER_LOG) << "couldn't open mode" << _mode;
    } else {
        m_mainWindow->setProperty("currentTopLevel", modeComp);
    }
}

void DiscoverObject::openMimeType(const QString &mime)
{
    Q_EMIT listMimeInternal(mime);
}

void DiscoverObject::showLoadingPage()
{
    if (m_mainWindow) {
        m_mainWindow->setProperty("currentTopLevel", QStringLiteral(DISCOVER_BASE_URL "/LoadingPage.qml"));
    }
}

void DiscoverObject::openCategory(const QString &category)
{
    showLoadingPage();
    auto action = new OneTimeAction(
        [this, category]() {
            std::shared_ptr<Category> cat = CategoryModel::global()->findCategoryByName(category);
            if (cat) {
                Q_EMIT listCategoryInternal(cat);
            } else {
                openMode(QStringLiteral("Browsing"));
                showError(i18n("Could not find category '%1'", category));
            }
        },
        this);

    if (CategoryModel::global()->rootCategories().isEmpty()) {
        connect(CategoryModel::global(), &CategoryModel::rootCategoriesChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverObject::startHeadlessUpdate()
{
    openMode(u"update"_s);
    QQuickItem *updatesPage = m_mainWindow->property("leftPage").value<QQuickItem *>();
    updatesPage->setProperty("startHeadlessUpdate", true);
}

void DiscoverObject::openLocalPackage(const QUrl &localfile)
{
    if (!QFile::exists(localfile.toLocalFile())) {
        showError(i18n("Trying to open inexisting file '%1'", localfile.toString()));
        openMode(QStringLiteral("Browsing"));
        return;
    }
    showLoadingPage();
    auto action = new OneTimeAction(
        [this, localfile]() {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = localfile;
            auto stream = new StoredResultsStream({ResourcesModel::global()->search(f)});
            connect(stream, &StoredResultsStream::finishedResources, this, [this, localfile](const QVector<StreamResult> &res) {
                if (res.count() == 1) {
                    Q_EMIT openApplicationInternal(res.first().resource);
                } else {
                    QMimeDatabase db;
                    auto mime = db.mimeTypeForUrl(localfile);
                    auto fIsFlatpakBackend = [](AbstractResourcesBackend *backend) {
                        return backend->metaObject()->className() == QByteArray("FlatpakBackend");
                    };
                    if (mime.name().startsWith(QLatin1String("application/vnd.flatpak"))
                        && !kContains(ResourcesModel::global()->backends(), fIsFlatpakBackend)) {
                        openApplication(QUrl(QStringLiteral("appstream://org.kde.discover.flatpak")));
                        showError(i18n("Cannot interact with flatpak resources without the flatpak backend %1. Please install it first.",
                                       localfile.toDisplayString()));
                    } else {
                        openMode(QStringLiteral("Browsing"));
                        showError(i18n("Could not open %1", localfile.toDisplayString()));
                    }
                }
            });
        },
        this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverObject::openApplication(const QUrl &url)
{
    Q_ASSERT(!url.isEmpty());
    showLoadingPage();
    auto action = new OneTimeAction(
        [this, url]() {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = url;
            auto stream = new StoredResultsStream({ResourcesModel::global()->search(f)});
            connect(stream, &StoredResultsStream::finishedResources, this, [this, url](const QVector<StreamResult> &res) {
                if (res.count() >= 1) {
                    QPointer<QTimer> timeout = new QTimer(this);
                    timeout->setSingleShot(true);
                    timeout->setInterval(20000);
                    connect(timeout, &QTimer::timeout, timeout, &QTimer::deleteLater);

                    auto openResourceOrWait = [this, res, timeout] {
                        if (m_isDeleting) {
                            return false;
                        }
                        auto idx = kIndexOf(res, [](auto res) {
                            return res.resource->isInstalled();
                        });
                        if (idx < 0) {
                            bool oneBroken = kContains(res, [](auto res) {
                                return res.resource->state() == AbstractResource::Broken;
                            });
                            if (oneBroken && timeout) {
                                return false;
                            }

                            idx = 0;
                        }
                        Q_EMIT openApplicationInternal(res[idx].resource);
                        return true;
                    };

                    if (!openResourceOrWait()) {
                        auto f = new OneTimeAction(0, openResourceOrWait, this);
                        for (auto r : res) {
                            if (r.resource->state() == AbstractResource::Broken) {
                                connect(r.resource, &AbstractResource::stateChanged, f, &OneTimeAction::trigger);
                            }
                        }
                        timeout->start();
                        connect(timeout, &QTimer::destroyed, f, &OneTimeAction::trigger);
                    } else {
                        delete timeout;
                    }
                } else if (url.scheme() == QLatin1String("snap")) {
                    openApplication(QUrl(QStringLiteral("appstream://org.kde.discover.snap")));
                    showError(i18n("Please make sure Snap support is installed"));
                } else {
                    const QString errorText = i18n(
                        "The requested application could not be found "
                        "in any available software repositories");
                    const QString errorExplanation = xi18nc("@info %1 is an additional error reference and %3 is the name of the operating system",
                                                            "(No entry for %1)<nl/><nl/>Please <link url='%2'>report this issue to %3.</link>",
                                                            url.toDisplayString(),
                                                            KOSRelease().bugReportUrl(),
                                                            KOSRelease().name());

                    Q_EMIT openErrorPage(errorText, errorExplanation, QString(), QString(), QString());
                }
            });
        },
        this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

class TransactionsJob : public KJob
{
public:
    void start() override
    {
        // no-op, this is just observing

        setTotalAmount(Items, TransactionModel::global()->rowCount());
        setPercent(TransactionModel::global()->progress());
        connect(TransactionModel::global(), &TransactionModel::lastTransactionFinished, this, &TransactionsJob::emitResult);
        connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &TransactionsJob::refreshInfo);
        connect(TransactionModel::global(), &TransactionModel::progressChanged, this, [this] {
            setPercent(TransactionModel::global()->progress());
        });
        refreshInfo();
    }

    void refreshInfo()
    {
        if (TransactionModel::global()->rowCount() == 0) {
            return;
        }

        setProcessedAmount(Items, totalAmount(Items) - TransactionModel::global()->rowCount() + 1);
        auto firstTransaction = TransactionModel::global()->transactions().constFirst();
        Q_EMIT description(this, firstTransaction->name());
    }

    void cancel()
    {
        setError(KJob::KilledJobError /*KIO::ERR_USER_CANCELED*/);
        deleteLater();
    }
};

bool DiscoverObject::quitWhenIdle()
{
    if (!isBusy()) {
        return true;
    }

    if (!m_sni) {
        m_sni.reset(new KStatusNotifierItem);
        m_sni->setStatus(KStatusNotifierItem::Active);
        m_sni->setIconByName(QStringLiteral("plasmadiscover"));
        m_sni->setTitle(i18n("Discover"));
        m_sni->setToolTip(QStringLiteral("process-working-symbolic"),
                          i18n("Discover"),
                          i18n("Discover was closed before certain tasks were done, waiting for it to finish."));
        m_sni->setStandardActionsEnabled(false);

        connect(TransactionModel::global(), &TransactionModel::countChanged, this, &DiscoverObject::reconsiderQuit);
        connect(m_sni.get(), &KStatusNotifierItem::activateRequested, this, &DiscoverObject::restore);

        auto job = new TransactionsJob;
        job->setParent(m_sni.get());
        auto tracker = new KUiServerV2JobTracker(m_sni.get());
        tracker->registerJob(job);
        job->start();
        connect(m_sni.get(), &KStatusNotifierItem::activateRequested, job, &TransactionsJob::cancel);

        m_mainWindow->hide();
    }
    return false;
}

void DiscoverObject::restore()
{
    if (m_sni) {
        disconnect(TransactionModel::global(), &TransactionModel::countChanged, this, &DiscoverObject::reconsiderQuit);
        disconnect(m_sni.get(), &KStatusNotifierItem::activateRequested, this, &DiscoverObject::restore);
        m_sni.reset();
    }
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}

bool DiscoverObject::isBusy() const
{
    return TransactionModel::global()->rowCount() > 0;
}

void DiscoverObject::reconsiderQuit()
{
    if (isBusy()) {
        return;
    }

    m_sni.reset();
    // Let the job UI to finalise properly
    QTimer::singleShot(20, qGuiApp, &QCoreApplication::quit);
}

void DiscoverObject::initMainWindow(QQuickWindow *mainWindow)
{
    Q_ASSERT(mainWindow);

    m_mainWindow = std::unique_ptr<QQuickWindow>(mainWindow);

    connect(m_mainWindow.get(), &QQuickView::sceneGraphError, this, [](QQuickWindow::SceneGraphError /*error*/, const QString &message) {
        KCrash::setErrorMessage(message);
        qFatal("%s", qPrintable(message));
    });

    m_mainWindow->installEventFilter(this);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        m_mainWindow.reset();
    });

    connect(qGuiApp, &QGuiApplication::commitDataRequest, this, [this](QSessionManager &sessionManager) {
        if (!quitWhenIdle()) {
            sessionManager.cancel();
        }
    });
}

bool DiscoverObject::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_mainWindow.get()) {
        return false;
    }

    if (event->type() == QEvent::Close) {
        if (!quitWhenIdle()) {
            return true;
        }
    }
    // } else if (event->type() == QEvent::ShortcutOverride) {
    //     qCWarning(DISCOVER_LOG) << "Action conflict" << event;
    // }
    return false;
}

QString DiscoverObject::iconName(const QIcon &icon)
{
    return icon.name();
}

void DiscoverObject::switchApplicationLanguage()
{
    // auto langDialog = new KSwitchLanguageDialog(nullptr);
    // connect(langDialog, SIGNAL(finished(int)), this, SLOT(dialogFinished()));
    // langDialog->show();
}

class DiscoverTestExecutor : public QObject
{
public:
    DiscoverTestExecutor(QQuickWindow *mainWindow, QQmlEngine *engine, const QUrl &url)
        : QObject(engine)
    {
        connect(engine, &QQmlEngine::quit, this, &DiscoverTestExecutor::finish, Qt::QueuedConnection);

        QQmlComponent *component = new QQmlComponent(engine, url, engine);
        m_testObject = component->create(engine->rootContext());

        if (!m_testObject) {
            qCWarning(DISCOVER_LOG) << "error loading test" << url << m_testObject << component->errors();
            Q_ASSERT(false);
        }

        m_testObject->setProperty("appRoot", QVariant::fromValue(mainWindow));
        connect(engine, &QQmlEngine::warnings, this, &DiscoverTestExecutor::processWarnings);
    }

    void processWarnings(const QList<QQmlError> &warnings)
    {
        for (const QQmlError &warning : warnings) {
            if (warning.url().path().endsWith(QLatin1String("DiscoverTest.qml"))) {
                qCWarning(DISCOVER_LOG) << "Test failed!" << warnings;
                qGuiApp->exit(1);
            }
        }
        m_warnings << warnings;
    }

    void finish()
    {
        if (m_warnings.isEmpty())
            qCDebug(DISCOVER_LOG) << "cool no warnings!";
        else
            qCDebug(DISCOVER_LOG) << "test finished successfully despite" << m_warnings;
        qGuiApp->exit(m_warnings.count());
    }

private:
    QObject *m_testObject;
    QList<QQmlError> m_warnings;
};

void DiscoverObject::loadTest(const QUrl &url)
{
    new DiscoverTestExecutor(m_mainWindow.get(), engine(), url);
}

QQuickWindow *DiscoverObject::mainWindow() const
{
    return m_mainWindow.get();
}

void DiscoverObject::showError(const QString &msg)
{
    QTimer::singleShot(100, this, [msg]() {
        Q_EMIT ResourcesModel::global()->passiveMessage(msg);
    });
}

void DiscoverObject::copyTextToClipboard(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

QUrl DiscoverObject::searchUrl(const QString &searchText)
{
    KUriFilterData filterData(searchText);
    if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::NormalTextFilter)) {
        return filterData.uri();
    }
    return {};
}

QString DiscoverObject::mimeTypeComment(const QString &mimeTypeName)
{
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForName(mimeTypeName);
    if (mimeType.isValid()) {
        return mimeType.comment();
    }
    return mimeTypeName;
}

Q_GLOBAL_STATIC_WITH_ARGS(const bool, s_weAreOnPlasma, (qgetenv("XDG_CURRENT_DESKTOP") == "KDE"))

void DiscoverObject::promptReboot()
{
    if (!s_weAreOnPlasma) {
        qCWarning(DISCOVER_LOG) << "Cannot prompt for reboot outside of Plasma";
    }
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverObject::rebootNow()
{
    const auto backends = ResourcesModel::global()->backends();
    for (auto backend : backends) {
        backend->aboutTo(AbstractResourcesBackend::Reboot);
    }

    QDBusMessage method;
    if (*s_weAreOnPlasma) {
        method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Shutdown"),
                                                QStringLiteral("/Shutdown"),
                                                QStringLiteral("org.kde.Shutdown"),
                                                QStringLiteral("logoutAndReboot"));
    } else {
        method = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.login1"),
                                                QStringLiteral("/org/freedesktop/login1"),
                                                QStringLiteral("org.freedesktop.login1.Manager"),
                                                QStringLiteral("Reboot"));
        method.setArguments({true /*interactive*/});
    }
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverObject::shutdownNow()
{
    bool shouldRebootFirst = false;
    const auto backends = ResourcesModel::global()->backends();
    for (auto backend : backends) {
        if (backend->needsRebootForPowerOffAction()) {
            shouldRebootFirst = true;
            break;
        }
    }

    for (auto backend : backends) {
        backend->aboutTo(AbstractResourcesBackend::PowerOff);
    }

    QDBusMessage method;
    if (*s_weAreOnPlasma) {
        method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Shutdown"),
                                                QStringLiteral("/Shutdown"),
                                                QStringLiteral("org.kde.Shutdown"),
                                                shouldRebootFirst ? QStringLiteral("logoutAndReboot") : QStringLiteral("logoutAndShutdown"));
    } else {
        method = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.login1"),
                                                QStringLiteral("/org/freedesktop/login1"),
                                                QStringLiteral("org.freedesktop.login1.Manager"),
                                                shouldRebootFirst ? QStringLiteral("Reboot") : QStringLiteral("PowerOff"));
        method.setArguments({true /*interactive*/});
    }
    QDBusConnection::sessionBus().asyncCall(method);
}

QString DiscoverObject::describeSources() const
{
    return mainWindow()->property("describeSources").toString();
}

InlineMessage *DiscoverObject::homePageMessage() const
{
    return m_homePageMessage.get();
}

int DiscoverObject::sidebarWidth() const
{
    KConfigGroup grp(KSharedConfig::openStateConfig(), u"MainWindow"_s);
    return grp.readEntry(u"sidebarWidth"_s, -1);
}

void DiscoverObject::setSidebarWidth(int width)
{
    if (width == sidebarWidth()) {
        return;
    }
    KConfigGroup grp(KSharedConfig::openStateConfig(), u"MainWindow"_s);
    grp.writeEntry(u"sidebarWidth"_s, width);
    Q_EMIT sidebarWidthChanged(width);
}

#include "DiscoverObject.moc"
#include "moc_DiscoverObject.cpp"
