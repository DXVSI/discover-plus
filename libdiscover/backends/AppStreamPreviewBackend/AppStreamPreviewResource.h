/*
 *   SPDX-FileCopyrightText: 2025 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <resources/AbstractResource.h>

#include <AppStreamQt/component.h>

class DISCOVERCOMMON_EXPORT AppStreamPreviewResource : public AbstractResource
{
    Q_OBJECT
public:
    explicit AppStreamPreviewResource(const AppStream::Component &component, AbstractResourcesBackend *parent);

    QString packageName() const override;
    QString name() const override;
    QString comment() override;
    QVariant icon() const override;
    bool canExecute() const override;
    void invokeApplication() const override;
    AbstractResource::State state() override;
    bool hasCategory(const QString &category) const override;
    AbstractResource::Type type() const override;
    quint64 size() override;
    QJsonArray licenses() override;
    QString installedVersion() const override;
    QString availableVersion() const override;
    QString longDescription() override;
    QString origin() const override;
    QString section() override;
    QString author() const override;
    QList<PackageState> addonsInformation() override;
    QString sourceIcon() const override;
    QDate releaseDate() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QUrl contributeURL() override;
    QString versionString() override;
    QString contentRatingDescription() const override;
    uint contentRatingMinimumAge() const override;
    bool isRemovable() const override;
    QString appstreamId() const override;
    QUrl url() const override;
    QStringList topObjects() const override;

private:
    static const QStringList s_topObjects;
    const AppStream::Component m_component;
};
