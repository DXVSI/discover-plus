/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ResourcesModel.h"

#include "AbstractResource.h"
#include "Category/CategoryModel.h"
#include "Transaction/TransactionModel.h"
#include "libdiscover_debug.h"
#include "resources/AbstractBackendUpdater.h"
#include "resources/AbstractResourcesBackend.h"
#include "utils.h"
#include <DiscoverBackendsFactory.h>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KOSRelease>
#include <KSharedConfig>
#include <QCoreApplication>
#include <QIcon>
#include <QMetaProperty>
#include <QThread>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Rating.h>
#include <Transaction/Transaction.h>
#include <functional>
#include <resources/DiscoverAction.h>

using namespace Qt::StringLiterals;

ResourcesModel *ResourcesModel::s_self = nullptr;
bool ResourcesModel::s_quitting = false;

ResourcesModel *ResourcesModel::global()
{
    if (!s_self) {
        s_self = new ResourcesModel;
        Q_ASSERT(!s_quitting);
        if (!s_quitting) {
            s_self->init(true);
        } else {
            // Only should happen if the singleton is instantiated after aboutToQuit
            QTimer::singleShot(0, s_self, &ResourcesModel::destroyObject);
        }
    }
    return s_self;
}

void ResourcesModel::destroyObject()
{
    s_quitting = true;
    deleteLater();
}

ResourcesModel::ResourcesModel(QObject *parent)
    : QObject(parent)
    , m_currentApplicationBackend(nullptr)
    , m_updatesCount(
          0,
          [this] {
              int ret = 0;
              for (AbstractResourcesBackend *backend : std::as_const(m_backends)) {
                  ret += backend->updatesCount();
              }
              return ret;
          },
          [this](int count) {
              Q_EMIT updatesCountChanged(count);
          })
    , m_fetchingUpdatesProgress(
          0,
          [this] {
              if (m_backends.isEmpty()) {
                  return 0;
              }

              int sum = 0;
              int weights = 0;
              for (auto backend : std::as_const(m_backends)) {
                  sum += backend->fetchingUpdatesProgress() * backend->fetchingUpdatesProgressWeight();
                  weights += backend->fetchingUpdatesProgressWeight();
              }
              return sum / weights;
          },
          [this](int progress) {
              Q_EMIT fetchingUpdatesProgressChanged(progress);
          })
{
    connect(this, &ResourcesModel::backendsChanged, this, &ResourcesModel::initApplicationsBackend);
}

void ResourcesModel::init(bool load)
{
    Q_ASSERT(QCoreApplication::instance()->thread() == QThread::currentThread());

    if (load) {
        registerAllBackends();
    }

    m_updateAction = new DiscoverAction(this);
    m_updateAction->setIconName(QStringLiteral("system-software-update"));
    m_updateAction->setText(i18n("Refresh"));
    connect(this, &ResourcesModel::fetchingUpdatesProgressChanged, m_updateAction, [this](int fetchingProgress) {
        m_updateAction->setEnabled(fetchingProgress == 100);
    });
    connect(m_updateAction, &DiscoverAction::triggered, this, &ResourcesModel::checkForUpdates);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
}

ResourcesModel::ResourcesModel(const QString &backendName, QObject *parent)
    : ResourcesModel(parent)
{
    s_self = this;
    registerBackendByName(backendName);
    init(false);
}

ResourcesModel::~ResourcesModel()
{
    s_self = nullptr;
    qDeleteAll(m_backends);
}

void ResourcesModel::addResourcesBackends(const QList<AbstractResourcesBackend *> &backends)
{
    auto added = false;

    for (const auto backend : backends) {
        added |= addResourcesBackend(backend);
    }

    if (added) {
        Q_EMIT backendsChanged();
    }
}

