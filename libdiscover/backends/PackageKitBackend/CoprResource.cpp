#include "CoprResource.h"
#include "PackageKitBackend.h"

#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KService>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QLocale>
#include <QSet>
#include <QVariantMap>

#include <algorithm>

static QString htmlParagraphBreak()
{
    return QStringLiteral("<br><br>");
}

static QString htmlLabel(const QString &label)
{
    return QStringLiteral("<b>%1</b>").arg(label.toHtmlEscaped());
}

static void appendTextDetail(QString &html, const QString &label, const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    html += QStringLiteral("<br>");
    html += htmlLabel(label) + QStringLiteral(" ") + value.toHtmlEscaped();
}

static void appendLinkDetail(QString &html, const QString &label, const QString &url, const QString &text = {})
{
    if (url.isEmpty()) {
        return;
    }

    const QString linkText = text.isEmpty() ? url : text;
    html += QStringLiteral("<br>");
    html += htmlLabel(label) + QStringLiteral(" ");
    html += QStringLiteral("<a href=\"%1\">%2</a>").arg(url.toHtmlEscaped(), linkText.toHtmlEscaped());
}

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

static QString boolText(bool value)
{
    return value ? i18nc("@item", "yes") : i18nc("@item", "no");
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

static QString packageSummaryLine(const CoprPackageInfo &package)
{
    QString summary = package.name.toHtmlEscaped();
    if (!package.version.isEmpty()) {
        summary += QStringLiteral(" ") + QStringLiteral("(%1)").arg(package.version.toHtmlEscaped());
    }
    if (!package.latestBuildState.isEmpty()) {
        summary += QStringLiteral(" - ") + i18n("latest build: %1", package.latestBuildState.toHtmlEscaped());
    }

    const QString chroots = formattedChrootsSummary(package.availableChroots);
    if (!chroots.isEmpty()) {
        summary += QStringLiteral(" - ") + chroots.toHtmlEscaped();
    }

    return summary;
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
    , m_projectChroots(packageInfo.projectChroots)
    , m_availability(packageInfo.availability)
    , m_currentChrootState(packageInfo.currentChrootState)
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
    // A package of a search result comes from the monitor of its project
    if (!m_isProjectResource) {
        m_monitorPackages = {packageInfo};
        m_projectPackages = m_monitorPackages;
        m_monitorFetch = Loaded;
    }

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
    QString desc;

    if (!m_description.isEmpty()) {
        desc = m_description;
    } else {
        desc = i18n("Package from COPR repository %1/%2", m_owner, m_project);
    }

    desc += htmlParagraphBreak();
    desc += QStringLiteral("<hr>");
    desc += htmlParagraphBreak();
    desc += htmlLabel(i18n("COPR Details"));

    appendTextDetail(desc, i18n("Repository:"), m_projectFullName.isEmpty() ? QStringLiteral("%1/%2").arg(m_owner, m_project) : m_projectFullName);
    if (!m_isProjectResource) {
        appendTextDetail(desc, i18n("Package:"), packageName());
    } else if (m_projectPackages.isEmpty()) {
        appendTextDetail(desc, i18n("Install target:"), i18n("Unknown until package metadata is loaded"));
        if (m_monitorFetch == Requested || m_packageListFetch == Requested) {
            appendTextDetail(desc, i18n("Project packages:"), i18n("Loading or unavailable"));
        }
    } else {
        if (m_installPackageName.isEmpty()) {
            appendTextDetail(desc, i18n("Install target:"), i18n("Select a specific package result to install"));
        } else {
            appendTextDetail(desc, i18n("Install target:"), m_installPackageName);
        }
        desc += QStringLiteral("<br>");
        desc += htmlLabel(i18n("Packages in this project:"));
        desc += QStringLiteral("<ul>");
        for (const CoprPackageInfo &package : m_projectPackages) {
            desc += QStringLiteral("<li>") + packageSummaryLine(package) + QStringLiteral("</li>");
        }
        desc += QStringLiteral("</ul>");
    }
    appendTextDetail(desc, i18n("Latest version:"), m_version);
    appendTextDetail(desc, i18n("Latest build:"), m_latestBuildState);
    appendTextDetail(desc, i18n("Submitted:"), formatBuildDateTime(m_latestBuildSubmittedOn));
    appendTextDetail(desc, i18n("Started:"), formatBuildDateTime(m_latestBuildStartedOn));
    appendTextDetail(desc, i18n("Finished:"), formatBuildDateTime(m_latestBuildEndedOn));
    appendTextDetail(desc, i18n("Submitter:"), m_latestBuildSubmitter);
    appendLinkDetail(desc, i18n("Build repository:"), m_latestBuildRepoUrl);

    // Until a package is selected and the monitor has spoken, what the project enables
    const bool packageChrootsKnown = !m_installPackageName.isEmpty() && m_availability != CoprAvailability::Unknown;
    const QStringList allChroots = mergedChroots(packageChrootsKnown ? m_availableChroots : m_projectChroots, m_projectPackages);
    const QString chrootsSummary = formattedChrootsSummary(allChroots);
    appendTextDetail(desc, i18n("Available for:"), chrootsSummary);

    desc += htmlParagraphBreak();
    const QString installWarning = coprInstallWarning();
    if (!isCurrentChrootKnown() || (m_projectChroots.isEmpty() && m_availability == CoprAvailability::Unknown)) {
        desc += QStringLiteral("<span style='font-weight: bold;'>");
        desc += i18n("Availability for your Fedora version is unknown.");
        desc += QStringLiteral("</span>");
    } else if (!installWarning.isEmpty()) {
        desc += QStringLiteral("<span style='color: #ff8800; font-weight: bold;'>");
        desc += installWarning.toHtmlEscaped();
        desc += QStringLiteral("</span>");
    } else if (!isInstallBlocked()) {
        desc += QStringLiteral("<span style='color: green; font-weight: bold;'>");
        desc += i18n("Available for your Fedora version.");
        desc += QStringLiteral("</span>");
    } else {
        desc += QStringLiteral("<span style='color: red; font-weight: bold;'>");
        desc += i18n("Not available for your Fedora version.");
        desc += QStringLiteral("</span>");
    }

    if (!m_sourceType.isEmpty() || !m_sourceUrl.isEmpty() || !m_sourceSpec.isEmpty() || !m_sourceSubdirectory.isEmpty()) {
        desc += htmlParagraphBreak();
        desc += htmlLabel(i18n("Source"));
        appendTextDetail(desc, i18n("Type:"), m_sourceType);
        appendLinkDetail(desc, i18n("URL:"), m_sourceUrl);
        appendTextDetail(desc, i18n("Spec:"), m_sourceSpec);
        appendTextDetail(desc, i18n("Subdirectory:"), m_sourceSubdirectory);
    }

    if (!m_contact.isEmpty() || !m_instructions.isEmpty()) {
        desc += htmlParagraphBreak();
        desc += htmlLabel(i18n("Project Information"));
        appendTextDetail(desc, i18n("Contact:"), m_contact);
        if (!m_instructions.isEmpty()) {
            desc += htmlParagraphBreak();
            desc += htmlLabel(i18n("Instructions"));
            desc += QStringLiteral("<br>");
            desc += m_instructions;
        }
    }

    desc += htmlParagraphBreak();
    desc += QStringLiteral("<span style='color: #ff8800;'><b>");
    desc += i18n("Notice:");
    desc += QStringLiteral("</b> ");
    desc += i18n("COPR repositories are not officially supported by Fedora. Use at your own risk.");
    desc += QStringLiteral("</span>");

    QStringList warnings;
    if (m_develMode) {
        warnings.append(i18n("This COPR project is in development mode."));
    }
    if (!m_additionalRepos.isEmpty()) {
        warnings.append(i18n("This project uses additional repositories: %1", m_additionalRepos.join(QStringLiteral(", "))));
    }
    if (m_enableNet) {
        warnings.append(i18n("Network access is enabled during builds."));
    }
    if (!m_repoPriority.isEmpty()) {
        warnings.append(i18n("Repository priority: %1", m_repoPriority));
    }
    if (m_moduleHotfixes) {
        warnings.append(i18n("Module hotfixes are enabled for this repository."));
    }

    if (!warnings.isEmpty()) {
        desc += htmlParagraphBreak();
        desc += QStringLiteral("<span style='color: #ff8800;'>");
        desc += htmlLabel(i18n("Additional warnings:"));
        desc += QStringLiteral("<ul>");
        for (const QString &warning : warnings) {
            desc += QStringLiteral("<li>") + warning.toHtmlEscaped() + QStringLiteral("</li>");
        }
        desc += QStringLiteral("</ul></span>");
    }

    desc += htmlParagraphBreak();
    desc += htmlLabel(i18n("Repository flags"));
    appendTextDetail(desc, i18n("AppStream metadata:"), boolText(m_appstream));
    appendTextDetail(desc, i18n("Follows Fedora branching:"), boolText(m_followFedoraBranching));
    appendTextDetail(desc, i18n("Auto-prune:"), boolText(m_autoPrune));

    return desc;
}

void CoprResource::fetchProjectPackages()
{
    auto pkBackend = qobject_cast<PackageKitBackend *>(backend());
    CoprClient *client = pkBackend ? pkBackend->coprClient() : nullptr;
    bool changed = false;

    // Without a client nothing is sent, so nothing may look requested
    if (m_packageListFetch == NotRequested || m_packageListFetch == Failed) {
        m_packageListFetch = client ? Requested : Failed;
        changed = true;
        if (client) {
            client->getProjectPackages(m_owner, m_project);
        }
    }
    // A monitor request of a list item is cancelled with its list: ask again so that
    // the client keeps it for this page
    if (m_isProjectResource && m_monitorFetch != Loaded && !m_monitorForOpenPage) {
        m_monitorFetch = client ? Requested : Failed;
        m_monitorForOpenPage = client;
        changed = true;
        if (client) {
            client->getProjectMonitor(m_owner, m_project, true);
        }
    }

    if (changed) {
        Q_EMIT projectPackagesChanged();
    }
}

bool CoprResource::fetchProjectMonitor()
{
    // Nothing can be installed from a project without the chroot of this system
    if (!m_isProjectResource || isProjectChrootMissing() || m_monitorFetch == Loaded) {
        return false;
    }
    if (m_monitorFetch == Requested) {
        return true;
    }

    auto pkBackend = qobject_cast<PackageKitBackend *>(backend());
    CoprClient *client = pkBackend ? pkBackend->coprClient() : nullptr;
    m_monitorFetch = client ? Requested : Failed;
    if (client) {
        client->getProjectMonitor(m_owner, m_project);
    }
    Q_EMIT projectPackagesChanged();
    return client;
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

    return CoprClient::projectWebUrl(m_owner, m_project);
}

QString CoprResource::currentChroot() const
{
    if (auto pkBackend = qobject_cast<PackageKitBackend *>(backend())) {
        if (auto client = pkBackend->coprClient()) {
            return client->getCurrentChroot();
        }
    }
    return {};
}

bool CoprResource::isCurrentChrootKnown() const
{
    return !currentChroot().isEmpty();
}

QStringList CoprResource::topObjects() const
{
    QStringList objects = PackageKitResource::topObjects();
    if (m_isProjectResource) {
        objects.append(QStringLiteral("qrc:/qml/CoprPackageSelector.qml"));
    }
    return objects;
}

QVariantList CoprResource::coprProjectPackages() const
{
    QVariantList packages;
    packages.reserve(m_projectPackages.size());
    const bool chrootKnown = isCurrentChrootKnown();

    for (const CoprPackageInfo &package : m_projectPackages) {
        QVariantMap item;
        item.insert(QStringLiteral("name"), package.name);
        item.insert(QStringLiteral("version"), package.version);
        item.insert(QStringLiteral("latestBuildState"), package.latestBuildState);
        item.insert(QStringLiteral("availableChroots"), package.availableChroots);
        item.insert(QStringLiteral("currentChrootState"), package.currentChrootState);
        item.insert(QStringLiteral("isAvailableForCurrentFedora"), package.availability == CoprAvailability::Available);
        // A last build that did not succeed is neither: an older build may still be published
        item.insert(QStringLiteral("isAvailabilityKnown"),
                    chrootKnown && (package.availability == CoprAvailability::Available || package.availability == CoprAvailability::NotAvailable));
        packages.append(item);
    }

    return packages;
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

bool CoprResource::isProjectChrootMissing() const
{
    return isCurrentChrootKnown() && !m_projectChroots.isEmpty() && !m_projectChroots.contains(currentChroot());
}

bool CoprResource::isInstallBlocked() const
{
    return isProjectChrootMissing() || (isCurrentChrootKnown() && m_availability == CoprAvailability::NotAvailable);
}

QString CoprResource::coprInstallWarning() const
{
    if (m_installPackageName.isEmpty() || isInstallBlocked() || m_availability != CoprAvailability::NotConfirmed) {
        return {};
    }
    return i18n("The last build of %1 for your Fedora version has the state \"%2\". An older build may still be installable.",
                m_installPackageName,
                m_currentChrootState);
}

QString CoprResource::coprInstallStatus() const
{
    if (!m_installPackageName.isEmpty()) {
        return isInstallBlocked() ? QStringLiteral("unavailable") : QStringLiteral("ready");
    }
    if (isProjectChrootMissing()) {
        return QStringLiteral("unavailable");
    }

    const bool loading = m_monitorFetch == Requested || m_packageListFetch == Requested;
    if (!coprProjectPackagesLoaded()) {
        if (loading) {
            return QStringLiteral("loading");
        }
        return m_monitorFetch == Failed || m_packageListFetch == Failed ? QStringLiteral("failed") : QStringLiteral("idle");
    }
    if (m_projectPackages.isEmpty()) {
        // The other source may still know packages
        return loading ? QStringLiteral("loading") : QStringLiteral("empty");
    }

    const bool anyInstallable = std::any_of(m_projectPackages.cbegin(), m_projectPackages.cend(), [](const CoprPackageInfo &package) {
        return package.availability != CoprAvailability::NotAvailable;
    });
    return anyInstallable || !isCurrentChrootKnown() ? QStringLiteral("needs-selection") : QStringLiteral("unavailable");
}

CoprResource::FetchState &CoprResource::fetchStateFor(const QString &requestType)
{
    return requestType == CoprClient::projectMonitorRequestType() ? m_monitorFetch : m_packageListFetch;
}

// A resource of a single package only keeps what is said about that package
static QList<CoprPackageInfo> packagesNamed(const QList<CoprPackageInfo> &packages, const QString &name)
{
    QList<CoprPackageInfo> named;
    for (const CoprPackageInfo &package : packages) {
        if (package.name == name) {
            named.append(package);
        }
    }
    return named;
}

void CoprResource::setProjectPackages(const QList<CoprPackageInfo> &packages, bool complete)
{
    m_listedPackages = m_isProjectResource ? packages : packagesNamed(packages, m_installPackageName);
    m_packageListComplete = complete;
    m_packageListFetch = Loaded;
    projectPackagesUpdated();
}

void CoprResource::setProjectMonitor(const QList<CoprPackageInfo> &packages, bool complete)
{
    m_monitorPackages = m_isProjectResource ? packages : packagesNamed(packages, m_installPackageName);
    m_monitorComplete = complete;
    m_monitorFetch = Loaded;
    m_monitorForOpenPage = false;
    projectPackagesUpdated();
}

void CoprResource::projectRequestFailed(const QString &requestType)
{
    // What was loaded before stays; without it the request can be repeated
    FetchState &fetchState = fetchStateFor(requestType);
    if (fetchState != Requested) {
        return;
    }
    fetchState = Failed;
    if (requestType == CoprClient::projectMonitorRequestType()) {
        m_monitorForOpenPage = false;
    }
    Q_EMIT projectPackagesChanged();
}

void CoprResource::projectRequestCancelled(const QString &requestType)
{
    // What an open page asked for is not cancelled with the list: this is about
    // an earlier request of the list item for the same data
    FetchState &fetchState = fetchStateFor(requestType);
    if (fetchState != Requested || (requestType == CoprClient::projectMonitorRequestType() && m_monitorForOpenPage)) {
        return;
    }
    fetchState = NotRequested;
    Q_EMIT projectPackagesChanged();
}

void CoprResource::mergeProjectPackages()
{
    // The package list has the details. The monitor has the availability and, for
    // what it confirms, the version that is published in the chroot of this system.
    QList<CoprPackageInfo> merged = m_listedPackages;
    QHash<QString, int> indexByName;
    for (int i = 0; i < merged.size(); ++i) {
        indexByName.insert(merged.at(i).name, i);
    }

    QSet<QString> monitored;
    for (const CoprPackageInfo &monitorPackage : std::as_const(m_monitorPackages)) {
        monitored.insert(monitorPackage.name);
        const auto it = indexByName.constFind(monitorPackage.name);
        if (it == indexByName.constEnd()) {
            indexByName.insert(monitorPackage.name, merged.size());
            merged.append(monitorPackage);
            continue;
        }

        CoprPackageInfo &package = merged[*it];
        package.availableChroots = monitorPackage.availableChroots;
        package.availability = monitorPackage.availability;
        package.currentChrootState = monitorPackage.currentChrootState;
        if (package.version.isEmpty() || (monitorPackage.availability == CoprAvailability::Available && !monitorPackage.version.isEmpty())) {
            package.version = monitorPackage.version;
        }
    }

    // The monitor leaves out the packages that were never built
    if (m_monitorFetch == Loaded && m_monitorComplete && isCurrentChrootKnown()) {
        for (CoprPackageInfo &package : merged) {
            if (!monitored.contains(package.name)) {
                package.availability = CoprAvailability::NotAvailable;
            }
        }
    }

    std::stable_sort(merged.begin(), merged.end(), [](const CoprPackageInfo &a, const CoprPackageInfo &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    m_projectPackages = merged;
}

void CoprResource::projectPackagesUpdated()
{
    const AbstractResource::State previousState = state();
    const QString previousInstallPackageName = m_installPackageName;

    mergeProjectPackages();

    // What the user selected stays selected for as long as that package is listed:
    // the same list is delivered again by a cache hit, a search or the second source
    const auto selected = std::find_if(m_projectPackages.cbegin(), m_projectPackages.cend(), [this](const CoprPackageInfo &package) {
        return !m_installPackageName.isEmpty() && package.name == m_installPackageName;
    });
    if (selected != m_projectPackages.cend()) {
        applyPackageDetails(*selected);
    } else if (m_isProjectResource) {
        if (const CoprPackageInfo *package = preferredProjectPackage()) {
            m_installPackageName = package->name;
            applyPackageDetails(*package);
        } else {
            m_installPackageName.clear();
            m_availableChroots.clear();
            m_availability = CoprAvailability::Unknown;
            m_currentChrootState.clear();
        }
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
    // Only what may be installable here: whichever source answers first, the choice is the same
    QList<const CoprPackageInfo *> candidates;
    for (const CoprPackageInfo &package : m_projectPackages) {
        if (!isCurrentChrootKnown() || package.availability != CoprAvailability::NotAvailable) {
            candidates.append(&package);
        }
    }

    // The only package, unless the list was cut short
    const bool complete = (m_packageListFetch != Loaded || m_packageListComplete) && (m_monitorFetch != Loaded || m_monitorComplete);
    if (candidates.size() == 1 && complete) {
        return candidates.constFirst();
    }

    // The package named like the project
    const auto it = std::find_if(candidates.cbegin(), candidates.cend(), [this](const CoprPackageInfo *package) {
        return package->name.compare(m_project, Qt::CaseInsensitive) == 0;
    });
    return it == candidates.cend() ? nullptr : *it;
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

    // Of this package only: what the project enables stays in m_projectChroots
    m_availableChroots = package.availableChroots;
    m_availability = package.availability;
    m_currentChrootState = package.currentChrootState;
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
