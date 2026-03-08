/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SnapBackend.h"
#include "SnapResource.h"
#include "SnapTransaction.h"
#include <Category/Category.h>
#include <Transaction/Transaction.h>
#include <appstream/AppStreamUtils.h>
#include <appstream/OdrsReviewsBackend.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>
#include <resources/StoredResultsStream.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QStandardItemModel>
#include <QThread>
#include <QTimer>
#include <QtConcurrentMap>
#include <QtConcurrentRun>

#include "utils.h"

DISCOVER_BACKEND_PLUGIN(SnapBackend)

class SnapSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit SnapSourcesBackend(AbstractResourcesBackend *parent)
        : AbstractSourcesBackend(parent)
        , m_model(new QStandardItemModel(this))
    {
        auto it = new QStandardItem(i18n("Snap"));
        it->setData(QStringLiteral("Snap"), IdRole);
        m_model->appendRow(it);
    }

    QAbstractItemModel *sources() override
    {
        return m_model;
    }
    bool addSource(const QString & /*id*/) override
    {
        return false;
    }
    bool removeSource(const QString & /*id*/) override
    {
        return false;
    }
    QString idDescription() override
    {
        return QStringLiteral("Snap");
    }
    QVariantList actions() const override
    {
        return {};
    }

    bool supportsAdding() const override
    {
        return false;
    }
    bool canMoveSources() const override
    {
        return false;
    }

private:
    QStandardItemModel *const m_model;
};

SnapBackend::SnapBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(OdrsReviewsBackend::global())
{
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &SnapBackend::updatesCountChanged);

    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, [this] {
        m_reviews->emitRatingFetched(this, kTransform<QList<AbstractResource *>>(m_resources.values(), [](AbstractResource *r) {
                                         return r;
                                     }));
    });

    // make sure we populate the installed resources first
    refreshStates();

    SourcesModel::global()->addSourcesBackend(new SnapSourcesBackend(this));

    m_threadPool.setMaxThreadCount(1);
}

SnapBackend::~SnapBackend()
{
    Q_EMIT shuttingDown();
    m_threadPool.waitForDone(1000);
    m_threadPool.clear();
}

int SnapBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

static ResultsStream *voidStream()
{
    return new ResultsStream(QStringLiteral("Snap-void"), {});
}

ResultsStream *SnapBackend::search(const AbstractResourcesBackend::Filters &filters)
{
    if (!filters.extends.isEmpty()) {
        return voidStream();
    } else if (!filters.resourceUrl.isEmpty()) {
        return findResourceByPackageName(filters.resourceUrl);
    } else if (filters.category && filters.category->type() == Category::Type::Addon) {
        return voidStream();
    } else if (filters.state >= AbstractResource::Installed || filters.origin == QLatin1String("Snap")) {
        std::function<bool(const QSharedPointer<QSnapdSnap> &)> f = [filters](const QSharedPointer<QSnapdSnap> &s) {
            return filters.search.isEmpty() || s->name().contains(filters.search, Qt::CaseInsensitive)
                || s->description().contains(filters.search, Qt::CaseInsensitive);
        };
        return populateWithFilter(m_client.getSnaps(), f);
    } else if (!filters.search.isEmpty()) {
        return populate(m_client.find(QSnapdClient::FindFlag::None, filters.search));
    }
    return voidStream();
}

ResultsStream *SnapBackend::findResourceByPackageName(const QUrl &search)
{
    Q_ASSERT(!search.host().isEmpty() || !AppStreamUtils::appstreamIds(search).isEmpty());
    return search.scheme() == QLatin1String("snap") ? populate(m_client.find(QSnapdClient::MatchName, search.host()))
        : search.scheme() == QLatin1String("appstream")
        ? populate(kTransform<QVector<QSnapdFindRequest *>>(AppStreamUtils::appstreamIds(search),
                                                            [this](const QString &id) {
                                                                return m_client.find(QSnapdClient::MatchCommonId, id);
                                                            }))
        : voidStream();
}

template<class T>
ResultsStream *SnapBackend::populate(T *job)
{
    return populate<T>(QVector<T *>{job});
}

