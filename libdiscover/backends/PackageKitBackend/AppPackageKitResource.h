/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "PackageKitBackend.h"
#include "PackageKitResource.h"
#include <QIcon>

class AppPackageKitResource : public PackageKitResource
{
    Q_OBJECT
public:
    explicit AppPackageKitResource(const AppStream::Component &data, const QString &packageName, PackageKitBackend *parent);

    QString appstreamId() const override;

    AbstractResource::Type type() const override;
    QString name() const override;
    QVariant icon() const override;
    QStringList mimetypes() const override;
    bool hasCategory(const QString &category) const override;
    QString longDescription() override;
    QUrl url() const override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QUrl contributeURL() override;
    QUrl faqURL() override;
    QUrl translateURL() override;
    QUrl contactURL() override;
    QUrl vcsBrowserURL() override;
    QString comment() override;
    QJsonArray licenses() override;
    QStringList allPackageNames() const override;
    QList<PackageState> addonsInformation() override;
    QStringList extends() const override;
    void fetchScreenshots() override;
    void invokeApplication() const override;
    bool canExecute() const override;
    QDate releaseDate() const override;
    QString changelog() const override;
    QString author() const override;
    QString versionString() override;
    void fetchChangelog() override;
    QSet<QString> alternativeAppstreamIds() const override;
    bool isCritical() const override;

    QString contentRatingDescription() const override;
    uint contentRatingMinimumAge() const override;
    [[nodiscard]] bool hasResolvedIcon() const override;
    void resolveIcon() override;

private:
    const AppStream::Component m_appdata;
    mutable QString m_name;
    std::optional<QIcon> m_icon;
};
