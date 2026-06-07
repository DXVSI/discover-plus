/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractSourcesBackend.h"
#include "AbstractResourcesBackend.h"
#include <QAbstractItemModel>

AbstractSourcesBackend::AbstractSourcesBackend(AbstractResourcesBackend *parent)
    : QObject(parent)
{
}

AbstractSourcesBackend::~AbstractSourcesBackend() = default;

AbstractResourcesBackend *AbstractSourcesBackend::resourcesBackend() const
{
    return dynamic_cast<AbstractResourcesBackend *>(parent());
}

bool AbstractSourcesBackend::moveSource(const QString &sourceId, int delta)
{
    Q_UNUSED(sourceId)
    Q_UNUSED(delta)
    return false;
}

QString AbstractSourcesBackend::firstDisambiguatedSourceId() const
{
    auto m = const_cast<AbstractSourcesBackend *>(this)->sources();
    return m->index(0, 0).data(AbstractSourcesBackend::DisambiguatedIdRole).toString();
}

QString AbstractSourcesBackend::lastDisambiguatedSourceId() const
{
    auto m = const_cast<AbstractSourcesBackend *>(this)->sources();
    return m->index(m->rowCount() - 1, 0).data(AbstractSourcesBackend::DisambiguatedIdRole).toString();
}

#include "moc_AbstractSourcesBackend.cpp"
