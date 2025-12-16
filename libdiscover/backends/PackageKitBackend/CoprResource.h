#ifndef COPRRESOURCE_H
#define COPRRESOURCE_H

#include "PackageKitResource.h"
#include "CoprClient.h"

class PackageKitBackend;

class CoprResource : public PackageKitResource
{
    Q_OBJECT

public:
    explicit CoprResource(const CoprPackageInfo &packageInfo, AbstractResourcesBackend *parent);
    ~CoprResource() override;

    QString section() override;
    QString origin() const override;
    QString comment() override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QUrl homepage() override;
    QString author() const override;
    QString sourceIcon() const override;

    AbstractResource::State state() override;
    QVariant icon() const override;
    QString sizeDescription() override;

    QString coprOwner() const { return m_owner; }
    QString coprProject() const { return m_project; }
    QStringList availableChroots() const { return m_availableChroots; }
    bool isAvailableForCurrentFedora() const { return m_isAvailableForCurrentFedora; }

    void setAvailableForCurrentFedora(bool available) { m_isAvailableForCurrentFedora = available; }
    void setState(AbstractResource::State state);
    void checkInstalledState();

    bool canExecute() const override;
    void invokeApplication() const override;

private:
    QString findDesktopFile() const;

    QString m_owner;
    QString m_project;
    QString m_description;
    QStringList m_availableChroots;
    bool m_isAvailableForCurrentFedora;
    QString m_homepage;
    bool m_isInstalled = false;
};

#endif // COPRRESOURCE_H
