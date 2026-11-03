/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "flatpak-helper.h"
#include "glibholder.h"
#include <QThread>

class FlatpakRefreshAppstreamMetadataJob : public QThread
{
    Q_OBJECT
public:
    FlatpakRefreshAppstreamMetadataJob(FlatpakInstallation *installation, FlatpakRemote *remote);
    ~FlatpakRefreshAppstreamMetadataJob() override;

    void cancel();
    void run() override;

    bool isEstimating() const
    {
        return m_estimating != 0;
    }
    uint progress() const
    {
        return m_progress;
    }
    bool hasChanged() const
    {
        return m_hasChanged != 0;
    }

Q_SIGNALS:
    void progressChanged();
    void jobRefreshAppstreamMetadataFinished(GLibHolder<FlatpakInstallation> installation, GLibHolder<FlatpakRemote> remote, bool changed);

private:
    static void updateCallback(const char *status, guint progress, gboolean estimating, gpointer user_data);

    GCancellable *m_cancellable;
    GLibHolder<FlatpakInstallation> m_installation;
    GLibHolder<FlatpakRemote> m_remote;
    QAtomicInt m_progress = 0;
    QAtomicInt m_estimating = true;
    QAtomicInt m_hasChanged = false;
};
