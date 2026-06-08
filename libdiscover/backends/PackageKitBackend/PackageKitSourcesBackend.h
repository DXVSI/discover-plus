/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <PackageKit/Transaction>
#include <resources/AbstractSourcesBackend.h>

class QStandardItem;
class PKSourcesModel;

class PackageKitSourcesBackend : public AbstractSourcesBackend
{
    Q_OBJECT
public:
    PackageKitSourcesBackend(AbstractResourcesBackend *parent);

    QString idDescription() override;

    bool supportsAdding() const override
    {
        return false;
    }
    bool addSource(const QString &id) override;
    bool removeSource(const QString &id) override;

    QAbstractItemModel *sources() override;
    QVariantList actions() const override;

    void transactionError(PackageKit::Transaction::Error, const QString &message);
    void resetSources();

private:
    void addRepositoryDetails(const QString &id, const QString &description, bool enabled);
    QStandardItem *findItemForId(const QString &id) const;

    PKSourcesModel *m_sources;
    QVariantList m_actions;
};
