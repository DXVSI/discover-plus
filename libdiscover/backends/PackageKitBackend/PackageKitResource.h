/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KService>
#include <PackageKit/Details>
#include <PackageKit/Transaction>
#include <resources/AbstractResource.h>

#include <optional>

#include "PackageKitDependencies.h"

class PackageKitBackend;

class PackageKitResource : public AbstractResource
{
    Q_OBJECT

    Q_PROPERTY(QList<PackageKitDependency> dependencies READ dependencies NOTIFY dependenciesChanged)

public:
    explicit PackageKitResource(QString packageName, QString summary, PackageKitBackend *parent);
    QString packageName() const override;
    QString name() const override;
    QString comment() override;
    QString longDescription() override;
    QUrl homepage() override;
    QVariant icon() const override;
    bool hasCategory(const QString &category) const override;
    QJsonArray licenses() override;
    QString origin() const override;
    QString section() override;
    AbstractResource::Type type() const override;
    quint64 size() override;
    void fetchChangelog() override;
    void fetchUpdateDetails() override;

    QList<PackageState> addonsInformation() override;
    State state() override;

    QString installedVersion() const override;
    QString availableVersion() const override;
    QString author() const override
    {
        return {};
    }
    virtual QStringList allPackageNames() const;
    QString installedPackageId() const;
    QString availablePackageId() const;

    void clearPackageIds()
    {
        m_packages.clear();
    }

    PackageKitBackend *backend() const;

    static QString joinPackages(const QStringList &pkgids, const QString &_sep, const QString &shadowPackageName);

    /**
     * Critical packages are those that might render an installation unusable if removed
     */
    virtual bool isCritical() const;

    QString verifiedMessage() const override;

    void invokeApplication() const override
    {
    }
    bool canExecute() const override
    {
        return false;
    }

    QList<PackageKitDependency> dependencies();

    QString sourceIcon() const override;

    QDate releaseDate() const override
    {
        return {};
    }

    QStringList topObjects() const override;

    QStringList bottomObjects() const override;

    virtual QString changelog() const
    {
        return m_changelog;
    }

    bool extendsItself() const;

    void runService(KService::Ptr service) const;
    bool containsPackageId(const QString &pkgid) const;

Q_SIGNALS:
    void dependenciesChanged();

public Q_SLOTS:
    void addPackageId(PackageKit::Transaction::Info info, const QString &packageId, bool arch);
    void setDetails(const PackageKit::Details &details);

    void updateDetail(const QString &packageID,
                      const QStringList &updates,
                      const QStringList &obsoletes,
                      const QStringList &vendorUrls,
                      const QStringList &bugzillaUrls,
                      const QStringList &cveUrls,
                      PackageKit::Transaction::Restart restart,
                      const QString &updateText,
                      const QString &changelog,
                      PackageKit::Transaction::UpdateState state,
                      const QDateTime &issued,
                      const QDateTime &updated);

    void failedFetchingDetails(PackageKit::Transaction::Error, const QString &msg);

protected:
    PackageKit::Details m_details;

private:
    /** fetches details individually, it's better if done in batch, like for updates */
    virtual void fetchDetails();

    struct Ids {
        QVector<QString> archPkgIds;
        QVector<QString> nonarchPkgIds;

        QString first() const
        {
            return !archPkgIds.isEmpty() ? archPkgIds.first() : nonarchPkgIds.first();
        }

        bool isEmpty() const
        {
            return archPkgIds.isEmpty() && nonarchPkgIds.isEmpty();
        }
    };
    QMap<PackageKit::Transaction::Info, Ids> m_packages;
    const QString m_summary;
    const QString m_name;
    QString m_changelog;
    PackageKitDependencies m_dependencies;
    static const QStringList s_topObjects;
    static const QStringList s_bottomObjects;
};
