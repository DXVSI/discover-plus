/*
 *   SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "../DiscoverVersion.h"
#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QMenu>

#include "NotifierItem.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    KCrash::setFlags(KCrash::AutoRestart);

    std::chrono::seconds checkDelay = 0s;
    bool hide = false;
    KDBusService::StartupOptions startup = {};
    {
        KAboutData about(QStringLiteral("discover.notifier"),
                         i18n("Discover Notifier"),
                         version,
                         i18n("System update status notifier"),
                         KAboutLicense::GPL,
                         i18n("© 2010-2025 Plasma Development Team"));
        about.addAuthor(QStringLiteral("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
        about.setProductName("discover/discover");
        about.setProgramLogo(app.windowIcon());
        about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

        KAboutData::setApplicationData(about);

        KCrash::initialize();

        QCommandLineParser parser;
        QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
        parser.addOption(replaceOption);
        QCommandLineOption checkDelayOption({QStringLiteral("check-delay")}, u"Delay checking for updates by N seconds"_s, u"seconds"_s, QStringLiteral("0"));
        parser.addOption(checkDelayOption);
        QCommandLineOption hideOption({QStringLiteral("hide")}, i18n("Do not show the notifier"), i18n("hidden"), QStringLiteral("false"));
        parser.addOption(hideOption);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);

        if (parser.isSet(replaceOption)) {
            startup |= KDBusService::Replace;
        }

        const auto config = KSharedConfig::openConfig();
        KConfigGroup group(config, u"Behavior"_s);

        if (parser.isSet(hideOption)) {
            hide = parser.value(hideOption) == QLatin1String("true");
            group.writeEntry<bool>("Hide", hide);
            config->sync();
        } else {
            hide = group.readEntry<bool>("Hide", false);
        }

        if (parser.isSet(checkDelayOption)) {
            checkDelay = std::chrono::seconds(parser.value(checkDelayOption).toInt());
        }
    }

    KDBusService service(KDBusService::Unique | startup);

    // Otherwise the QEventLoopLocker in KStatusNotifierItem quits the
    // application when the status notifier is destroyed
    app.setQuitLockEnabled(false);

    NotifierItem notifier(checkDelay);
    notifier.setStatusNotifierEnabled(!hide);

    return app.exec();
}
