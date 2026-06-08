#include "CoprResource.h"
#include "PackageKitBackend.h"

#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KService>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QLocale>
#include <QVariantMap>

#include <algorithm>

static QString formatBuildDateTime(const QDateTime &dateTime)
{
    if (!dateTime.isValid()) {
        return {};
    }

    return QLocale().toString(dateTime.toLocalTime(), QLocale::ShortFormat);
}

static QString formattedChrootsSummary(const QStringList &chroots)
{
    QStringList fedoraVersions;
    QStringList architectures;

    for (const QString &chroot : chroots) {
        if (!chroot.startsWith(QStringLiteral("fedora-"))) {
            continue;
        }

        const QStringList parts = chroot.split(QLatin1Char('-'));
        if (parts.size() < 3) {
            continue;
        }

        const QString version = parts[1];
        const QString arch = parts[2];

        if (!fedoraVersions.contains(version)) {
            fedoraVersions.append(version);
        }
        if (!architectures.contains(arch)) {
            architectures.append(arch);
        }
    }

    if (fedoraVersions.isEmpty()) {
        return chroots.join(QStringLiteral(", "));
    }

    if (architectures.isEmpty()) {
        return QStringLiteral("Fedora %1").arg(fedoraVersions.join(QStringLiteral(", ")));
    }

    return QStringLiteral("Fedora %1 (%2)").arg(fedoraVersions.join(QStringLiteral(", ")), architectures.join(QStringLiteral(", ")));
}

static QStringList mergedChroots(const QStringList &baseChroots, const QList<CoprPackageInfo> &packages)
{
    QStringList chroots = baseChroots;
    for (const CoprPackageInfo &package : packages) {
        for (const QString &chroot : package.availableChroots) {
            if (!chroots.contains(chroot)) {
                chroots.append(chroot);
            }
        }
    }
    return chroots;
}

