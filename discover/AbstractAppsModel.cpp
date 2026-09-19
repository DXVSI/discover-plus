/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractAppsModel.h"

#include "discover_debug.h"
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KSharedConfig>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QtGlobal>

#include <libdiscover_debug.h>
#include <resources/ResourcesModel.h>
#include <utils.h>

class BestInResultsStream : public QObject
{
    Q_OBJECT
public:
    BestInResultsStream(const QSet<ResultsStream *> &streams)
        : QObject()
    {
        connect(this, &BestInResultsStream::finished, this, &QObject::deleteLater);
        Q_ASSERT(!streams.contains(nullptr));
        if (streams.isEmpty()) {
            QTimer::singleShot(0, this, &BestInResultsStream::clear);
        }

        for (auto stream : streams) {
            m_streams.insert(stream);
            connect(stream, &ResultsStream::resourcesFound, this, [this](const QVector<StreamResult> &resources) {
                m_resources.append(resources.constFirst());
            });
            connect(stream, &QObject::destroyed, this, &BestInResultsStream::streamDestruction);
        }
    }

    void streamDestruction(QObject *obj)
    {
        m_streams.remove(obj);
        clear();
    }

    void clear()
    {
        if (m_streams.isEmpty()) {
            Q_EMIT finished(m_resources);
        }
    }

Q_SIGNALS:
    void finished(QVector<StreamResult> resources);

private:
    QVector<StreamResult> m_resources;
    QSet<QObject *> m_streams;
};

AbstractAppsModel::AbstractAppsModel()
{
    connect(ResourcesModel::global(), &ResourcesModel::currentApplicationBackendChanged, this, &AbstractAppsModel::refreshCurrentApplicationBackend);
    refreshCurrentApplicationBackend();
}

void AbstractAppsModel::refreshCurrentApplicationBackend()
{
    auto backend = ResourcesModel::global()->currentApplicationBackend();
    if (m_backend == backend)
        return;

    if (m_backend) {
        disconnect(m_backend, &AbstractResourcesBackend::contentsChanged, this, &AbstractAppsModel::refresh);
        disconnect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, &AbstractAppsModel::removeResource);
    }

    m_backend = backend;

    if (backend) {
        connect(backend, &AbstractResourcesBackend::contentsChanged, this, &AbstractAppsModel::refresh);
        connect(backend, &AbstractResourcesBackend::resourceRemoved, this, &AbstractAppsModel::removeResource);
    }

    Q_EMIT currentApplicationBackendChanged(m_backend);
}

void AbstractAppsModel::setUris(const QVector<QUrl> &uris)
{
    if (!m_backend)
        return;

    if (m_uris == uris) {
        return;
    }
    m_uris = uris;

    QSet<ResultsStream *> streams;
    for (const auto &uri : uris) {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = uri;
        streams << m_backend->search(filter);
    }
    if (!streams.isEmpty()) {
        auto stream = new BestInResultsStream(streams);
        acquireFetching(true);
        connect(stream, &BestInResultsStream::finished, this, &AbstractAppsModel::setResources);
    }
}

static void filterDupes(QVector<StreamResult> &resources)
{
    QSet<QString> found;
    for (auto it = resources.begin(); it != resources.end();) {
        auto id = it->resource->appstreamId();
        if (found.contains(id)) {
            it = resources.erase(it);
        } else {
            found.insert(id);
            ++it;
        }
    }
}

void AbstractAppsModel::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching == 0) || (f && m_isFetching == 1)) {
        Q_EMIT isFetchingChanged();
    }
    Q_ASSERT(m_isFetching >= 0);
}

void AbstractAppsModel::setResources(const QVector<StreamResult> &_resources)
{
    auto resources = _resources;
    filterDupes(resources);

    if (m_resources != resources) {
        // TODO: sort like in the json files
        beginResetModel();
        m_resources = resources;
        endResetModel();
        Q_EMIT appsCountChanged();
    }

    acquireFetching(false);
}

void AbstractAppsModel::removeResource(AbstractResource *resource)
{
    int index = m_resources.indexOf(resource);
    if (index < 0)
        return;

    beginRemoveRows({}, index, index);
    m_resources.removeAt(index);
    endRemoveRows();
}

QVariant AbstractAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::UserRole)
        return {};

    auto res = m_resources.value(index.row()).resource;
    if (!res)
        return {};

    return QVariant::fromValue<QObject *>(res);
}

int AbstractAppsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_resources.count();
}

QHash<int, QByteArray> AbstractAppsModel::roleNames() const
{
    return {{Qt::UserRole, "application"}};
}

#include "AbstractAppsModel.moc"
#include "moc_AbstractAppsModel.cpp"
