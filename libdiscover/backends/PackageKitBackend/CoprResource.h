#ifndef COPRRESOURCE_H
#define COPRRESOURCE_H

#include "CoprClient.h"
#include "PackageKitResource.h"

#include <QVariantList>
#include <QVariantMap>

class PackageKitBackend;

class CoprResource : public PackageKitResource
{
    Q_OBJECT
    Q_PROPERTY(bool isCoprProjectResource READ isCoprProjectResource CONSTANT)
    Q_PROPERTY(bool coprProjectPackagesLoaded READ coprProjectPackagesLoaded NOTIFY projectPackagesChanged)
    Q_PROPERTY(QVariantList coprProjectPackages READ coprProjectPackages NOTIFY projectPackagesChanged)
    Q_PROPERTY(QString selectedCoprPackageName READ selectedCoprPackageName NOTIFY projectPackagesChanged)
    Q_PROPERTY(QVariantMap coprDetails READ coprDetails NOTIFY projectPackagesChanged)
    Q_PROPERTY(QVariantList coprWarnings READ coprWarnings NOTIFY projectPackagesChanged)

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
    bool isAvailableForCurrentFedora() const { return m_isAvailableForCurrentFedora; }

    void setAvailableForCurrentFedora(bool available) { m_isAvailableForCurrentFedora = available; }
    void setState(AbstractResource::State state);
    void setInstalledStateFromSystem(bool installed);
    void setProjectPackages(const QList<CoprPackageInfo> &packages);
    Q_INVOKABLE void fetchProjectPackages();
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
        return m_projectPackagesLoaded;
    }
    QVariantList coprProjectPackages() const;
    QVariantMap coprDetails() const;
    QVariantList coprWarnings() const;
    QString selectedCoprPackageName() const
    {
        return m_installPackageName;
    }

Q_SIGNALS:
    void projectPackagesChanged();

private:
    QString findDesktopFile() const;
    const CoprPackageInfo *preferredProjectPackage() const;
    void applyPackageDetails(const CoprPackageInfo &package);

    QString m_owner;
    QString m_project;
    QString m_installPackageName;
    QString m_projectFullName;
    QString m_description;
    QString m_version;
    QStringList m_availableChroots;
    bool m_isAvailableForCurrentFedora;
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
    QList<CoprPackageInfo> m_projectPackages;
    bool m_projectPackagesRequested = false;
    bool m_projectPackagesLoaded = false;
    bool m_isInstalled = false;
};

#endif // COPRRESOURCE_H
