/*
 *   SPDX-FileCopyrightText: 2025 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "AppStreamPreviewBackendUpdater.h"
#include <appstream/OdrsReviewsBackend.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/StandardBackendUpdater.h>

class AppStreamPreviewBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit AppStreamPreviewBackend(QObject *parent = nullptr);

    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    bool isValid() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    int updatesCount() const override;
    QString displayName() const override;
    int fetchingUpdatesProgress() const override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addon) override;
    Transaction *removeApplication(AbstractResource *app) override;
    void checkForUpdates() override;

private:
    AppStreamPreviewBackendUpdater *m_updater;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
};
