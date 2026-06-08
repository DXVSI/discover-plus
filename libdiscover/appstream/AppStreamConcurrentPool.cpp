/*
 *   SPDX-FileCopyrightText: 2024 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamConcurrentPool.h"

#include <QDebug>
#include <QtConcurrentMap>
#include <QtConcurrentRun>

using namespace AppStream;

void ConcurrentPool::reset(AppStream::Pool *pool, QThreadPool *threadPool)
{
    m_pool.reset(pool);
    connect(pool, &Pool::loadFinished, this, &ConcurrentPool::loadFinished);

    m_threadPool = threadPool;
}

void ConcurrentPool::loadAsync()
{
    QMutexLocker lock(&m_mutex);
    return m_pool->loadAsync();
}

QString ConcurrentPool::lastError()
{
    QMutexLocker lock(&m_mutex);
    return m_pool->lastError();
}

QFuture<ComponentBox> ConcurrentPool::search(const QString &term)
{
    return QtConcurrent::run(m_threadPool.get(), [this, term] {
        QMutexLocker lock(&m_mutex);
        return m_pool->search(term);
    });
}

QFuture<ComponentBox> ConcurrentPool::components()
{
    return QtConcurrent::run(m_threadPool.get(), [this] {
        QMutexLocker lock(&m_mutex);
        return m_pool->components();
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsById(const QString &cid)
{
    return QtConcurrent::run(m_threadPool.get(), [this, cid] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsById(cid);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByProvided(Provided::Kind kind, const QString &item)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind, item] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByProvided(kind, item);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByKind(Component::Kind kind)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByKind(kind);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByCategories(const QStringList &categories)
{
    return QtConcurrent::run(m_threadPool.get(), [this, categories] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByCategories(categories);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByLaunchable(Launchable::Kind kind, const QString &value)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind, value] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByLaunchable(kind, value);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByExtends(const QString &extendedId)
{
    return QtConcurrent::run(m_threadPool.get(), [this, extendedId] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByExtends(extendedId);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByBundleId(Bundle::Kind kind, const QString &bundleId, bool matchPrefix)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind, bundleId, matchPrefix] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByBundleId(kind, bundleId, matchPrefix);
    });
}

QFuture<QMap<ConcurrentPool *, QList<Component>>>
ConcurrentPool::componentsByNames(QThreadPool *threadPool, const QList<ConcurrentPool *> &pools, const QStringList &names)
{
    return QtConcurrent::mappedReduced(
        threadPool,
        pools,
        [names](ConcurrentPool *pool) -> std::pair<ConcurrentPool *, QList<Component>> {
            if (!pool) {
                return {};
            }
            QMutexLocker lock(&pool->m_mutex);
            QList<Component> ret;
            for (const QString &name : names) {
                ComponentBox components = pool->m_pool->componentsById(name);
                if (!components.isEmpty()) {
                    ret += components.toList();
                    break;
                }
                ret += pool->m_pool->componentsByProvided(AppStream::Provided::KindId, name).toList();
            }
            return {pool, ret};
        },
        [](QMap<ConcurrentPool *, QList<Component>> &acc, const std::pair<ConcurrentPool *, QList<Component>> &next) {
            acc.insert(next.first, next.second);
        });
}

#include "moc_AppStreamConcurrentPool.cpp"
