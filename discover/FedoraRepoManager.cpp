/*
 *   SPDX-FileCopyrightText: 2025 Discover Plus Contributors
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "FedoraRepoManager.h"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>

FedoraRepoManager *FedoraRepoManager::s_instance = nullptr;

FedoraRepoManager::FedoraRepoManager(QObject *parent)
    : QObject(parent)
{
    // Check if we're on Fedora
    QFile osRelease(QStringLiteral("/etc/os-release"));
    if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(osRelease.readAll());
        m_isFedora = content.contains(QStringLiteral("ID=fedora")) ||
                     content.contains(QStringLiteral("ID_LIKE=fedora"));
        osRelease.close();
    }

    // Load first run state
    QSettings settings(QStringLiteral("discover-plus"), QStringLiteral("discover-plus"));
    m_firstRunCompleted = settings.value(QStringLiteral("firstRunCompleted"), false).toBool();

    if (m_isFedora) {
        refreshStatus();
    }
}

FedoraRepoManager *FedoraRepoManager::create(QQmlEngine *engine, QJSEngine *)
{
    Q_UNUSED(engine)
    return instance();
}

FedoraRepoManager *FedoraRepoManager::instance()
{
    if (!s_instance) {
        s_instance = new FedoraRepoManager();
    }
    return s_instance;
}

bool FedoraRepoManager::isFedora() const
{
    return m_isFedora;
}

bool FedoraRepoManager::rpmFusionFreeInstalled() const
{
    return m_rpmFusionFreeInstalled;
}

bool FedoraRepoManager::rpmFusionNonfreeInstalled() const
{
    return m_rpmFusionNonfreeInstalled;
}

bool FedoraRepoManager::rpmFusionFreeAppstreamInstalled() const
{
    return m_rpmFusionFreeAppstreamInstalled;
}

bool FedoraRepoManager::rpmFusionNonfreeAppstreamInstalled() const
{
    return m_rpmFusionNonfreeAppstreamInstalled;
}

bool FedoraRepoManager::flathubInstalled() const
{
    return m_flathubInstalled;
}

bool FedoraRepoManager::setupNeeded() const
{
    if (!m_isFedora) {
        return false;
    }
    // Setup needed if RPM Fusion Free or Nonfree repos are not installed
    return !m_rpmFusionFreeInstalled || !m_rpmFusionNonfreeInstalled;
}

bool FedoraRepoManager::firstRunCompleted() const
{
    return m_firstRunCompleted;
}

void FedoraRepoManager::setFirstRunCompleted(bool completed)
{
    if (m_firstRunCompleted != completed) {
        m_firstRunCompleted = completed;
        QSettings settings(QStringLiteral("discover-plus"), QStringLiteral("discover-plus"));
        settings.setValue(QStringLiteral("firstRunCompleted"), completed);
        Q_EMIT firstRunCompletedChanged();
    }
}

bool FedoraRepoManager::installing() const
{
    return m_installing;
}

QString FedoraRepoManager::installError() const
{
    return m_installError;
}

void FedoraRepoManager::refreshStatus()
{
    if (!m_isFedora) {
        return;
    }

    // Check RPM Fusion Free
    QProcess rpmQueryFree;
    rpmQueryFree.start(QStringLiteral("rpm"), {QStringLiteral("-q"), QStringLiteral("rpmfusion-free-release")});
    rpmQueryFree.waitForFinished(5000);
    m_rpmFusionFreeInstalled = (rpmQueryFree.exitCode() == 0);

    // Check RPM Fusion Nonfree
    QProcess rpmQueryNonfree;
    rpmQueryNonfree.start(QStringLiteral("rpm"), {QStringLiteral("-q"), QStringLiteral("rpmfusion-nonfree-release")});
    rpmQueryNonfree.waitForFinished(5000);
    m_rpmFusionNonfreeInstalled = (rpmQueryNonfree.exitCode() == 0);

    // Check RPM Fusion Free AppStream data
    QProcess rpmQueryFreeAppstream;
    rpmQueryFreeAppstream.start(QStringLiteral("rpm"), {QStringLiteral("-q"), QStringLiteral("rpmfusion-free-appstream-data")});
    rpmQueryFreeAppstream.waitForFinished(5000);
    m_rpmFusionFreeAppstreamInstalled = (rpmQueryFreeAppstream.exitCode() == 0);

    // Check RPM Fusion Nonfree AppStream data
    QProcess rpmQueryNonfreeAppstream;
    rpmQueryNonfreeAppstream.start(QStringLiteral("rpm"), {QStringLiteral("-q"), QStringLiteral("rpmfusion-nonfree-appstream-data")});
    rpmQueryNonfreeAppstream.waitForFinished(5000);
    m_rpmFusionNonfreeAppstreamInstalled = (rpmQueryNonfreeAppstream.exitCode() == 0);

    // Check Flathub
    QProcess flatpakRemotes;
    flatpakRemotes.start(QStringLiteral("flatpak"), {QStringLiteral("remotes"), QStringLiteral("--columns=name")});
    flatpakRemotes.waitForFinished(5000);
    QString remotes = QString::fromUtf8(flatpakRemotes.readAllStandardOutput());
    m_flathubInstalled = remotes.contains(QStringLiteral("flathub"));

    Q_EMIT statusChanged();
}

void FedoraRepoManager::installRpmFusion(bool free, bool nonfree, bool appstreamData)
{
    if (m_installing) {
        return;
    }

    QStringList releasePackages;
    QStringList appstreamPackages;

    // Get Fedora version using rpm -E %fedora (most reliable method)
    QString fedoraVersion;
    QProcess rpmQuery;
    rpmQuery.start(QStringLiteral("rpm"), {QStringLiteral("-E"), QStringLiteral("%fedora")});
    rpmQuery.waitForFinished(5000);
    if (rpmQuery.exitCode() == 0) {
        fedoraVersion = QString::fromUtf8(rpmQuery.readAllStandardOutput()).trimmed();
    }

    if (fedoraVersion.isEmpty()) {
        m_installError = tr("Could not determine Fedora version");
        Q_EMIT installErrorChanged();
        Q_EMIT installationFinished(false);
        return;
    }

    // Stage 1: RPM Fusion release packages (repositories)
    if (free && !m_rpmFusionFreeInstalled) {
        releasePackages << QStringLiteral("https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-%1.noarch.rpm").arg(fedoraVersion);
    }

    if (nonfree && !m_rpmFusionNonfreeInstalled) {
        releasePackages << QStringLiteral("https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-%1.noarch.rpm").arg(fedoraVersion);
    }

    // Stage 2: AppStream data (must be installed AFTER release packages)
    if (appstreamData) {
        if (free) {
            appstreamPackages << QStringLiteral("rpmfusion-free-appstream-data");
        }
        if (nonfree) {
            appstreamPackages << QStringLiteral("rpmfusion-nonfree-appstream-data");
        }
    }

    if (releasePackages.isEmpty() && appstreamPackages.isEmpty()) {
        Q_EMIT installationFinished(true);
        return;
    }

    // If we need to install release packages first, do two-stage install
    if (!releasePackages.isEmpty()) {
        m_installing = true;
        m_installError.clear();
        Q_EMIT installingChanged();
        Q_EMIT installErrorChanged();

        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process, appstreamPackages](int exitCode, QProcess::ExitStatus) {
            process->deleteLater();

            if (exitCode != 0) {
                m_installing = false;
                m_installError = QString::fromUtf8(process->readAllStandardError());
                if (m_installError.isEmpty()) {
                    m_installError = QString::fromUtf8(process->readAllStandardOutput());
                }
                Q_EMIT installingChanged();
                Q_EMIT installErrorChanged();
                Q_EMIT installationFinished(false);
                return;
            }

            // Stage 1 complete, now install appstream data
            refreshStatus();

            if (!appstreamPackages.isEmpty()) {
                runDnfInstall(appstreamPackages);
            } else {
                m_installing = false;
                Q_EMIT installingChanged();
                Q_EMIT installationFinished(true);
            }
        });

        QStringList args = {QStringLiteral("dnf"), QStringLiteral("install"), QStringLiteral("-y")};
        args.append(releasePackages);
        process->start(QStringLiteral("pkexec"), args);
    } else if (!appstreamPackages.isEmpty()) {
        // Only appstream packages needed (release already installed)
        runDnfInstall(appstreamPackages);
    }
}

void FedoraRepoManager::installFlathub()
{
    if (m_installing || m_flathubInstalled) {
        return;
    }

    m_installing = true;
    m_installError.clear();
    Q_EMIT installingChanged();
    Q_EMIT installErrorChanged();

    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
        m_installing = false;
        Q_EMIT installingChanged();

        if (exitCode != 0) {
            m_installError = QString::fromUtf8(process->readAllStandardError());
            Q_EMIT installErrorChanged();
            Q_EMIT installationFinished(false);
        } else {
            refreshStatus();
            Q_EMIT installationFinished(true);
        }
        process->deleteLater();
    });

    // Use pkexec for elevated privileges
    process->start(QStringLiteral("pkexec"),
                   {QStringLiteral("flatpak"),
                    QStringLiteral("remote-add"),
                    QStringLiteral("--if-not-exists"),
                    QStringLiteral("flathub"),
                    QStringLiteral("https://dl.flathub.org/repo/flathub.flatpakrepo")});
}

void FedoraRepoManager::runDnfInstall(const QStringList &packages)
{
    m_installing = true;
    m_installError.clear();
    Q_EMIT installingChanged();
    Q_EMIT installErrorChanged();

    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
        m_installing = false;
        Q_EMIT installingChanged();

        if (exitCode != 0) {
            m_installError = QString::fromUtf8(process->readAllStandardError());
            if (m_installError.isEmpty()) {
                m_installError = QString::fromUtf8(process->readAllStandardOutput());
            }
            Q_EMIT installErrorChanged();
            Q_EMIT installationFinished(false);
        } else {
            refreshStatus();
            Q_EMIT installationFinished(true);
        }
        process->deleteLater();
    });

    // Use pkexec for elevated privileges
    QStringList args = {QStringLiteral("dnf"), QStringLiteral("install"), QStringLiteral("-y")};
    args.append(packages);
    process->start(QStringLiteral("pkexec"), args);
}

#include "moc_FedoraRepoManager.cpp"
