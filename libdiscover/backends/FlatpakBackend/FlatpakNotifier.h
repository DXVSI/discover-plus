/*
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <BackendNotifierModule.h>
#include <functional>

#include "flatpak-helper.h"

class FlatpakNotifier : public BackendNotifierModule
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DISCOVER_NOTIFIER_IID)
    Q_INTERFACES(BackendNotifierModule)
public:
    explicit FlatpakNotifier(QObject *parent = nullptr);
    ~FlatpakNotifier() override;

    bool hasUpdates() override;
    bool hasSecurityUpdates() override
    {
        return false;
    }
    void recheckSystemUpdateNeeded() override;
    bool needsReboot() const override
    {
        return false;
    }

    struct Installation {
        explicit Installation(FlatpakNotifier *notifier, FlatpakInstallation *installation);
        ~Installation();
        Q_DISABLE_COPY(Installation);

        bool ensureInitialized(GCancellable *);

        FlatpakNotifier *const m_notifier;
        bool m_hasUpdates = false;
        GFileMonitor *m_monitor = nullptr;
        FlatpakInstallation *const m_installation;
    };

    void onFetchUpdatesFinished(const std::shared_ptr<Installation> &flatpakInstallation, bool hasUpdates);
    void loadRemoteUpdates(const std::shared_ptr<Installation> &installation);
    void setupFlatpakInstallations();
    QList<std::shared_ptr<Installation>> m_installations;
    GCancellable *const m_cancellable;
    bool m_lastHasUpdates = false;
};
