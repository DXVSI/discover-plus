/***************************************************************************
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#include "libdiscover_backend_flatpak_debug.h"

#include <ApplicationAddonsModel.h>
#include <Category/CategoryModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/DiscoverAction.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/SourcesModel.h>

#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <flatpak.h>

// Should make sure it's available on all tested architectures
constexpr QLatin1StringView s_testId("com.chez.GrafX2");

class FlatpakTest : public QObject
{
    Q_OBJECT
public:
    AbstractResourcesBackend *backendByName(ResourcesModel *m, const QString &name)
    {
        const QVector<AbstractResourcesBackend *> backends = m->backends();
        for (AbstractResourcesBackend *backend : backends) {
            if (QLatin1String(backend->metaObject()->className()) == name) {
                return backend;
            }
        }
        return nullptr;
    }

    explicit FlatpakTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test")).removeRecursively();

        QStandardPaths::setTestModeEnabled(true);
        qputenv("FLATPAK_TEST_MODE", "ON");
        m_model = new ResourcesModel(QStringLiteral("flatpak-backend"), this);
        m_appBackend = backendByName(m_model, QStringLiteral("FlatpakBackend"));
    }

private Q_SLOTS:
    void initTestCase()
    {
    }

    void testAddSource()
    {
        auto res = getAllResources(m_appBackend);
        QCOMPARE(res.count(), 0);

        auto m = SourcesModel::global();
        auto bk = qobject_cast<AbstractSourcesBackend *>(m->index(0, 0).data(SourcesModel::SourcesBackend).value<QObject *>());

        QSignalSpy initializedSpy(m_appBackend, SIGNAL(initialized()));
        if (m->rowCount() == 1) {
            QSignalSpy spy(m, &SourcesModel::rowsInserted);
            qobject_cast<DiscoverAction *>(bk->actions().constFirst().value<QObject *>())->trigger();
            QVERIFY(spy.count() || spy.wait(200000));
        }
        QVERIFY(initializedSpy.count() || initializedSpy.wait(200000));
        auto resFlathub = getAllResources(m_appBackend);
        QVERIFY(resFlathub.count() > 0);
    }

    void testListOrigin()
    {
        AbstractResourcesBackend::Filters f;
        f.origin = QStringLiteral("flathub");
        auto resources = getResources(m_appBackend->search(f), true);
        QVERIFY(resources.count() > 0);
    }

    void testFlatpakrefWithoutExistingRemote()
    {
        QString flatpakRefPath;
        QVERIFY(createMockFlatpakRef(&flatpakRefPath));
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl::fromLocalFile(flatpakRefPath);
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);
        QCOMPARE(res.constFirst()->appstreamId(), QStringLiteral("org.kde.DiscoverMock"));
    }

    void testInstallApp()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("appstream://") + s_testId);
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        const auto ourResource = res.constFirst();
        QCOMPARE(ourResource->state(), AbstractResource::None);
        QCOMPARE(waitTransaction(m_appBackend->installApplication(ourResource)), Transaction::DoneStatus);
        QCOMPARE(ourResource->state(), AbstractResource::Installed);
        f.resourceUrl = QUrl(QStringLiteral("flatpak:app/") + s_testId + u'/' + QLatin1StringView(flatpak_get_default_arch()) + QLatin1StringView("/stable"));
        QCOMPARE(getResources(m_appBackend->search(f)).count(), 1);
        QCOMPARE(waitTransaction(m_appBackend->removeApplication(ourResource)), Transaction::DoneStatus);
        QCOMPARE(ourResource->state(), AbstractResource::None);
    }

    void testFlatpakref()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("https://dl.flathub.org/repo/appstream/") + s_testId + u".flatpakref");
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        f.resourceUrl = QUrl(QStringLiteral("appstream://") + s_testId);
        const auto res2 = getResources(m_appBackend->search(f));
        QCOMPARE(res2, res);
    }

    void testSearches()
    {
        // We test an item that provides another appstream id, this way we test both
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("flatpak:app/") + s_testId + u'/' + QLatin1StringView(flatpak_get_default_arch()) + QStringLiteral("/stable"));
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        f.resourceUrl = QUrl(QStringLiteral("appstream://grafx2.desktop")); // That's the alternatively provided id
        const auto res2 = getResources(m_appBackend->search(f));
        QCOMPARE(res2, res);

        f.resourceUrl = QUrl(QStringLiteral("appstream://") + s_testId);
        const auto res3 = getResources(m_appBackend->search(f));
        QCOMPARE(res3, res);
    }

    void testExtends()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("appstream://org.videolan.VLC"));
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);
        QVERIFY(m_appBackend->extends(res[0]->appstreamId()));
    }

    /*
        void testCancelInstallation()
        {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = QUrl(QStringLiteral("appstream://com.github.rssguard.desktop"));
            const auto res = getResources(m_appBackend->search(f));
            QCOMPARE(res.count(), 1);

            const auto resRssguard = res.constFirst();
            QCOMPARE(resRssguard->state(), AbstractResource::None);
            auto t = m_appBackend->installApplication(resRssguard);
            QSignalSpy spy(t, &Transaction::statusChanged);
            QVERIFY(spy.wait());
            QCOMPARE(t->status(), Transaction::CommittingStatus);
            t->cancel();
            QVERIFY(spy.wait());
            QCOMPARE(t->status(), Transaction::CancelledStatus);
        }*/

