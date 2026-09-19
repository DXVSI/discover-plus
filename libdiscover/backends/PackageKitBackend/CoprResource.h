/*
 *   SPDX-FileCopyrightText: 2025-2026 DXVSI <https://github.com/DXVSI>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COPRRESOURCE_H
#define COPRRESOURCE_H

#include "PackageKitResource.h"
#include "CoprClient.h"

#include <QVariantList>

class PackageKitBackend;

class CoprResource : public PackageKitResource
{
    Q_OBJECT
    Q_PROPERTY(bool isCoprProjectResource READ isCoprProjectResource CONSTANT)
    Q_PROPERTY(bool coprProjectPackagesLoaded READ coprProjectPackagesLoaded NOTIFY projectPackagesChanged)
    // Not with projectPackagesChanged: selecting a package must not rebuild a list of hundreds of rows
    Q_PROPERTY(QVariantList coprProjectPackages READ coprProjectPackages NOTIFY projectPackageListChanged)
    Q_PROPERTY(QString selectedCoprPackageName READ selectedCoprPackageName NOTIFY projectPackagesChanged)
    // "idle", "loading", "failed", "empty", "needs-selection", "unavailable" or "ready".
    // A string, because this class is not registered as a QML type. state() cannot tell
    // these apart: Broken only guards against an install without a package name.
    Q_PROPERTY(QString coprInstallStatus READ coprInstallStatus NOTIFY projectPackagesChanged)
    // Not empty when the install is allowed although the monitor could not confirm a build
    Q_PROPERTY(QString coprInstallWarning READ coprInstallWarning NOTIFY projectPackagesChanged)
    // Not empty while a request for the packages has failed and can be repeated: what went wrong.
    // Also when the other source answered, that is with any status.
    Q_PROPERTY(QString coprInstallError READ coprInstallError NOTIFY projectPackagesChanged)
    // True when the project has more packages than were loaded: one that is not listed may still exist
    Q_PROPERTY(bool coprPackageListLimited READ isCoprPackageListLimited NOTIFY projectPackagesChanged)
    // Why a search lists this project although its name, owner and description do not show
    // the query. Empty otherwise. comment() is constant and cannot follow the monitor.
    Q_PROPERTY(QString coprSearchReason READ coprSearchReason NOTIFY projectPackagesChanged)

public:
    explicit CoprResource(const CoprPackageInfo &packageInfo, AbstractResourcesBackend *parent);
    ~CoprResource() override;

    QString section() override;
    QString origin() const override;
    QString packageName() const override;
    QStringList allPackageNames() const override;
    QString comment() override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QUrl homepage() override;
    QString author() const override;
    QString sourceIcon() const override;
    QDate releaseDate() const override;
    QStringList topObjects() const override;

    AbstractResource::State state() override;
    QVariant icon() const override;
    QString sizeDescription() override;

    QString coprOwner() const { return m_owner; }
    QString coprProject() const { return m_project; }
    QStringList availableChroots() const { return m_availableChroots; }
    QStringList projectChroots() const
    {
        return m_projectChroots;
    }
    // False when the chroot of this system could not be detected: availability is unknown then
    bool isCurrentChrootKnown() const;
    // True only for a known negative: the chroot of this system is known and either it is
    // not enabled in the project or the monitor has no build of the package for it
    bool isInstallBlocked() const;
    QString coprInstallStatus() const;
    QString coprInstallWarning() const;
    QString coprInstallError() const;
    bool isCoprPackageListLimited() const;
    QString coprSearchReason() const;

    // What the list was searched for (empty when browsing) and whether the name, the
    // owner or the description of the project shows it
    void setCoprSearchQuery(const QString &query, bool matchIsVisible);
    // The version is empty when the package is not installed from the repository of this project
    void setInstalledStateFromSystem(const QString &installedVersion);
    // What the client delivered; requestType is one of CoprClient::project*RequestType()
    void setProjectPackages(const QList<CoprPackageInfo> &packages, bool complete);
    void setProjectMonitor(const QList<CoprPackageInfo> &packages, bool complete);
    void projectRequestFailed(const QString &requestType, const QString &errorMessage);
    void projectRequestCancelled(const QString &requestType);
    bool isProjectMonitorLoaded() const
    {
        return m_monitorFetch == Loaded;
    }
    // For an open application page: the monitor and the detailed package list
    Q_INVOKABLE void fetchProjectPackages();
    // For a list item: the monitor only, and nothing when the project lacks the chroot
    // of this system. Returns whether an answer is on its way.
    Q_INVOKABLE bool fetchProjectMonitor();
    // The same for a list item that merely became visible: only when nothing was asked
    // yet (never again after a failure), and the client may refuse or drop it
    Q_INVOKABLE bool fetchProjectMonitorLazily();
    // That list item went away: takes the request back while the client still queues it
    Q_INVOKABLE void dropLazyProjectMonitor();
    Q_INVOKABLE void selectCoprProjectPackage(const QString &packageName);
    void checkInstalledState();

    bool canExecute() const override;
    void invokeApplication() const override;

    bool isCoprProjectResource() const
    {
        return m_isProjectResource;
    }
    bool coprProjectPackagesLoaded() const
    {
        return m_monitorFetch == Loaded || m_packageListFetch == Loaded;
    }
    QVariantList coprProjectPackages() const;
    QString selectedCoprPackageName() const
    {
        return m_installPackageName;
    }

Q_SIGNALS:
    void projectPackagesChanged();
    void projectPackageListChanged();

private:
    QString findDesktopFile() const;
    QString currentChroot() const;
    const CoprPackageInfo *preferredProjectPackage() const;
    void applyPackageDetails(const CoprPackageInfo &package);
    bool isProjectChrootMissing() const;
    bool requestProjectMonitor(bool lazy);
    void mergeProjectPackages();
    void projectPackagesUpdated();

    enum FetchState {
        NotRequested,
        Requested,
        Loaded,
        Failed,
    };
    FetchState &fetchStateFor(const QString &requestType);

    QString m_owner;
    QString m_project;
    QString m_installPackageName;
    QString m_projectFullName;
    QString m_description;
    QString m_version;
    // Chroots with an installable build of the selected package
    QStringList m_availableChroots;
    // Chroots that are enabled in the project
    QStringList m_projectChroots;
    CoprAvailability m_availability = CoprAvailability::Unknown;
    QString m_currentChrootState;
    QString m_homepage;
    QString m_instructions;
    QString m_contact;
    QStringList m_additionalRepos;
    QString m_repoPriority;
    bool m_appstream = false;
    bool m_develMode = false;
    bool m_enableNet = false;
    bool m_followFedoraBranching = false;
    bool m_autoPrune = false;
    bool m_moduleHotfixes = false;
    bool m_isProjectResource = false;
    QString m_sourceType;
    QString m_sourceUrl;
    QString m_sourceSpec;
    QString m_sourceSubdirectory;
    QString m_latestBuildState;
    QString m_latestBuildRepoUrl;
    QString m_latestBuildSubmitter;
    QDateTime m_latestBuildSubmittedOn;
    QDateTime m_latestBuildStartedOn;
    QDateTime m_latestBuildEndedOn;
    // m_projectPackages is what the two sources say together
    QList<CoprPackageInfo> m_projectPackages;
    QList<CoprPackageInfo> m_listedPackages;
    QList<CoprPackageInfo> m_monitorPackages;
    FetchState m_packageListFetch = NotRequested;
    FetchState m_monitorFetch = NotRequested;
    bool m_monitorForOpenPage = false;
    QString m_requestError;
    // An automatic selection is made again whenever a source answers, this one is kept
    bool m_packageSelectedByUser = false;
    // False when the project has more packages than the client keeps
    bool m_packageListComplete = true;
    bool m_monitorComplete = true;
    bool m_isInstalled = false;
    QString m_installedVersion;
    QString m_searchQuery;
    bool m_searchMatchIsVisible = true;
};

#endif // COPRRESOURCE_H
