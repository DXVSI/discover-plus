/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QStack>
#include <QStandardItemModel>
#include <functional>
#include <resources/AbstractSourcesBackend.h>

#include "flatpak-helper.h"

class DiscoverAction;
class FlatpakResource;
class FlatpakSourcesBackend : public AbstractSourcesBackend
{
    Q_OBJECT
public:
    explicit FlatpakSourcesBackend(const QVector<FlatpakInstallation *> &installations, AbstractResourcesBackend *parent);
    ~FlatpakSourcesBackend() override;

    enum Roles {
        IconUrlRole = LastRole + 1,
        PrioRole,
    };

    QAbstractItemModel *sources() override;
    bool addSource(const QString &id) override;
    bool removeSource(const QString &id) override;
    QString idDescription() override;
    QVariantList actions() const override;
    bool supportsAdding() const override
    {
        return true;
    }
    bool canFilterSources() const override
    {
        return true;
    }

    FlatpakRemote *installSource(FlatpakResource *resource);
    bool canMoveSources() const override
    {
        return true;
    }

    bool moveSource(const QString &disambiguatedSourceId, int delta) override;
    int originIndex(const QString &disambiguatedSourceId) const;
    QStandardItem *sourceByUrl(const QString &url) const;
    QStandardItem *sourceByDisambiguatedId(const QString &disambiguatedSourceId) const;
    DiscoverAction *inlineAction() const override
    {
        return m_saveAction;
    }

    void cancel() override;
    void proceed() override;

    void save();
    void addRemote(FlatpakRemote *remote, FlatpakInstallation *installation);

private:
    FlatpakInstallation *m_preferredInstallation;
    QStandardItemModel *m_sources;
    DiscoverAction *const m_flathubAction;
    DiscoverAction *const m_saveAction;
    QStandardItem *m_noSourcesItem;
    QStack<std::function<void()>> m_proceedFunctions;
};
