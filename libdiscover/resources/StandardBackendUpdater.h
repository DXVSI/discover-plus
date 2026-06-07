/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "AbstractResourcesBackend.h"
#include "discovercommon_export.h"
#include <EmitWhenChanged.h>
#include <QDateTime>
#include <QSet>
#include <QTimer>
#include <resources/AbstractBackendUpdater.h>

class AbstractResourcesBackend;

class DISCOVERCOMMON_EXPORT StandardBackendUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
public:
    explicit StandardBackendUpdater(AbstractResourcesBackend *parent = nullptr);

    bool hasUpdates() const override;
    qreal progress() const override;
    void start() override;

    QList<AbstractResource *> toUpdate() const override;
    void addResources(const QList<AbstractResource *> &apps) override;
    void removeResources(const QList<AbstractResource *> &apps) override;
    void prepare() override;
    QDateTime lastUpdate() const override;
    bool isCancelable() const override;
    bool isProgressing() const override;
    bool isMarked(AbstractResource *res) const override;
    double updateSize() const override;
    void setProgress(qreal p);
    int updatesCount() const;
    void cancel() override;
    quint64 downloadSpeed() const override;
    bool isFetchingUpdates() const override;

    bool isSettingUp() const
    {
        return m_settingUp;
    }

Q_SIGNALS:
    void cancelTransaction();
    void updatesCountChanged(int updatesCount);
    void settingUpChanged();

public Q_SLOTS:
    void transactionRemoved(Transaction *t);
    void cleanup();

private:
    void resourcesChanged(AbstractResource *res, const QVector<QByteArray> &props);
    void refreshUpdateable();
    void transactionAdded(Transaction *newTransaction);
    void transactionProgressChanged();
    void refreshProgress();
    void setSettingUp(bool settingUp);
    QVector<Transaction *> transactions() const;

    QSet<AbstractResource *> m_toUpgrade;
    QSet<AbstractResource *> m_upgradeable;
    AbstractResourcesBackend *const m_backend;
    QSet<AbstractResource *> m_pendingResources;
    bool m_hasBeenPopulated = false;
    bool m_settingUp = false;
    qreal m_progress;
    QDateTime m_lastUpdate;
    QTimer m_timer;
    bool m_canCancel = false;
    bool m_anyTransactionFailed = false;
    EmitWhenChanged<bool> m_isFetchingUpdates;
};