template<class T>
ResultsStream *SnapBackend::populate(const QVector<T *> &jobs)
{
    std::function<bool(const QSharedPointer<QSnapdSnap> &)> acceptAll = [](const QSharedPointer<QSnapdSnap> &) {
        return true;
    };
    return populateJobsWithFilter(jobs, acceptAll);
}

template<class T>
ResultsStream *SnapBackend::populateWithFilter(T *job, std::function<bool(const QSharedPointer<QSnapdSnap> &s)> &filter)
{
    return populateJobsWithFilter<T>({job}, filter);
}

template<class T>
ResultsStream *SnapBackend::populateJobsWithFilter(const QVector<T *> &jobs, std::function<bool(const QSharedPointer<QSnapdSnap> &s)> &filter)
{
    auto stream = new ResultsStream(QStringLiteral("Snap-populate"));
    auto future = QtConcurrent::run(&m_threadPool, [this, jobs]() {
        for (auto job : jobs) {
            connect(this, &SnapBackend::shuttingDown, job, &T::cancel);
            job->runSync();
        }
    });

    auto watcher = new QFutureWatcher<void>(this);
    watcher->setFuture(future);
    connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
    connect(watcher, &QFutureWatcher<void>::finished, stream, [this, jobs, filter, stream] {
        QVector<StreamResult> ret;
        for (auto job : jobs) {
            job->deleteLater();
            if (job->error()) {
                qDebug() << "error:" << job->error() << job->errorString();
                continue;
            }

            for (int i = 0, c = job->snapCount(); i < c; ++i) {
                QSharedPointer<QSnapdSnap> snap(job->snap(i));

                if (!filter(snap))
                    continue;

                const auto snapname = snap->name();
                SnapResource *&res = m_resources[snapname];
                if (!res) {
                    res = new SnapResource(snap, AbstractResource::None, this);
                    Q_ASSERT(res->packageName() == snapname);
                } else {
                    res->setSnap(snap);
                }
                ret += res;
            }
        }

        if (!ret.isEmpty())
            Q_EMIT stream->resourcesFound(ret);
        stream->finish();
    });
    return stream;
}

AbstractBackendUpdater *SnapBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *SnapBackend::reviewsBackend() const
{
    return m_reviews.data();
}

Transaction *SnapBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_ASSERT(addons.isEmpty());
    return installApplication(app);
}

Transaction *SnapBackend::installApplication(AbstractResource *_app)
{
    auto app = qobject_cast<SnapResource *>(_app);
    return new SnapTransaction(&m_client, app, Transaction::InstallRole, AbstractResource::Installed);
}

Transaction *SnapBackend::removeApplication(AbstractResource *_app)
{
    auto app = qobject_cast<SnapResource *>(_app);
    return new SnapTransaction(&m_client, app, Transaction::RemoveRole, AbstractResource::None);
}

void SnapBackend::checkForUpdates()
{
    if (m_updatesFetcher) {
        qWarning() << "Already fetching updates";
        return;
    }

    m_updatesFetcher = new StoredResultsStream({populate(m_client.findRefreshable())});
    connect(m_updatesFetcher, &StoredResultsStream::finishedResources, this, [this](const QVector<StreamResult> &resources) {
        for (SnapResource *res : std::as_const(m_resources)) {
            bool contained = kContains(resources, [res](const StreamResult &in) {
                return in.resource == res;
            });
            if (contained) {
                res->setState(AbstractResource::Upgradeable);
                res->updateSizes();
            }
        }
    });
}

QString SnapBackend::displayName() const
{
    return QStringLiteral("Snap");
}

void SnapBackend::refreshStates()
{
    auto ret = new StoredResultsStream({populate(m_client.getSnaps())});
    connect(ret, &StoredResultsStream::finishedResources, this, [this](const QVector<StreamResult> &resources) {
        for (auto res : std::as_const(m_resources)) {
            bool contained = kContains(resources, [res](const StreamResult &in) {
                return in.resource == res;
            });
            res->updateSizes();
            if (contained)
                res->setState(AbstractResource::Installed);
            else
                res->setState(AbstractResource::None);
        }
        checkForUpdates();
    });
}

#include "SnapBackend.moc"
#include "moc_SnapBackend.cpp"