CoprResource::CoprResource(const CoprPackageInfo &packageInfo, AbstractResourcesBackend *parent)
    : PackageKitResource(packageInfo.name, QString(), qobject_cast<PackageKitBackend *>(parent))
    , m_owner(packageInfo.owner)
    , m_project(packageInfo.projectName)
    , m_installPackageName(packageInfo.isProjectResource ? QString() : packageInfo.name)
    , m_projectFullName(packageInfo.projectFullName)
    , m_description(packageInfo.description)
    , m_version(packageInfo.version)
    , m_availableChroots(packageInfo.availableChroots)
    , m_isAvailableForCurrentFedora(packageInfo.isAvailableForCurrentFedora)
    , m_homepage(packageInfo.homepage)
    , m_instructions(packageInfo.instructions)
    , m_contact(packageInfo.contact)
    , m_additionalRepos(packageInfo.additionalRepos)
    , m_repoPriority(packageInfo.repoPriority)
    , m_appstream(packageInfo.appstream)
    , m_develMode(packageInfo.develMode)
    , m_enableNet(packageInfo.enableNet)
    , m_followFedoraBranching(packageInfo.followFedoraBranching)
    , m_autoPrune(packageInfo.autoPrune)
    , m_moduleHotfixes(packageInfo.moduleHotfixes)
    , m_isProjectResource(packageInfo.isProjectResource)
    , m_sourceType(packageInfo.sourceType)
    , m_sourceUrl(packageInfo.sourceUrl)
    , m_sourceSpec(packageInfo.sourceSpec)
    , m_sourceSubdirectory(packageInfo.sourceSubdirectory)
    , m_latestBuildState(packageInfo.latestBuildState)
    , m_latestBuildRepoUrl(packageInfo.latestBuildRepoUrl)
    , m_latestBuildSubmitter(packageInfo.latestBuildSubmitter)
    , m_latestBuildSubmittedOn(packageInfo.latestBuildSubmittedOn)
    , m_latestBuildStartedOn(packageInfo.latestBuildStartedOn)
    , m_latestBuildEndedOn(packageInfo.latestBuildEndedOn)
{
    // Check if the package is already installed (deferred to avoid blocking the UI during batch creation)
    QMetaObject::invokeMethod(this, &CoprResource::checkInstalledState, Qt::QueuedConnection);
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

QString CoprResource::packageName() const
{
    return m_installPackageName;
}

QStringList CoprResource::allPackageNames() const
{
    return m_installPackageName.isEmpty() ? QStringList() : QStringList{m_installPackageName};
}

QString CoprResource::comment()
{
    // Return a short one-line description for the package list view
    // Don't return the full HTML description here
    return i18n("COPR package from %1", m_projectFullName.isEmpty() ? QStringLiteral("%1/%2").arg(m_owner, m_project) : m_projectFullName);
}

QString CoprResource::longDescription()
{
    if (!m_description.isEmpty()) {
        return m_description;
    }

    return i18n("Package from COPR repository %1/%2", m_owner, m_project);
}

void CoprResource::fetchProjectPackages()
{
    if (!m_isProjectResource || m_projectPackagesRequested) {
        return;
    }

    m_projectPackagesRequested = true;
    if (auto pkBackend = qobject_cast<PackageKitBackend *>(backend())) {
        if (auto client = pkBackend->coprClient()) {
            client->getProjectPackages(m_owner, m_project);
        }
    }
}

QString CoprResource::availableVersion() const
{
    return m_version;
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

QStringList CoprResource::topObjects() const
{
    QStringList objects = PackageKitResource::topObjects();
    if (m_isProjectResource) {
        objects.append(QStringLiteral("qrc:/qml/CoprPackageSelector.qml"));
    }
    objects.append(QStringLiteral("qrc:/qml/CoprDetails.qml"));
    return objects;
}

QVariantList CoprResource::coprProjectPackages() const
{
    QVariantList packages;
    packages.reserve(m_projectPackages.size());

    for (const CoprPackageInfo &package : m_projectPackages) {
        QVariantMap item;
        item.insert(QStringLiteral("name"), package.name);
        item.insert(QStringLiteral("version"), package.version);
        item.insert(QStringLiteral("latestBuildState"), package.latestBuildState);
        item.insert(QStringLiteral("availableChroots"), package.availableChroots);
        item.insert(QStringLiteral("isAvailableForCurrentFedora"), package.isAvailableForCurrentFedora);
        packages.append(item);
    }

    return packages;
}

QVariantMap CoprResource::coprDetails() const
{
    QVariantMap details;
    const QStringList allChroots = mergedChroots(m_availableChroots, m_projectPackages);

    details.insert(QStringLiteral("repository"), m_projectFullName.isEmpty() ? QStringLiteral("%1/%2").arg(m_owner, m_project) : m_projectFullName);
    details.insert(QStringLiteral("owner"), m_owner);
    details.insert(QStringLiteral("project"), m_project);
    details.insert(QStringLiteral("packageName"), m_installPackageName);
    details.insert(QStringLiteral("latestVersion"), m_version);
    details.insert(QStringLiteral("latestBuildState"), m_latestBuildState);
    details.insert(QStringLiteral("buildRepositoryUrl"), m_latestBuildRepoUrl);
    details.insert(QStringLiteral("buildSubmitter"), m_latestBuildSubmitter);
    details.insert(QStringLiteral("buildSubmittedOn"), formatBuildDateTime(m_latestBuildSubmittedOn));
    details.insert(QStringLiteral("buildStartedOn"), formatBuildDateTime(m_latestBuildStartedOn));
    details.insert(QStringLiteral("buildFinishedOn"), formatBuildDateTime(m_latestBuildEndedOn));
    details.insert(QStringLiteral("availableFor"), formattedChrootsSummary(allChroots));
    details.insert(QStringLiteral("availableChroots"), allChroots);

    if (allChroots.isEmpty()) {
        details.insert(QStringLiteral("currentFedoraState"), QStringLiteral("unknown"));
        details.insert(QStringLiteral("currentFedoraText"), i18n("Availability for your Fedora version is unknown."));
    } else if (m_isAvailableForCurrentFedora) {
        details.insert(QStringLiteral("currentFedoraState"), QStringLiteral("available"));
        details.insert(QStringLiteral("currentFedoraText"), i18n("Available for your Fedora version."));
    } else {
        details.insert(QStringLiteral("currentFedoraState"), QStringLiteral("unavailable"));
        details.insert(QStringLiteral("currentFedoraText"), i18n("Not available for your Fedora version."));
    }

    details.insert(QStringLiteral("sourceType"), m_sourceType);
    details.insert(QStringLiteral("sourceUrl"), m_sourceUrl);
    details.insert(QStringLiteral("sourceSpec"), m_sourceSpec);
    details.insert(QStringLiteral("sourceSubdirectory"), m_sourceSubdirectory);
    details.insert(QStringLiteral("contact"), m_contact);
    details.insert(QStringLiteral("instructions"), m_instructions);
    details.insert(QStringLiteral("additionalRepos"), m_additionalRepos);
    details.insert(QStringLiteral("repoPriority"), m_repoPriority);
    details.insert(QStringLiteral("appstream"), m_appstream);
    details.insert(QStringLiteral("develMode"), m_develMode);
    details.insert(QStringLiteral("enableNet"), m_enableNet);
    details.insert(QStringLiteral("followFedoraBranching"), m_followFedoraBranching);
    details.insert(QStringLiteral("autoPrune"), m_autoPrune);
    details.insert(QStringLiteral("moduleHotfixes"), m_moduleHotfixes);

    return details;
}

QVariantList CoprResource::coprWarnings() const
{
    QVariantList warnings;
    const QStringList allChroots = mergedChroots(m_availableChroots, m_projectPackages);

    auto addWarning = [&warnings](const QString &text) {
        QVariantMap warning;
        warning.insert(QStringLiteral("text"), text);
        warnings.append(warning);
    };

    if (!allChroots.isEmpty() && !m_isAvailableForCurrentFedora) {
        addWarning(i18n("This COPR package is not available for your Fedora version."));
    }
    if (m_develMode) {
        addWarning(i18n("This COPR project is in development mode."));
    }
    if (!m_additionalRepos.isEmpty()) {
        addWarning(i18n("This project uses additional repositories: %1", m_additionalRepos.join(QStringLiteral(", "))));
    }
    if (m_enableNet) {
        addWarning(i18n("Network access is enabled during builds."));
    }
    if (!m_repoPriority.isEmpty()) {
        addWarning(i18n("Repository priority: %1", m_repoPriority));
    }
    if (m_moduleHotfixes) {
        addWarning(i18n("Module hotfixes are enabled for this repository."));
    }

    return warnings;
}

AbstractResource::State CoprResource::state()
{
    if (m_isInstalled) {
        return AbstractResource::Installed;
    }
    if (m_installPackageName.isEmpty() && m_isProjectResource) {
        return AbstractResource::Broken;
    }

    return AbstractResource::None;
}

void CoprResource::setState(AbstractResource::State state)
{
    setInstalledStateFromSystem(state == AbstractResource::Installed);
}

void CoprResource::setInstalledStateFromSystem(bool installed)
{
    const bool wasInstalled = m_isInstalled;
    m_isInstalled = installed;

    if (auto pkBackend = qobject_cast<PackageKitBackend *>(backend())) {
        pkBackend->setCoprInstalledStateCache(m_owner, m_installPackageName, installed);
    }

    // Also emit the change through the backend so the UI updates
    if (wasInstalled != m_isInstalled) {
        Q_EMIT stateChanged();
    }
    if (backend() && wasInstalled != m_isInstalled) {
        Q_EMIT backend()->resourcesChanged(this, {"state"});
    }
}

void CoprResource::setProjectPackages(const QList<CoprPackageInfo> &packages)
{
    const AbstractResource::State previousState = state();
    const QString previousInstallPackageName = m_installPackageName;

    m_projectPackages = packages;
    m_projectPackagesLoaded = true;

    if (m_isProjectResource) {
        if (const CoprPackageInfo *package = preferredProjectPackage()) {
            m_installPackageName = package->name;
            applyPackageDetails(*package);
        } else {
            m_installPackageName.clear();
        }
    }

    if (m_installPackageName.isEmpty()) {
        m_availableChroots = mergedChroots(m_availableChroots, m_projectPackages);
        m_isAvailableForCurrentFedora = std::any_of(m_availableChroots.cbegin(), m_availableChroots.cend(), [this](const QString &chroot) {
            if (auto pkBackend = qobject_cast<PackageKitBackend *>(backend())) {
                if (auto client = pkBackend->coprClient()) {
                    return chroot == client->getCurrentChroot();
                }
            }
            return false;
        });
    }

    Q_EMIT longDescriptionChanged();
    Q_EMIT versionsChanged();
    Q_EMIT projectPackagesChanged();
    if (previousState != state() || previousInstallPackageName != m_installPackageName) {
        Q_EMIT stateChanged();
    }
    if (previousInstallPackageName != m_installPackageName && !m_installPackageName.isEmpty()) {
        QMetaObject::invokeMethod(this, &CoprResource::checkInstalledState, Qt::QueuedConnection);
    }
}

void CoprResource::selectCoprProjectPackage(const QString &packageName)
{
    if (!m_isProjectResource || packageName.isEmpty()) {
        return;
    }

    const auto it = std::find_if(m_projectPackages.cbegin(), m_projectPackages.cend(), [&packageName](const CoprPackageInfo &package) {
        return package.name == packageName;
    });
    if (it == m_projectPackages.cend()) {
        return;
    }

    const AbstractResource::State previousState = state();
    const QString previousInstallPackageName = m_installPackageName;

    m_installPackageName = it->name;
    applyPackageDetails(*it);

    Q_EMIT longDescriptionChanged();
    Q_EMIT versionsChanged();
    Q_EMIT projectPackagesChanged();
    if (previousState != state() || previousInstallPackageName != m_installPackageName) {
        Q_EMIT stateChanged();
    }
    if (previousInstallPackageName != m_installPackageName) {
        QMetaObject::invokeMethod(this, &CoprResource::checkInstalledState, Qt::QueuedConnection);
    }
}

const CoprPackageInfo *CoprResource::preferredProjectPackage() const
{
    if (m_projectPackages.isEmpty()) {
        return nullptr;
    }
    if (m_projectPackages.size() == 1) {
        return &m_projectPackages.constFirst();
    }

    const auto it = std::find_if(m_projectPackages.cbegin(), m_projectPackages.cend(), [this](const CoprPackageInfo &package) {
        return package.name.compare(m_project, Qt::CaseInsensitive) == 0;
    });
    return it == m_projectPackages.cend() ? nullptr : &(*it);
}

void CoprResource::applyPackageDetails(const CoprPackageInfo &package)
{
    m_version = package.version;
    m_latestBuildState = package.latestBuildState;
    m_latestBuildRepoUrl = package.latestBuildRepoUrl;
    m_latestBuildSubmitter = package.latestBuildSubmitter;
    m_latestBuildSubmittedOn = package.latestBuildSubmittedOn;
    m_latestBuildStartedOn = package.latestBuildStartedOn;
    m_latestBuildEndedOn = package.latestBuildEndedOn;
    m_sourceType = package.sourceType;
    m_sourceUrl = package.sourceUrl;
    m_sourceSpec = package.sourceSpec;
    m_sourceSubdirectory = package.sourceSubdirectory;

    if (!package.availableChroots.isEmpty()) {
        m_availableChroots = package.availableChroots;
    }
    m_isAvailableForCurrentFedora = package.isAvailableForCurrentFedora;
}

QVariant CoprResource::icon() const
{
    return QStringLiteral("package");
}

QString CoprResource::sizeDescription()
{
    return {};
}

QString CoprResource::author() const
{
    // Return the COPR owner as the author
    return m_owner;
}

QString CoprResource::sourceIcon() const
{
    return QStringLiteral("cloud-upload");
}

QDate CoprResource::releaseDate() const
{
    if (m_latestBuildEndedOn.isValid()) {
        return m_latestBuildEndedOn.date();
    }
    if (m_latestBuildSubmittedOn.isValid()) {
        return m_latestBuildSubmittedOn.date();
    }
    return {};
}

void CoprResource::checkInstalledState()
{
    if (m_installPackageName.isEmpty()) {
        setInstalledStateFromSystem(false);
        return;
    }

    if (auto pkBackend = qobject_cast<PackageKitBackend *>(backend())) {
        pkBackend->requestCoprInstalledStateCheck(this);
    } else {
        setInstalledStateFromSystem(false);
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