private:
    bool createMockFlatpakRef(QString *flatpakRefPath)
    {
        const QString root = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test/mock-remote");
        const QString repository = root + QLatin1String("/repository");
        const QString buildDirectory = root + QLatin1String("/build");
        const QString applicationId = QStringLiteral("org.kde.DiscoverMock");
        const QString arch = QString::fromUtf8(flatpak_get_default_arch());

        QDir(root).removeRecursively();
        const auto runFlatpak = [](const QStringList &arguments) {
            QProcess process;
            process.setProcessChannelMode(QProcess::MergedChannels);
            process.start(QStringLiteral("flatpak"), arguments);
            if (!process.waitForStarted() || !process.waitForFinished(60000) || process.exitCode() != 0) {
                qWarning() << "Failed to create Flatpak test repository:" << process.readAll();
                return false;
            }
            return true;
        };
        if (!QDir().mkpath(root) || !QDir().mkpath(buildDirectory + QLatin1String("/files"))
            || !QDir().mkpath(buildDirectory + QLatin1String("/usr/share/metainfo"))
            || !QDir().mkpath(buildDirectory + QLatin1String("/usr/share/applications"))) {
            return false;
        }

        const auto writeFile = [](const QString &path, const QString &contents) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(contents.toUtf8()) == contents.toUtf8().size();
        };
        const QString metadata = QStringLiteral(
                                     "[Application]\n"
                                     "name=%1\n"
                                     "runtime=org.kde.DiscoverMock.Runtime/%2/stable\n"
                                     "sdk=org.kde.DiscoverMock.Sdk/%2/stable\n"
                                     "command=mock\n")
                                     .arg(applicationId, arch);
        const QString metainfo = QStringLiteral(
                                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                     "<component type=\"desktop-application\">\n"
                                     "  <id>%1</id>\n"
                                     "  <name>Discover Mock Application</name>\n"
                                     "  <summary>Flatpak backend test fixture</summary>\n"
                                     "  <metadata_license>CC0-1.0</metadata_license>\n"
                                     "  <project_license>MIT</project_license>\n"
                                     "  <launchable type=\"desktop-id\">%1.desktop</launchable>\n"
                                     "  <bundle type=\"flatpak\">app/%1/%2/stable</bundle>\n"
                                     "</component>\n")
                                     .arg(applicationId, arch);
        const QString desktop = QStringLiteral("[Desktop Entry]\nType=Application\nName=Discover Mock Application\nExec=mock\n");
        if (!writeFile(buildDirectory + QLatin1String("/metadata"), metadata)
            || !writeFile(buildDirectory + QLatin1String("/usr/share/metainfo/") + applicationId + QLatin1String(".metainfo.xml"), metainfo)
            || !writeFile(buildDirectory + QLatin1String("/usr/share/applications/") + applicationId + QLatin1String(".desktop"), desktop)) {
            return false;
        }

        if (!runFlatpak({QStringLiteral("build-export"),
                         QStringLiteral("--runtime"),
                         QStringLiteral("--disable-sandbox"),
                         QStringLiteral("--arch=") + arch,
                         repository,
                         buildDirectory,
                         QStringLiteral("stable")})
            || !runFlatpak({QStringLiteral("build-update-repo"), repository})) {
            return false;
        }

        *flatpakRefPath = root + QLatin1String("/mock.flatpakref");
        QSettings flatpakRef(*flatpakRefPath, QSettings::NativeFormat);
        flatpakRef.setValue(QStringLiteral("Flatpak Ref/Name"), applicationId);
        flatpakRef.setValue(QStringLiteral("Flatpak Ref/Branch"), QStringLiteral("stable"));
        flatpakRef.setValue(QStringLiteral("Flatpak Ref/IsRuntime"), false);
        flatpakRef.setValue(QStringLiteral("Flatpak Ref/SuggestRemoteName"), QStringLiteral("discover-test"));
        flatpakRef.setValue(QStringLiteral("Flatpak Ref/Url"), QUrl::fromLocalFile(repository).toString());
        flatpakRef.sync();
        return flatpakRef.status() == QSettings::NoError;
    }

    Transaction::Status waitTransaction(Transaction *t)
    {
        int lastProgress = -1;
        connect(t, &Transaction::progressChanged, this, [t, &lastProgress] {
            Q_ASSERT(lastProgress <= t->progress());
            lastProgress = t->progress();
        });

        TransactionModel::global()->addTransaction(t);
        QSignalSpy spyInstalled(TransactionModel::global(), &TransactionModel::transactionRemoved);
        QSignalSpy destructionSpy(t, &QObject::destroyed);

        Transaction::Status ret = t->status();
        connect(TransactionModel::global(), &TransactionModel::transactionRemoved, t, [t, &ret](Transaction *trans) {
            if (trans == t) {
                ret = trans->status();
            }
        });
        connect(t, &Transaction::passiveMessage, t, [t](const QString &msg) {
            qCInfo(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "message" << msg;
        });
        connect(t, &Transaction::statusChanged, t, [t] {
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "status" << t->status();
        });
        connect(t, &Transaction::proceedRequest, t, [t](const QString &title, const QString &description) {
            qCInfo(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "proceed?" << t << title << description;
            t->proceed();
        });
        while (t && spyInstalled.count() == 0) {
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "waiting, currently" << ret << t->progress() << spyInstalled.count() << destructionSpy.count();
            spyInstalled.wait(1000);
        }
        Q_ASSERT(destructionSpy.count() || destructionSpy.wait());
        return ret;
    }

    QVector<AbstractResource *> getResources(ResultsStream *stream, bool canBeEmpty = true)
    {
        Q_ASSERT(stream);
        QSignalSpy spyResources(stream, &ResultsStream::destroyed);
        QVector<AbstractResource *> resources;
        connect(stream, &ResultsStream::resourcesFound, this, [&resources](const QVector<StreamResult> &res) {
            for (auto result : res) {
                resources += result.resource;
            }
        });
        Q_ASSERT(spyResources.wait(100000));
        Q_ASSERT(!resources.isEmpty() || canBeEmpty);
        return resources;
    }

    QVector<AbstractResource *> getAllResources(AbstractResourcesBackend *backend)
    {
        AbstractResourcesBackend::Filters f;
        if (CategoryModel::global()->rootCategories().isEmpty())
            CategoryModel::global()->populateCategories();
        f.category = CategoryModel::global()->rootCategories().constFirst();
        return getResources(backend->search(f), true);
    }

    ResourcesModel *m_model;
    AbstractResourcesBackend *m_appBackend;
};

QTEST_GUILESS_MAIN(FlatpakTest)

#include "FlatpakTest.moc"
