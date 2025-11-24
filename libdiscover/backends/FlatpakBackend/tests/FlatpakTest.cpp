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
        // Remove the automatically added source again!
        auto sourcesModel = SourcesModel::global();
        QVERIFY(sourcesModel);
        auto sourcesBackend = sourcesModel->index(0, 0).data(SourcesModel::SourcesBackend).value<AbstractSourcesBackend *>();
        QVERIFY(sourcesBackend);
        auto sources = sourcesBackend->sources();
        QVERIFY(sources);
        for (auto i = sources->rowCount() - 1; i >= 0; --i) {
            QSignalSpy spy(sources, &QAbstractItemModel::rowsRemoved);
            const auto id = sources->index(i, 0).data(AbstractSourcesBackend::IdRole).toString();
            QVERIFY(sourcesBackend->removeSource(id));
            QVERIFY(spy.count() >= 1 || spy.wait());
        }
        // NOTE: there will be a stub entry in the model now, don't assume it to be rowCount==0!

        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("https://dl.flathub.org/repo/appstream/io.github.dosbox-staging.flatpakref"));
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        const auto ourResource = res.constFirst();
        QCOMPARE(ourResource->state(), AbstractResource::None);
        QCOMPARE(waitTransaction(m_appBackend->installApplication(ourResource)), Transaction::DoneStatus);
        QCOMPARE(ourResource->state(), AbstractResource::Installed);
        f.resourceUrl =
            QUrl(QStringLiteral("flatpak:app/io.github.dosbox-staging/") + QLatin1StringView(flatpak_get_default_arch()) + QStringLiteral("/stable"));
        QCOMPARE(getResources(m_appBackend->search(f)).count(), 1);
        QCOMPARE(waitTransaction(m_appBackend->removeApplication(ourResource)), Transaction::DoneStatus);
        QCOMPARE(ourResource->state(), AbstractResource::None);
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
