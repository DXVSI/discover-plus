/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#include <ApplicationAddonsModel.h>
#include <Category/CategoryModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/DiscoverAction.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/SourcesModel.h>

#include "libdiscover_holo_debug.h"

class Atomupd1Adaptor;
class MockServer;

class HoloTest : public QObject
{
    Q_OBJECT
public:
    AbstractResourcesBackend *backendByName(ResourcesModel *m, const QString &name);

    HoloTest(QObject *parent = nullptr);

private Q_SLOTS:
    void initTestCase();

    void testUpdateInProgress();

private:
    ResourcesModel *m_model;
    AbstractResourcesBackend *m_appBackend;
    MockServer *m_server;
    Atomupd1Adaptor *m_adaptor;
};