bool ResourcesModel::addResourcesBackend(AbstractResourcesBackend *backend)
{
    Q_ASSERT(!m_backends.contains(backend));
    if (!backend->isValid()) {
        qCWarning(LIBDISCOVER_LOG) << "Discarding invalid backend" << backend->name();
        CategoryModel::global()->blacklistPlugin(backend->name());
        backend->deleteLater();
        return false;
    }

    m_backends += backend;
    m_updatesCount.reevaluate();

    connect(backend, &AbstractResourcesBackend::contentsChanged, this, &ResourcesModel::callerContentsChanged);
    connect(backend, &AbstractResourcesBackend::allDataChanged, this, &ResourcesModel::updateCaller);
    connect(backend, &AbstractResourcesBackend::resourcesChanged, this, &ResourcesModel::resourceDataChanged);
    connect(backend, &AbstractResourcesBackend::updatesCountChanged, this, [this] {
        m_updatesCount.reevaluate();
    });
    connect(backend, &AbstractResourcesBackend::fetchingUpdatesProgressChanged, this, [this] {
        m_fetchingUpdatesProgress.reevaluate();
    });
    connect(backend, &AbstractResourcesBackend::resourceRemoved, this, &ResourcesModel::resourceRemoved);
    connect(backend, &AbstractResourcesBackend::passiveMessage, this, &ResourcesModel::passiveMessage);
    connect(backend, &AbstractResourcesBackend::inlineMessageChanged, this, &ResourcesModel::setInlineMessage);
    if (auto reviewsBackend = backend->reviewsBackend()) {
        connect(reviewsBackend, &AbstractReviewsBackend::error, this, &ResourcesModel::passiveMessage, Qt::UniqueConnection);
    }
    return true;
}

void ResourcesModel::callerContentsChanged()
{
    AbstractResourcesBackend *backend = qobject_cast<AbstractResourcesBackend *>(sender());

    if (!backend->isValid()) {
        qCWarning(LIBDISCOVER_LOG) << "Discarding invalid backend" << backend->name();
        int idx = m_backends.indexOf(backend);
        Q_ASSERT(idx >= 0);
        m_backends.removeAt(idx);
        Q_EMIT backendsChanged();
        CategoryModel::global()->blacklistPlugin(backend->name());
        backend->deleteLater();
        return;
    }
}

void ResourcesModel::updateCaller(const QVector<QByteArray> &properties)
{
    AbstractResourcesBackend *backend = qobject_cast<AbstractResourcesBackend *>(sender());

    Q_EMIT backendDataChanged(backend, properties);
}

QVector<AbstractResourcesBackend *> ResourcesModel::backends() const
{
    return m_backends;
}

bool ResourcesModel::hasSecurityUpdates() const
{
    return std::any_of(m_backends.constBegin(), m_backends.constEnd(), [](const AbstractResourcesBackend *backend) {
        return backend->hasSecurityUpdates();
    });
}

void ResourcesModel::installApplication(AbstractResource *app)
{
    TransactionModel::global()->addTransaction(app->backend()->installApplication(app));
}

void ResourcesModel::installApplication(AbstractResource *app, const AddonList &addons)
{
    TransactionModel::global()->addTransaction(app->backend()->installApplication(app, addons));
}

void ResourcesModel::removeApplication(AbstractResource *app)
{
    TransactionModel::global()->addTransaction(app->backend()->removeApplication(app));
}

void ResourcesModel::registerAllBackends()
{
    DiscoverBackendsFactory f;
    addResourcesBackends(f.allBackends());
    m_isInitializing = false;
}

void ResourcesModel::registerBackendByName(const QString &name)
{
    DiscoverBackendsFactory f;
    const auto backends = f.backend(name);
    addResourcesBackends(backends);
}

bool ResourcesModel::isInitializing() const
{
    return m_isInitializing;
}

bool ResourcesModel::isExtended(const QString &id)
{
    return std::any_of(m_backends.constBegin(), m_backends.constEnd(), [&](AbstractResourcesBackend *backend) {
        return backend->extends(id);
    });
}

AggregatedResultsStream::AggregatedResultsStream(const QSet<ResultsStream *> &streams)
    : ResultsStream(QStringLiteral("AggregatedResultsStream"))
{
    Q_ASSERT(!streams.contains(nullptr));
    if (streams.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "AggregatedResultsStream: No streams to aggregate!";
        QTimer::singleShot(0, this, &AggregatedResultsStream::clear);
    }

    for (const auto &stream : streams) {
        connect(stream, &ResultsStream::resourcesFound, this, &AggregatedResultsStream::addResults);
        connect(stream, &QObject::destroyed, this, &AggregatedResultsStream::streamDestruction);
        connect(this, &ResultsStream::fetchMore, stream, &ResultsStream::fetchMore);
        m_streams << stream;
    }

    m_delayedEmission.setInterval(0);
    connect(&m_delayedEmission, &QTimer::timeout, this, &AggregatedResultsStream::emitResults);
}

AggregatedResultsStream::~AggregatedResultsStream() = default;

