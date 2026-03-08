/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "SystemdSysupdateTransaction.h"

#include <optional>
#include <qobject.h>
#include <resources/AbstractResource.h>

#include "SysupdateInternal.h"
#include "sysupdate1.h"

#include <AppStreamQt/component.h>

class SystemdSysupdateResource : public AbstractResource
{
    Q_OBJECT
public:
    SystemdSysupdateResource(AbstractResourcesBackend *parent,
                             const AppStream::Component &component,
                             const Sysupdate::TargetInfo &targetInfo,
                             org::freedesktop::sysupdate1::Target *target);
    QString packageName() const override;
    QString name() const override;
    QString comment() override;
    QVariant icon() const override;
    bool canExecute() const override;
    bool isRemovable() const override;
    void invokeApplication() const override;
    State state() override;
    bool hasCategory(const QString &category) const override;
    Type type() const override;
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

    SystemdSysupdateTransaction *update();

private:
    AppStream::Component m_component;
    Sysupdate::TargetInfo m_targetInfo;
    org::freedesktop::sysupdate1::Target *m_target;
    bool m_fetchedSize = false;
    quint64 m_size = 0;
};
