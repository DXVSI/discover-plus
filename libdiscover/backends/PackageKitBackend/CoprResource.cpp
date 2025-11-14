#include "CoprResource.h"
#include "PackageKitBackend.h"

#include <KLocalizedString>
#include <KService>
#include <KIO/ApplicationLauncherJob>
#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

CoprResource::CoprResource(const CoprPackageInfo &packageInfo, AbstractResourcesBackend *parent)
    : PackageKitResource(packageInfo.name, QString(), qobject_cast<PackageKitBackend*>(parent))
    , m_owner(packageInfo.owner)
    , m_project(packageInfo.projectName)
    , m_description(packageInfo.description)
    , m_availableChroots(packageInfo.availableChroots)
    , m_isAvailableForCurrentFedora(packageInfo.isAvailableForCurrentFedora)
    , m_homepage(packageInfo.homepage)
{
    // Check if the package is already installed
    checkInstalledState();
}

CoprResource::~CoprResource()
{
}

QString CoprResource::section()
{
    return i18n("COPR Packages");
}

QString CoprResource::origin() const
{
    return QStringLiteral("COPR");
}

QString CoprResource::comment()
{
    // Return a short one-line description for the package list view
    // Don't return the full HTML description here
    return i18n("COPR package from %1/%2", m_owner, m_project);
}

QString CoprResource::longDescription()
{
    // If we have a description from COPR (already converted from Markdown to HTML),
    // just return it as is - it already contains all the information
    if (!m_description.isEmpty()) {
        QString desc = m_description;

        // Only add our additional metadata at the end
        desc += QStringLiteral("<br><br><hr><br>");
        desc += QStringLiteral("<b>") + i18n("COPR Repository:") + QStringLiteral("</b> ");
        desc += m_owner + QStringLiteral("/") + m_project;

        // Parse available versions
        QStringList fedoraVersions;
        QStringList architectures;

        for (const QString &chroot : m_availableChroots) {
            if (chroot.startsWith(QStringLiteral("fedora-"))) {
                QStringList parts = chroot.split(QLatin1Char('-'));
                if (parts.size() >= 3) {
                    QString version = parts[1];
                    QString arch = parts[2];

                    if (!fedoraVersions.contains(version)) {
                        fedoraVersions.append(version);
                    }
                    if (!architectures.contains(arch)) {
                        architectures.append(arch);
                    }
                }
            }
        }

        if (!fedoraVersions.isEmpty()) {
            desc += QStringLiteral("<br><br>");
            desc += QStringLiteral("<b>") + i18n("Available for:") + QStringLiteral("</b> Fedora ");
            desc += fedoraVersions.join(QStringLiteral(", "));
            desc += QStringLiteral(" (");
            desc += architectures.join(QStringLiteral(", "));
            desc += QStringLiteral(")");
        }

        desc += QStringLiteral("<br><br>");
        if (m_isAvailableForCurrentFedora) {
            desc += QStringLiteral("<span style='color: green; font-weight: bold;'>✓ ") + i18n("Available for your Fedora version") + QStringLiteral("</span>");
        } else {
            desc += QStringLiteral("<span style='color: red; font-weight: bold;'>⚠ ") + i18n("Not available for your Fedora version") + QStringLiteral("</span>");
        }

        desc += QStringLiteral("<br><br>");
        desc += QStringLiteral("<span style='color: #ff8800;'><b>") + i18n("Notice:") + QStringLiteral("</b> ");
        desc += i18n("COPR repositories are not officially supported by Fedora. Use at your own risk.") + QStringLiteral("</span>");

        return desc;
    } else {
        // No description from COPR, create a default one
        return i18n("Package from COPR repository %1/%2", m_owner, m_project);
    }
}

QString CoprResource::availableVersion() const
{
    return i18n("COPR latest");
}

QString CoprResource::installedVersion() const
{
    return QString();
}

QUrl CoprResource::homepage()
{
    if (!m_homepage.isEmpty()) {
        return QUrl(m_homepage);
    }

    return QUrl(QStringLiteral("https://copr.fedorainfracloud.org/coprs/%1/%2/").arg(m_owner, m_project));
}

AbstractResource::State CoprResource::state()
{
    // Check if the package is actually installed
    // We need to query the system to see if the package exists

    // For now, let's check using a simple approach
    // Later we can integrate with PackageKit to get the actual state

    if (m_isInstalled) {
        return AbstractResource::Installed;
    }

    return AbstractResource::None;
}

void CoprResource::setState(AbstractResource::State state)
{
    m_isInstalled = (state == AbstractResource::Installed);
    Q_EMIT stateChanged();

    // Also emit the change through the backend so the UI updates
    if (backend()) {
        Q_EMIT backend()->resourcesChanged(this, {"state"});
    }
}