void AggregatedResultsStream::addResults(const QVector<StreamResult> &streams)
{
    for (const auto &stream : streams) {
        connect(stream.resource, &QObject::destroyed, this, &AggregatedResultsStream::resourceDestruction);
    }

    m_results += streams;

    m_delayedEmission.start();
}

void AggregatedResultsStream::emitResults()
{
    if (!m_results.isEmpty()) {
        Q_EMIT resourcesFound(m_results);
        m_results.clear();
    }
    m_delayedEmission.setInterval(m_delayedEmission.interval() + 100);
    m_delayedEmission.stop();
}

void AggregatedResultsStream::resourceDestruction(QObject *obj)
{
    for (auto it = m_results.begin(); it != m_results.end(); ++it) {
        if (obj == it->resource) {
            it = m_results.erase(it);
        } else {
            ++it;
        }
    }
}

void AggregatedResultsStream::streamDestruction(QObject *obj)
{
    m_streams.remove(obj);
    clear();
}

void AggregatedResultsStream::clear()
{
    if (m_streams.isEmpty()) {
        emitResults();
        Q_EMIT finished();
        deleteLater();
    }
}

AggregatedResultsStream *ResourcesModel::search(const AbstractResourcesBackend::Filters &search)
{
    if (search.isEmpty()) {
        return new AggregatedResultsStream({new ResultsStream(QStringLiteral("emptysearch"), {})});
    }

    auto streams = kTransform<QSet<ResultsStream *>>(m_backends, [search](AbstractResourcesBackend *backend) {
        return backend->search(search);
    });
    return new AggregatedResultsStream(streams);
}

void ResourcesModel::checkForUpdates()
{
    for (const auto backend : std::as_const(m_backends)) {
        backend->checkForUpdates();
    }
}

AbstractResourcesBackend *ResourcesModel::currentApplicationBackend() const
{
    return m_currentApplicationBackend;
}

void ResourcesModel::setCurrentApplicationBackend(AbstractResourcesBackend *backend, bool write)
{
    if (backend != m_currentApplicationBackend) {
        if (write) {
            KConfigGroup settings(KSharedConfig::openConfig(), u"ResourcesModel"_s);
            if (backend) {
                settings.writeEntry("currentApplicationBackend", backend->name());
            } else {
                settings.deleteEntry("currentApplicationBackend");
            }
        }

        qCDebug(LIBDISCOVER_LOG) << "setting currentApplicationBackend to" << backend;
        m_currentApplicationBackend = backend;
        Q_EMIT currentApplicationBackendChanged(backend);
    }
}

void ResourcesModel::initApplicationsBackend()
{
    const auto name = applicationSourceName();

    auto idx = kIndexOf(m_backends, [name](AbstractResourcesBackend *backend) {
        return backend->hasApplications() && backend->name() == name;
    });
    if (idx < 0) {
        idx = kIndexOf(m_backends, [](AbstractResourcesBackend *backend) {
            return backend->hasApplications();
        });
        qCDebug(LIBDISCOVER_LOG) << "falling back applications backend to" << idx;
    }
    setCurrentApplicationBackend(m_backends.value(idx, nullptr), false);
}

QString ResourcesModel::applicationSourceName() const
{
    KConfigGroup settings(KSharedConfig::openConfig(), u"ResourcesModel"_s);
    return settings.readEntry<QString>("currentApplicationBackend", QStringLiteral("packagekit-backend"));
}

QString ResourcesModel::distroName() const
{
    return KOSRelease().name();
}

QUrl ResourcesModel::distroBugReportUrl()
{
    return QUrl(KOSRelease().bugReportUrl());
}

void ResourcesModel::setInlineMessage(const QSharedPointer<InlineMessage> &inlineMessage)
{
    if (inlineMessage == m_inlineMessage) {
        return;
    }

    m_inlineMessage = inlineMessage;
    Q_EMIT inlineMessageChanged(inlineMessage);
}

QString ResourcesModel::remainingDescription() const
{
    QStringList ret;
    for (auto backend : std::as_const(m_backends)) {
        if (backend->fetchingUpdatesProgress() < 100) {
            ret += i18nc("@info:status %1 is the name of a source of updates", "Refreshing %1", backend->displayName());
        }
    }
    if (ret.isEmpty()) {
        return {};
    }
    ret.removeDuplicates();
    return ret.join("\n"_L1);
}

#include "moc_ResourcesModel.cpp"
