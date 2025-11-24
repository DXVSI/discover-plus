/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DiscoverDeclarativePlugin.h"
#include "ReadFile.h"
#include <ApplicationAddonsModel.h>
#include <SearchHistory.h>
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <ScreenshotsModel.h>
#include <Transaction/Transaction.h>
#include <Transaction/TransactionListener.h>
#include <Transaction/TransactionModel.h>
#include <UpdateModel/UpdateModel.h>
#include <qqml.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/AbstractResource.h>
#include <resources/AbstractSourcesBackend.h>
#include <resources/DiscoverAction.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/SourcesModel.h>

class ActivatableType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
public:
    explicit ActivatableType(QQuickItem *parent)
        : QObject(parent)
        , m_active(parent->isVisible())
        , m_theItem(parent)
    {
    }

    void setActive(bool active)
    {
        if (active == m_active) {
            return;
        }

        m_active = active;
        m_theItem->setVisible(active);
        Q_EMIT activeChanged();
    }
    bool active() const
    {
        return m_active;
    }

    static ActivatableType *qmlAttachedProperties(QObject *object)
    {
        auto item = qobject_cast<QQuickItem *>(object);
        if (!item) {
            return nullptr;
        }
        return new ActivatableType(item);
    }

Q_SIGNALS:
    void activeChanged();

private:
    bool m_active = false;
    QQuickItem *m_theItem = nullptr;
};
QML_DECLARE_TYPEINFO(ActivatableType, QML_HAS_ATTACHED_PROPERTIES)

void DiscoverDeclarativePlugin::registerTypes(const char * /*uri*/)
{
    qRegisterMetaType<std::shared_ptr<Category>>("std::shared_ptr<Category>");

    qmlRegisterType<TransactionListener>("org.kde.discover", 2, 0, "TransactionListener");
    qmlRegisterType<ResourcesUpdatesModel>("org.kde.discover", 2, 0, "ResourcesUpdatesModel");
    qmlRegisterType<ResourcesProxyModel>("org.kde.discover", 2, 0, "ResourcesProxyModel");
    qmlRegisterUncreatableType<ResourcesCount>("org.kde.discover", 2, 0, "resourcesCount", QStringLiteral("should come from ResourcesProxyModel"));

    qmlRegisterType<ReviewsModel>("org.kde.discover", 2, 0, "ReviewsModel");
    qmlRegisterType<ApplicationAddonsModel>("org.kde.discover", 2, 0, "ApplicationAddonsModel");
    qmlRegisterType<ScreenshotsModel>("org.kde.discover", 2, 0, "ScreenshotsModel");
    qmlRegisterType<UpdateModel>("org.kde.discover", 2, 0, "UpdateModel");
    qmlRegisterType<ReadFile>("org.kde.discover", 2, 0, "ReadFile");
    qmlRegisterType<SearchHistory>("org.kde.discover", 2, 0, "SearchHistory");

    qmlRegisterUncreatableType<DiscoverAction>("org.kde.discover", 2, 0, "DiscoverAction", QStringLiteral("Use QQC Action"));
    qmlRegisterUncreatableType<AbstractResource>("org.kde.discover", 2, 0, "AbstractResource", QStringLiteral("should come from the ResourcesModel"));
    qmlRegisterUncreatableType<AbstractSourcesBackend>("org.kde.discover", 2, 0, "AbstractSourcesBackend", QStringLiteral("should come from the SourcesModel"));
    qmlRegisterUncreatableType<Transaction>("org.kde.discover", 2, 0, "Transaction", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<SourcesModel>("org.kde.discover", 2, 0, "SourcesModelClass", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<AbstractBackendUpdater>("org.kde.discover", 2, 0, "AbstractBackendUpdater", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<InlineMessage>("org.kde.discover", 2, 0, "InlineMessage", QStringLiteral("should come from the backend"));
    qmlRegisterUncreatableType<Category>("org.kde.discover", 2, 0, "Category", QStringLiteral("should come from CategoryModel"));
    qmlRegisterUncreatableType<AbstractReviewsBackend>("org.kde.discover", 2, 0, "AbstractReviewsBackend", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<AbstractResourcesBackend>("org.kde.discover", 2, 0, "AbstractResourcesBackend", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<ActivatableType>("org.kde.discover", 2, 0, "Activatable", QStringLiteral("attached"));

    qmlRegisterAnonymousType<TransactionModel>("org.kde.discover", 1);
    qmlRegisterAnonymousType<Rating>("org.kde.discover", 1);
    qmlRegisterAnonymousType<AbstractResourcesBackend>("org.kde.discover", 1);
    qmlRegisterAnonymousType<AbstractReviewsBackend>("org.kde.discover", 1);
    qmlRegisterAnonymousType<Category>("org.kde.discover", 1);
    qmlRegisterAnonymousType<ResourcesModel>("org.kde.discover", 1);

    qmlRegisterSingletonInstance("org.kde.discover", 2, 0, "CategoryModel", CategoryModel::global());
    qmlRegisterSingletonInstance("org.kde.discover", 2, 0, "ResourcesModel", ResourcesModel::global());
    qmlRegisterSingletonInstance("org.kde.discover", 2, 0, "TransactionModel", TransactionModel::global());
    qmlRegisterSingletonInstance("org.kde.discover", 2, 0, "SourcesModel", SourcesModel::global());

    qmlProtectModule("org.kde.discover", 2);
}

#include "DiscoverDeclarativePlugin.moc"
