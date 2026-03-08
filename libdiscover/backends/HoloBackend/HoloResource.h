/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef HOLORESOURCE_H
#define HOLORESOURCE_H

#include <resources/AbstractResource.h>

class AddonList;
class HoloResource : public AbstractResource
{
    Q_OBJECT
public:
    explicit HoloResource(const QString &version,
                          const QString &name,
                          const QString &build,
                          quint64 size,
                          const QString &currentVersion,
                          AbstractResourcesBackend *parent);

    QString appstreamId() const override;
    QList<PackageState> addonsInformation() override;
    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QJsonArray licenses() override;
    quint64 size() override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QUrl contributeURL() override;
    bool hasCategory(const QString &category) const override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    bool isRemovable() const override;
    AbstractResource::Type type() const override;
    bool canExecute() const override;
    void invokeApplication() const override { };
    void fetchChangelog() override;
    QUrl url() const override;
    QString author() const override;
    void setState(State state);
    void setSize(quint64 size);

    QString sourceIcon() const override;
    QDate releaseDate() const override;

    void setVersion(const QString &version);
    void setBuild(const QString &build);
    QString getBuild() const;

public:
    const QString m_name;
    QString m_build;
    QString m_version;
    QString m_currentVersion;
    QString m_appstreamId;
    AbstractResource::State m_state;
    QList<PackageState> m_addons;
    const AbstractResource::Type m_type;
    quint64 m_size;
};

#endif // HOLORESOURCE_H
