/*
 *   SPDX-FileCopyrightText: 2026 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <resources/AbstractBackendUpdater.h>
#include <resources/AbstractResourcesBackend.h>

class AppStreamPreviewBackendUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
public:
    explicit AppStreamPreviewBackendUpdater(AbstractResourcesBackend *parent = nullptr);

    void prepare() override;
    bool hasUpdates() const override;
    qreal progress() const override;
    void removeResources(const QList<AbstractResource *> &apps) override;
    void addResources(const QList<AbstractResource *> &apps) override;
    QList<AbstractResource *> toUpdate() const override;
    QDateTime lastUpdate() const override;
    bool isCancelable() const override;
    bool isProgressing() const override;
    bool isMarked(AbstractResource *res) const override;
    double updateSize() const override;
    quint64 downloadSpeed() const override;
    bool isFetchingUpdates() const override;
    void start() override;

private:
    AbstractResourcesBackend *const m_backend;
};
