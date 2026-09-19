/*
 *   SPDX-FileCopyrightText: 2026 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamPreviewBackendUpdater.h"
#include <QDateTime>

AppStreamPreviewBackendUpdater::AppStreamPreviewBackendUpdater(AbstractResourcesBackend *parent)
    : AbstractBackendUpdater(parent)
    , m_backend(parent)
{
}

void AppStreamPreviewBackendUpdater::prepare()
{
}

bool AppStreamPreviewBackendUpdater::hasUpdates() const
{
    return false;
}

qreal AppStreamPreviewBackendUpdater::progress() const
{
    return 100;
}

void AppStreamPreviewBackendUpdater::removeResources(const QList<AbstractResource *> &apps)
{
    Q_UNUSED(apps)
}

void AppStreamPreviewBackendUpdater::addResources(const QList<AbstractResource *> &apps)
{
    Q_UNUSED(apps)
}

QList<AbstractResource *> AppStreamPreviewBackendUpdater::toUpdate() const
{
    return QList<AbstractResource *>();
}

QDateTime AppStreamPreviewBackendUpdater::lastUpdate() const
{
    return QDateTime();
}

bool AppStreamPreviewBackendUpdater::isCancelable() const
{
    return false;
}

bool AppStreamPreviewBackendUpdater::isProgressing() const
{
    return false;
}

bool AppStreamPreviewBackendUpdater::isMarked(AbstractResource *res) const
{
    Q_UNUSED(res)

    return false;
}

double AppStreamPreviewBackendUpdater::updateSize() const
{
    return 0;
}

quint64 AppStreamPreviewBackendUpdater::downloadSpeed() const
{
    return 0;
}

bool AppStreamPreviewBackendUpdater::isFetchingUpdates() const
{
    return false;
}

void AppStreamPreviewBackendUpdater::start()
{
}