QVariant CoprResource::icon() const
{
    return QStringLiteral("package");
}

QString CoprResource::sizeDescription()
{
    return i18n("Unknown size");
}

QString CoprResource::author() const
{
    // Return the COPR owner as the author
    return m_owner;
}

void CoprResource::checkInstalledState()
{
    // Store previous state to check if it actually changed
    bool wasInstalled = m_isInstalled;

    // Check if the package is installed using rpm command
    QProcess process;
    process.start(QStringLiteral("rpm"), {QStringLiteral("-q"), packageName()});
    process.waitForFinished();

    if (process.exitCode() != 0) {
        // Package is not installed at all
        m_isInstalled = false;
    } else {
        // Package is installed, now check if it's from this specific COPR repo
        // Get the vendor/packager info to determine the source
        QProcess vendorProcess;
        vendorProcess.start(QStringLiteral("rpm"), {QStringLiteral("-qi"), packageName()});
        vendorProcess.waitForFinished();

        QString output = QString::fromUtf8(vendorProcess.readAllStandardOutput());

        // Check if the package is from the specific COPR repository
        // COPR packages have "Vendor: Fedora Copr - user <owner>" format
        QString vendorString = QStringLiteral("Fedora Copr - user ") + m_owner;
        QString vendorString2 = QStringLiteral("Fedora Copr - group ") + m_owner;  // For group-owned repos

        // Also check in the source RPM name which often includes repo info
        QString sourcePattern = m_owner + QStringLiteral("-") + packageName();
        QString sourcePattern2 = QStringLiteral("copr:") + m_owner;

        m_isInstalled = output.contains(vendorString, Qt::CaseInsensitive) ||
                       output.contains(vendorString2, Qt::CaseInsensitive) ||
                       output.contains(sourcePattern, Qt::CaseInsensitive) ||
                       output.contains(sourcePattern2, Qt::CaseInsensitive);
    }

    // Only emit signal if state actually changed
    if (wasInstalled != m_isInstalled) {
        Q_EMIT stateChanged();
    }
}

bool CoprResource::canExecute() const
{
    // Only executable if installed
    if (!m_isInstalled) {
        return false;
    }

    // Try to find a desktop file for this package
    QString desktopFile = findDesktopFile();
    return !desktopFile.isEmpty();
}

void CoprResource::invokeApplication() const
{
    if (!canExecute()) {
        Q_EMIT backend()->passiveMessage(i18n("Cannot launch %1", name()));
        return;
    }

    QString desktopFile = findDesktopFile();
    if (desktopFile.isEmpty()) {
        Q_EMIT backend()->passiveMessage(i18n("No launcher found for %1", name()));
        return;
    }

    // Try to launch using the desktop file
    KService::Ptr service = KService::serviceByStorageId(desktopFile);
    if (!service) {
        service = KService::serviceByDesktopPath(desktopFile);
    }

    if (!service) {
        Q_EMIT backend()->passiveMessage(i18n("Cannot launch %1", name()));
        return;
    }

    // Launch the application
    auto *job = new KIO::ApplicationLauncherJob(service);
    connect(job, &KJob::finished, this, [this, service](KJob *job) {
        if (job->error()) {
            Q_EMIT backend()->passiveMessage(i18n("Failed to start '%1': %2", service->name(), job->errorString()));
        }
    });
    job->start();
}

QString CoprResource::findDesktopFile() const
{
    // Common locations for desktop files
    QStringList desktopDirs = {
        QStringLiteral("/usr/share/applications"),
        QStringLiteral("/usr/local/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/applications")
    };

    // Try to find desktop file matching the package name
    QString packageNameLower = packageName().toLower();

    for (const QString &dir : desktopDirs) {
        QDir applicationsDir(dir);
        if (!applicationsDir.exists()) {
            continue;
        }

        // Try exact match first
        QString exactMatch = packageNameLower + QStringLiteral(".desktop");
        if (applicationsDir.exists(exactMatch)) {
            return dir + QStringLiteral("/") + exactMatch;
        }

        // Try with org. prefix (common for Flatpak-style naming)
        QString orgMatch = QStringLiteral("org.") + packageNameLower + QStringLiteral(".desktop");
        if (applicationsDir.exists(orgMatch)) {
            return dir + QStringLiteral("/") + orgMatch;
        }

        // Try case-insensitive search
        QDirIterator it(dir, QStringList() << QStringLiteral("*.desktop"), QDir::Files);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString fileName = QFileInfo(filePath).baseName().toLower();

            // Check if filename contains package name
            if (fileName.contains(packageNameLower)) {
                return filePath;
            }
        }
    }

    return QString();
}
