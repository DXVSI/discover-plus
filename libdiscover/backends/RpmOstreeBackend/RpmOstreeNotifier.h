/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "OstreeFormat.h"

#include <BackendNotifierModule.h>

#include <QDebug>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QTimer>

/* Look for new system updates with rpm-ostree.
 * Uses only the rpm-ostree command line to simplify logic for now.
 * TODO: Use the DBus interface.
 */
class RpmOstreeNotifier : public BackendNotifierModule
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DISCOVER_NOTIFIER_IID)
    Q_INTERFACES(BackendNotifierModule)
public:
    explicit RpmOstreeNotifier(QObject *parent = nullptr);

    void recheckSystemUpdateNeeded() override;
    bool hasSecurityUpdates() override;
    bool hasUpdates() override;
    bool needsReboot() const override;

private:
    /* Only run this code if we are on an rpm-ostree managed system */
    bool isValid() const;

    /* Called by recheckSystemUpdateNeeded to check for system update when the classic
     * ostree format is used. */
    void checkSystemUpdateClassic();

    /* Called by recheckSystemUpdateNeeded to check for system update when the OCI
     * ostree format is used. */
    void checkSystemUpdateOCI();

    /* Store which format is used for the ostree image */
    QScopedPointer<::OstreeFormat> m_ostreeFormat;

    /* Store the version of the currently booted deployment */
    QString m_version;

    /* Tracks the rpm-ostree command used to check for updates or to look at the
     * status. */
    QProcess *m_process;

    /* Store standard output from rpm-ostree command line calls */
    QByteArray m_stdout;

    /* The update version that we've already found in a previous check. Used to
     * only notify once about an update for a given version. */
    QString m_updateVersion;

    /* Setup the paths in the filesystem watcher to look for changes in deployments */
    void setupWatcherPaths();

    /* Check if we already have a pending deployment for the version availalbe
     * for update */
    void checkForPendingDeployment();

    /* Do we have updates available? */
    bool m_hasUpdates;

    /* Do we need to reboot to apply updates? */
    bool m_needsReboot;

    /* Watcher to trigger a reboot check when deployments are modified */
    QFileSystemWatcher *m_watcher;

    /* Timer triggered by the above watcher to wait for things to settle down before re-doing a deployment check */
    QTimer *m_timer;
};
