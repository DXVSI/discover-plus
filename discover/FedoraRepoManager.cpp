/*
 *   SPDX-FileCopyrightText: 2025 Discover Plus Contributors
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "FedoraRepoManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>

FedoraRepoManager *FedoraRepoManager::s_instance = nullptr;

FedoraRepoManager::FedoraRepoManager(QObject *parent)
    : QObject(parent)
{
    // Check if we're on Fedora
    QFile osRelease(QStringLiteral("/etc/os-release"));
    if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(osRelease.readAll());
        m_isFedora = content.contains(QStringLiteral("ID=fedora")) || content.contains(QStringLiteral("ID_LIKE=fedora"));
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

bool FedoraRepoManager::dnfConfigured() const
{
    return m_dnfConfigured;
}

bool FedoraRepoManager::ciscoRepoEnabled() const
{
    return m_ciscoRepoEnabled;
}

bool FedoraRepoManager::googleChromeRepoEnabled() const
{
    return m_googleChromeRepoEnabled;
}

bool FedoraRepoManager::nvidiaRepoEnabled() const
{
    return m_nvidiaRepoEnabled;
}

bool FedoraRepoManager::steamRepoEnabled() const
{
    return m_steamRepoEnabled;
}

bool FedoraRepoManager::setupNeeded() const
{
    if (!m_isFedora) {
        return false;
    }
    return !m_dnfConfigured || m_ciscoRepoEnabled || !m_rpmFusionFreeInstalled || !m_rpmFusionNonfreeInstalled || !m_flathubInstalled;
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

bool FedoraRepoManager::isRepoEnabled(const QString &repoId) const
{
    // First check dnf5 override files (take priority over .repo files)
    const QStringList overrideDirs = {
        QStringLiteral("/etc/dnf/repos.override.d"),
        QStringLiteral("/etc/dnf5/repos.override.d"),
    };
    const QString sectionHeader = QStringLiteral("[%1]").arg(repoId);
    const QRegularExpression enabledRe(QStringLiteral("^enabled\\s*=\\s*(\\d+)"), QRegularExpression::MultilineOption);

    for (const QString &dirPath : overrideDirs) {
        QDir overrideDir(dirPath);
        if (!overrideDir.exists()) {
            continue;
        }
        const QStringList overrideFiles = overrideDir.entryList({QStringLiteral("*.repo")}, QDir::Files);
        for (const QString &fileName : overrideFiles) {
            QFile file(overrideDir.filePath(fileName));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            const QString content = QString::fromUtf8(file.readAll());
            file.close();

            const int sectionPos = content.indexOf(sectionHeader);
            if (sectionPos < 0) {
                continue;
            }

            const int nextSection = content.indexOf(QStringLiteral("\n["), sectionPos + 1);
            const QString section = (nextSection > 0) ? content.mid(sectionPos, nextSection - sectionPos) : content.mid(sectionPos);

            const QRegularExpressionMatch match = enabledRe.match(section);
            if (match.hasMatch()) {
                return match.captured(1) == QStringLiteral("1");
            }
        }
    }

    // Then check standard .repo files
    QDir repoDir(QStringLiteral("/etc/yum.repos.d"));
    const QStringList repoFiles = repoDir.entryList({QStringLiteral("*.repo")}, QDir::Files);

    for (const QString &fileName : repoFiles) {
        QFile file(repoDir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        const QString content = QString::fromUtf8(file.readAll());
        file.close();

        const int sectionPos = content.indexOf(sectionHeader);
        if (sectionPos < 0) {
            continue;
        }

        // Extract section content until next [section] or end of file
        const int nextSection = content.indexOf(QStringLiteral("\n["), sectionPos + 1);
        const QString section = (nextSection > 0) ? content.mid(sectionPos, nextSection - sectionPos) : content.mid(sectionPos);

        const QRegularExpressionMatch match = enabledRe.match(section);
        if (match.hasMatch()) {
            return match.captured(1) == QStringLiteral("1");
        }

        // No enabled= line means enabled by default
        return true;
    }

    // Repo section not found in any file
    return false;
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
    const QString remotes = QString::fromUtf8(flatpakRemotes.readAllStandardOutput());
    m_flathubInstalled = remotes.contains(QStringLiteral("flathub"));

    // Check DNF configuration
    QFile dnfConf(QStringLiteral("/etc/dnf/dnf.conf"));
    if (dnfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString content = QString::fromUtf8(dnfConf.readAll());
        dnfConf.close();
        m_dnfConfigured = content.contains(QStringLiteral("max_parallel_downloads=")) && content.contains(QStringLiteral("fastestmirror="));
    } else {
        m_dnfConfigured = false;
    }

    // Check repo states
    m_ciscoRepoEnabled = isRepoEnabled(QStringLiteral("fedora-cisco-openh264"));
    m_googleChromeRepoEnabled = isRepoEnabled(QStringLiteral("google-chrome"));
    m_nvidiaRepoEnabled = isRepoEnabled(QStringLiteral("rpmfusion-nonfree-nvidia-driver"));
    m_steamRepoEnabled = isRepoEnabled(QStringLiteral("rpmfusion-nonfree-steam"));

    Q_EMIT statusChanged();
}

void FedoraRepoManager::applySetup(bool configureDnf,
                                   bool disableCisco,
                                   bool rpmFusionFree,
                                   bool rpmFusionNonfree,
                                   bool flathub,
                                   bool enableChrome,
                                   bool enableNvidia,
                                   bool enableSteam)
{
    if (m_installing) {
        return;
    }

    // Get Fedora version
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

    // Build setup script
    QStringList script;
    script << QStringLiteral("#!/bin/bash");
    bool hasCommands = false;

    // 1. DNF configuration
    if (configureDnf && !m_dnfConfigured) {
        hasCommands = true;
        script << QString();
        script << QStringLiteral("# Optimize DNF configuration");
        script << QStringLiteral("conf=/etc/dnf/dnf.conf");
        script << QStringLiteral("grep -q '^max_parallel_downloads=' \"$conf\" 2>/dev/null || echo 'max_parallel_downloads=10' >> \"$conf\"");
        script << QStringLiteral("grep -q '^fastestmirror=' \"$conf\" 2>/dev/null || echo 'fastestmirror=True' >> \"$conf\"");
        script << QStringLiteral("grep -q '^defaultyes=' \"$conf\" 2>/dev/null || echo 'defaultyes=True' >> \"$conf\"");
        script << QStringLiteral("grep -q '^keepcache=' \"$conf\" 2>/dev/null || echo 'keepcache=True' >> \"$conf\"");
    }

    // 2. Disable Cisco OpenH264
    if (disableCisco && m_ciscoRepoEnabled) {
        hasCommands = true;
        script << QString();
        script << QStringLiteral("# Disable Cisco OpenH264 repository");
        script << QStringLiteral("mkdir -p /etc/dnf/repos.override.d");
        script << QStringLiteral(
            "if [ -f /etc/dnf/repos.override.d/99-config_manager.repo ] && grep -q '\\[fedora-cisco-openh264\\]' "
            "/etc/dnf/repos.override.d/99-config_manager.repo "
            "2>/dev/null; then");
        script << QStringLiteral("    sed -i '/\\[fedora-cisco-openh264\\]/,/^\\[/{s/enabled=1/enabled=0/}' /etc/dnf/repos.override.d/99-config_manager.repo");
        script << QStringLiteral("else");
        script << QStringLiteral("    printf '\\n[fedora-cisco-openh264]\\nenabled=0\\n' >> /etc/dnf/repos.override.d/99-config_manager.repo");
        script << QStringLiteral("fi");
    }

    // 3. RPM Fusion
    QStringList fusionPackages;
    if (rpmFusionFree && !m_rpmFusionFreeInstalled) {
        fusionPackages << QStringLiteral("https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-%1.noarch.rpm").arg(fedoraVersion);
    }
    if (rpmFusionNonfree && !m_rpmFusionNonfreeInstalled) {
        fusionPackages << QStringLiteral("https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-%1.noarch.rpm").arg(fedoraVersion);
    }
    if (!fusionPackages.isEmpty()) {
        hasCommands = true;
        script << QString();
        script << QStringLiteral("# Install RPM Fusion repositories");
        script << QStringLiteral("dnf install -y %1").arg(fusionPackages.join(QStringLiteral(" ")));
    }

    // 4. AppStream data
    QStringList appstreamPackages;
    if (rpmFusionFree && !m_rpmFusionFreeAppstreamInstalled) {
        appstreamPackages << QStringLiteral("rpmfusion-free-appstream-data");
    }
    if (rpmFusionNonfree && !m_rpmFusionNonfreeAppstreamInstalled) {
        appstreamPackages << QStringLiteral("rpmfusion-nonfree-appstream-data");
    }
    if (!appstreamPackages.isEmpty()) {
        hasCommands = true;
        script << QString();
        script << QStringLiteral("# Install AppStream metadata");
        script << QStringLiteral("dnf install -y %1").arg(appstreamPackages.join(QStringLiteral(" ")));
    }

    // 5-7. Enable repos via dnf5 override (works correctly with dnf5 config-manager)
    QStringList reposToEnable;
    if (enableNvidia && !m_nvidiaRepoEnabled) {
        reposToEnable << QStringLiteral("rpmfusion-nonfree-nvidia-driver");
    }
    if (enableSteam && !m_steamRepoEnabled) {
        reposToEnable << QStringLiteral("rpmfusion-nonfree-steam");
    }
    if (enableChrome && !m_googleChromeRepoEnabled) {
        // If Chrome repo doesn't exist at all, create it first
        script << QString();
        script << QStringLiteral("# Ensure Google Chrome repository exists");
        script << QStringLiteral(
            "if ! grep -rq '\\[google-chrome\\]' /etc/yum.repos.d/ 2>/dev/null && "
            "! grep -rq '\\[google-chrome\\]' /etc/dnf/repos.override.d/ 2>/dev/null; then");
        script << QStringLiteral("    cat > /etc/yum.repos.d/google-chrome.repo << 'REPO'");
        script << QStringLiteral("[google-chrome]");
        script << QStringLiteral("name=google-chrome");
        script << QStringLiteral("baseurl=https://dl.google.com/linux/chrome/rpm/stable/x86_64");
        script << QStringLiteral("enabled=1");
        script << QStringLiteral("gpgcheck=1");
        script << QStringLiteral("gpgkey=https://dl.google.com/linux/linux_signing_key.pub");
        script << QStringLiteral("REPO");
        script << QStringLiteral("fi");
        hasCommands = true;
        reposToEnable << QStringLiteral("google-chrome");
    }

    if (!reposToEnable.isEmpty()) {
        hasCommands = true;
        script << QString();
        script << QStringLiteral("# Enable additional repositories via dnf5 override");
        script << QStringLiteral("mkdir -p /etc/dnf/repos.override.d");
        script << QStringLiteral("override_file=/etc/dnf/repos.override.d/99-config_manager.repo");
        for (const QString &repo : reposToEnable) {
            // Add override entry if not already present
            script << QStringLiteral("if ! grep -q '\\[%1\\]' \"$override_file\" 2>/dev/null; then").arg(repo);
            script << QStringLiteral("    printf '\\n[%1]\\nenabled=1\\n' >> \"$override_file\"").arg(repo);
            script << QStringLiteral("else");
            script << QStringLiteral("    sed -i '/\\[%1\\]/,/^\\[/{s/enabled=0/enabled=1/}' \"$override_file\"").arg(repo);
            script << QStringLiteral("fi");
        }
    }

    // 8. Flathub
    if (flathub && !m_flathubInstalled) {
        hasCommands = true;
        script << QString();
        script << QStringLiteral("# Add Flathub repository");
        script << QStringLiteral("flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo");
    }

    if (!hasCommands) {
        setFirstRunCompleted(true);
        Q_EMIT installationFinished(true);
        return;
    }

    // Write script to temp file
    auto *tmpScript = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/discover-setup-XXXXXX.sh"), this);
    if (!tmpScript->open()) {
        m_installError = tr("Could not create temporary script file");
        Q_EMIT installErrorChanged();
        Q_EMIT installationFinished(false);
        tmpScript->deleteLater();
        return;
    }

    tmpScript->write(script.join(QStringLiteral("\n")).toUtf8());
    tmpScript->flush();
    tmpScript->setAutoRemove(false);
    const QString scriptPath = tmpScript->fileName();
    tmpScript->close();

    QFile::setPermissions(scriptPath,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther
                              | QFileDevice::ExeOther);

    m_installing = true;
    m_installError.clear();
    Q_EMIT installingChanged();
    Q_EMIT installErrorChanged();

    auto *process = new QProcess(this);
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process, scriptPath, tmpScript](int exitCode, QProcess::ExitStatus) {
                m_installing = false;
                Q_EMIT installingChanged();

                // Clean up temp file
                QFile::remove(scriptPath);
                tmpScript->deleteLater();

                if (exitCode == 126) {
                    // User cancelled pkexec authentication
                    Q_EMIT installationFinished(false);
                } else if (exitCode != 0) {
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

    process->start(QStringLiteral("pkexec"), {QStringLiteral("bash"), scriptPath});
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
        connect(process,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                [this, process, appstreamPackages](int exitCode, QProcess::ExitStatus) {
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
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus) {
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
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus) {
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
