/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

// #define QT_QML_DEBUG

#include "DiscoverObject.h"
#include "DiscoverVersion.h"
#include <DiscoverBackendsFactory.h>
#include <KAboutData>
#include <KConfig>
#include <KDBusService>
#include <KLocalizedString>
#include <KWindowSystem>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QWindow>
#if WITH_QTWEBVIEW
#include <qtwebviewfunctions.h>
#endif

#include <KirigamiAppDefaults>
#include <QProcessEnvironment>

std::unique_ptr<QCommandLineParser> createParser()
{
    // clang-format off
    auto parser = std::make_unique<QCommandLineParser>();
    parser->addOption(QCommandLineOption(QStringLiteral("application"), i18n("Directly open the specified application by its appstream:// URI."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("mime"), i18n("Open with a search for programs that can deal with the given mimetype."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("category"), i18n("Display a list of entries with a category."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("mode"), i18n("Open Discover in a said mode. Modes correspond to the toolbar buttons."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("listmodes"), i18n("List all the available modes.")));
    parser->addOption(QCommandLineOption(QStringLiteral("local-filename"), i18n("Local package file to install"), QStringLiteral("package")));
    parser->addOption(QCommandLineOption(QStringLiteral("listbackends"), i18n("List all the available backends.")));
    parser->addOption(QCommandLineOption(QStringLiteral("search"), i18n("Search string."), QStringLiteral("text")));
    parser->addOption(QCommandLineOption(QStringLiteral("feedback"), i18n("Lists the available options for user feedback")));
    parser->addOption(QCommandLineOption(QStringLiteral("headless-update"), i18n("Starts an update automatically, headless")));
    parser->addOption(QCommandLineOption(QStringLiteral("test"), QStringLiteral("Test file"), QStringLiteral("file.qml")));
    parser->addPositionalArgument(QStringLiteral("urls"), i18n("Supports appstream: url scheme"));
    // clang-format on
    DiscoverBackendsFactory::setupCommandLine(parser.get());
    KAboutData::applicationData().setupCommandLine(parser.get());
    return parser;
}

void processArgs(QCommandLineParser *parser, DiscoverObject *discoverObject)
{
    if (parser->isSet(QStringLiteral("application"))) {
        discoverObject->openApplication(QUrl(parser->value(QStringLiteral("application"))));
    } else if (parser->isSet(QStringLiteral("mime"))) {
        discoverObject->openMimeType(parser->value(QStringLiteral("mime")));
    } else if (parser->isSet(QStringLiteral("category"))) {
        discoverObject->openCategory(parser->value(QStringLiteral("category")));
    } else if (parser->isSet(QStringLiteral("mode"))) {
        discoverObject->openMode(parser->value(QStringLiteral("mode")));
    }

    if (parser->isSet(QStringLiteral("search"))) {
        Q_EMIT discoverObject->openSearch(parser->value(QStringLiteral("search")));
    }

    if (parser->isSet(QStringLiteral("local-filename"))) {
        discoverObject->openLocalPackage(QUrl::fromUserInput(parser->value(QStringLiteral("local-filename")), QDir::currentPath(), QUrl::AssumeLocalFile));
    }

    if (parser->isSet(QStringLiteral("headless-update"))) {
        discoverObject->startHeadlessUpdate();
    }

    const auto positionalArguments = parser->positionalArguments();
    for (const QString &arg : positionalArguments) {
        const QUrl url = QUrl::fromUserInput(arg, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (url.isLocalFile()) {
            discoverObject->openLocalPackage(url);
        } else if (url.scheme() == QLatin1String("apt")) {
            Q_EMIT discoverObject->openSearch(url.host());
        } else {
            discoverObject->openApplication(url);
        }
    }

    if (auto window = discoverObject->mainWindow()) {
        if (window->property("pageStack").value<QObject *>()->property("depth").toInt() == 0) {
            discoverObject->openMode(QStringLiteral("Browsing"));
        }
    }
}

static void raiseWindow(QWindow *window)
{
    KWindowSystem::updateStartupId(window);
    KWindowSystem::activateWindow(window);
}

int main(int argc, char **argv)
{
    // needs to be set before we create the QGuiApplication
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager, true);
#if WITH_QTWEBVIEW
    { // as required by a QtWebEngine warning
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    }
    QtWebView::initialize();
#endif

    QApplication app(argc, argv);
    KirigamiAppDefaults::apply(&app);

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("plasmadiscover")));
    app.setQuitLockEnabled(false);
    KLocalizedString::setApplicationDomain("plasma-discover");
    KAboutData about(QStringLiteral("discover"),
                     i18n("Discover"),
                     version,
                     i18n("An application explorer"),
                     KAboutLicense::GPL,
                     i18n("© 2010-2026 Plasma Development Team"));
    about.addAuthor(i18n("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@kde.org"), QStringLiteral("https://proli.net"), QStringLiteral("apol"));
    about.addAuthor(i18n("Nate Graham"),
                    i18n("Quality Assurance, Design and Usability"),
                    QStringLiteral("nate@kde.org"),
                    QStringLiteral("https://pointieststick.com/"),
                    QStringLiteral("ngraham"));
    about.addAuthor(i18n("Dan Leinir Turthra Jensen"),
                    i18n("KNewStuff"),
                    QStringLiteral("admin@leinir.dk"),
                    QStringLiteral("https://leinir.dk/"),
                    QStringLiteral("leinir"));
    about.setProductName("discover/discover");
    about.setProgramLogo(app.windowIcon());

    about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KAboutData::setApplicationData(about);

    {
        // clean up old window geometry data
        KConfig config;
        config.deleteGroup(QStringLiteral("Window"));
    }

    DiscoverObject *discoverObject = nullptr;
    {
        std::unique_ptr<QCommandLineParser> parser(createParser());
        parser->process(app);
        about.processCommandLine(parser.get());
        DiscoverBackendsFactory::processCommandLine(parser.get(), parser->isSet(QStringLiteral("test")));
        const bool feedback = parser->isSet(QStringLiteral("feedback"));
        const bool headlessUpdate = parser->isSet(QStringLiteral("headless-update"));

        if (parser->isSet(QStringLiteral("listbackends"))) {
            QTextStream(stdout) << i18n("Available backends:\n");
            DiscoverBackendsFactory f;
            const auto backendNames = f.allBackendNames(false, true);
            for (const QString &name : backendNames)
                QTextStream(stdout) << " * " << name << '\n';
            return 0;
        }

        if (parser->isSet(QStringLiteral("test"))) {
            QStandardPaths::setTestModeEnabled(true);
        }

        KDBusService *service = !feedback ? new KDBusService(KDBusService::Unique, &app) : nullptr;

        {
            auto options = parser->optionNames();
            options.removeAll(QStringLiteral("backends"));
            options.removeAll(QStringLiteral("test"));
            QVariantMap initialProperties;
            if (!options.isEmpty() || !parser->positionalArguments().isEmpty())
                initialProperties = {{QStringLiteral("currentTopLevel"), QStringLiteral(DISCOVER_BASE_URL "/LoadingPage.qml")}};
            if (feedback || headlessUpdate) {
                initialProperties.insert(QStringLiteral("visible"), false);
            }
            discoverObject = new DiscoverObject(initialProperties);
        }
        if (feedback) {
            QTextStream(stdout) << discoverObject->describeSources() << '\n';
            delete discoverObject;
            return 0;
        } else {
            QObject::connect(service,
                             &KDBusService::activateRequested,
                             discoverObject,
                             [discoverObject](const QStringList &arguments, const QString & /*workingDirectory*/) {
                                 discoverObject->restore();
                                 if (auto window = discoverObject->mainWindow()) {
                                     raiseWindow(window);
                                     if (arguments.isEmpty()) {
                                         return;
                                     }
                                     std::unique_ptr<QCommandLineParser> parser(createParser());
                                     parser->parse(arguments);
                                     processArgs(parser.get(), discoverObject);
                                 }
                             });
        }

        QObject::connect(&app, &QCoreApplication::aboutToQuit, discoverObject, &DiscoverObject::deleteLater);

        processArgs(parser.get(), discoverObject);

        if (parser->isSet(QStringLiteral("listmodes"))) {
            QTextStream(stdout) << i18n("Available modes:\n");
            const auto modes = discoverObject->modes();
            for (const QString &mode : modes)
                QTextStream(stdout) << " * " << mode << '\n';
            delete discoverObject;
            return 0;
        }

        if (parser->isSet(QStringLiteral("test"))) {
            const QUrl testFile = QUrl::fromUserInput(parser->value(QStringLiteral("test")), {}, QUrl::AssumeLocalFile);
            Q_ASSERT(!testFile.isEmpty() && testFile.isLocalFile());

            discoverObject->loadTest(testFile);
        }
    }

    return app.exec();
}
