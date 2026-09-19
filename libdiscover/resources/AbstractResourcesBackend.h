/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "DiscoverConfig.h"

#include <QObject>
#include <QPair>
#include <QVariantList>
#include <QVector>

#include "AbstractResource.h"
#include "DiscoverAction.h"
#include "Transaction/AddonList.h"

#include "discovercommon_export.h"

class Transaction;
class Category;
class AbstractReviewsBackend;
class AbstractBackendUpdater;

struct StreamResult {
    StreamResult(AbstractResource *resource = nullptr, uint sortScore = 0)
        : resource(resource)
        , sortScore(sortScore)
    {
    }

    AbstractResource *resource = nullptr;
    uint sortScore = 0;

    bool operator==(const StreamResult &other) const
    {
        return resource == other.resource;
    }
};

inline size_t qHash(const StreamResult &key, size_t seed = 0)
{
    return qHash(quintptr(key.resource), seed);
}

class DISCOVERCOMMON_EXPORT ResultsStream : public QObject
{
    Q_OBJECT
public:
    ResultsStream(const QString &objectName);

    /// assumes all the information is in @p resources
    ResultsStream(const QString &objectName, const QVector<StreamResult> &resources);
    ~ResultsStream() override;

    void finish();

Q_SIGNALS:
    void resourcesFound(const QVector<StreamResult> &resources);
    void fetchMore();
};

class DISCOVERCOMMON_EXPORT InlineMessage : public QObject
{
    Q_OBJECT
public:
    // Keep in sync with Kirigami's in enums.h
    enum InlineMessageType {
        Information = 0,
        Positive,
        Warning,
        Error,
    };
    Q_ENUM(InlineMessageType)
    Q_PROPERTY(InlineMessageType type MEMBER type CONSTANT)
    Q_PROPERTY(QString iconName MEMBER iconName CONSTANT)
    Q_PROPERTY(QString message MEMBER message CONSTANT)
    Q_PROPERTY(QVariantList actions MEMBER actions CONSTANT)

    InlineMessage(InlineMessageType type, const QString &iconName, const QString &message, DiscoverAction *action = nullptr)
        : type(type)
        , iconName(iconName)
        , message(message)
        , actions(action ? QVariantList{QVariant::fromValue<QObject *>(action)} : QVariantList())
    {
    }

    InlineMessage(InlineMessageType type, const QString &iconName, const QString &message, const QVariantList &actions)
        : type(type)
        , iconName(iconName)
        , message(message)
        , actions(actions)
    {
    }

    InlineMessageType type;
    const QString iconName;
    const QString message;
    const QVariantList actions;
};

/**
 * \class AbstractResourcesBackend  AbstractResourcesBackend.h "AbstractResourcesBackend.h"
 *
 * \brief This is the base class of all resource backends.
 *
 * For writing basic new resource backends, we need to implement two classes: this and the
 * AbstractResource one. Basic questions on how to build your plugin with those classes
 * can be answered by looking at the dummy plugin.
 *
 * As this is the base class of a backend, we save all the created resources here and also
 * accept calls to install and remove applications or to cancel transactions.
 *
 * To show resources in Discover, we need to initialize all resources we want to show beforehand,
 * we should not create resources in the search function. When we reload the resources
 * (e.g. when initializing), the backend needs change the fetching property throughout the
 * process.
 */
class DISCOVERCOMMON_EXPORT AbstractResourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(AbstractReviewsBackend *reviewsBackend READ reviewsBackend CONSTANT)
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(int fetchingUpdatesProgress READ fetchingUpdatesProgress NOTIFY fetchingUpdatesProgressChanged)
    Q_PROPERTY(bool hasSecurityUpdates READ hasSecurityUpdates NOTIFY updatesCountChanged)
    Q_PROPERTY(bool hasApplications READ hasApplications CONSTANT)

    Q_MOC_INCLUDE("ReviewsBackend/AbstractReviewsBackend.h")
public:
    /**
     * Constructs an AbstractResourcesBackend
     * @param parent the parent of the class (the object will be deleted when the parent gets deleted)
     */
    explicit AbstractResourcesBackend(QObject *parent = nullptr);

    /**
     * @returns true when the backend is in a valid state, which means it is able to work
     * You must return true here if you want the backend to be loaded.
     */
    virtual bool isValid() const = 0;

    struct Filters {
        std::shared_ptr<Category> category = nullptr;
        AbstractResource::State state = AbstractResource::Broken;
        QString mimetype;
        QString search;
        QString extends;
        QUrl resourceUrl;
        QString origin;
        bool allBackends = false;
        bool filterMinimumState = true;
        AbstractResourcesBackend *backend = nullptr;
        // Not a filter: the list is shown by name. A backend that pages through a
        // remote list (COPR) asks for it in that order, everything else ignores this.
        bool orderedByName = false;

        bool isEmpty() const
        {
            return !category && state == AbstractResource::Broken && mimetype.isEmpty() && search.isEmpty() && extends.isEmpty() && resourceUrl.isEmpty()
                && origin.isEmpty();
        }

        bool shouldFilter(AbstractResource *res) const;
        void filterJustInCase(QVector<AbstractResource *> &input) const;
        void filterJustInCase(QVector<StreamResult> &input) const;
    };

    /**
     * @returns a stream that will provide elements that match the search
     */

    virtual ResultsStream *search(const Filters &search) = 0; // FIXME: Probably provide a standard implementation?!

    /**
     * @returns the reviews backend of this AbstractResourcesBackend (which handles all ratings and reviews of resources)
     */
    virtual AbstractReviewsBackend *reviewsBackend() const = 0; // FIXME: Have a standard impl which returns 0?

    /**
     * @returns the class which is used by Discover to update the users system, if you are unsure what to do
     * just return the StandardBackendUpdater
     */
    virtual AbstractBackendUpdater *backendUpdater() const = 0; // FIXME: Standard impl returning the standard updater?

    /**
     * @returns the number of resources for which an update is available, it should only count technical packages
     */
    virtual int updatesCount() const = 0; // FIXME: Probably provide a standard implementation?!

    /**
     * @returns whether either of the updates contains a security fix
     */
    virtual bool hasSecurityUpdates() const
    {
        return false;
    }

    /**
     * @returns the appstream ids that this backend extends
     */
    virtual bool extends(const QString &id) const;

    /** @returns the plugin's name */
    QString name() const;

    /** @internal only to be used by the factory */
    void setName(const QString &name);

    virtual QString displayName() const = 0;

    /**
     * emits a change for all rating properties
     */
    void emitRatingsReady();

    /**
     * @returns the root category tree
     */
    virtual QList<std::shared_ptr<Category>> category() const
    {
        return {};
    }

    virtual bool hasApplications() const
    {
        return false;
    }

    virtual int fetchingUpdatesProgress() const = 0;

    /**
     * @returns how much this backend should influence the global fetching progress.
     * This is not a percentage.
     */
    virtual uint fetchingUpdatesProgressWeight() const;

    enum AboutToAction { Reboot, PowerOff };

    /**
     * Notifies the backend to prepare for a certain power action.
     * Currently only used in the PackageKit backend.
     * @param action the action that will occur
     */
    virtual void aboutTo(AboutToAction action) { Q_UNUSED(action); };

    /**
     * @returns if the backend needs a reboot even when the PowerOff action is chosen,
     * which would occur if the PowerOff happens after a reboot happens first
     */
    virtual bool needsRebootForPowerOffAction() const
    {
        return false;
    }
public Q_SLOTS:
    /**
     * This gets called when the backend should install an application.
     * The AbstractResourcesBackend should create a Transaction object, is returned and
     * will be included in the TransactionModel
     * @param app the application to be installed
     * @param addons the addons which should be installed with the application
     * @returns the Transaction that keeps track of the installation process
     */
    virtual Transaction *installApplication(AbstractResource *app, const AddonList &addons) = 0;

    /**
     * Overloaded function, which simply does the same, except not installing any addons.
     */
    virtual Transaction *installApplication(AbstractResource *app);

    /**
     * This gets called when the backend should remove an application.
     * Like in the installApplication() method, we'll return the Transaction
     * responsible for the removal.
     *
     * @see installApplication
     * @param app the application to be removed
     * @returns the Transaction that keeps track of the removal process
     */
    virtual Transaction *removeApplication(AbstractResource *app) = 0;

    /**
     * Notifies the backend that the user wants the information to be up to date
     */
    virtual void checkForUpdates() = 0;

    /**
     * Provides a guess why a search might not have offered satisfactory results
     */
    Q_SCRIPTABLE virtual InlineMessage *explainDysfunction() const;

Q_SIGNALS:
    /**
     * Notify of a change in the backend
     */
    void contentsChanged();

    /**
     * This should be emitted when the number of upgradeable packages changed.
     */
    void updatesCountChanged();
    /**
     * This should be emitted when all data of the backends resources changed. Internally it will Q_EMIT
     * a signal in the model to show the view that all data of a certain backend changed.
     */
    void allDataChanged(const QVector<QByteArray> &propertyNames);

    /**
     * Allows to notify some @p properties in @p resource have changed
     */
    void resourcesChanged(AbstractResource *resource, const QVector<QByteArray> &properties);
    void resourceRemoved(AbstractResource *resource);

    void passiveMessage(const QString &message);
    void inlineMessageChanged(const QSharedPointer<InlineMessage> &inlineMessage);
    void fetchingUpdatesProgressChanged();

    /**
     * isValid will now return false
     *
     * A backend cannot become valid in its lifetime.
     */
    void invalidated();

private:
    QString m_name;
};

DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const AbstractResourcesBackend::Filters &filters);
DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const StreamResult &filters);

/**
 * @internal Workaround because QPluginLoader enforces 1 instance per plugin
 */
class DISCOVERCOMMON_EXPORT AbstractResourcesBackendFactory : public QObject
{
    Q_OBJECT
public:
    virtual QVector<AbstractResourcesBackend *> newInstance(QObject *parent, const QString &name) const = 0;
};

#define DISCOVER_BACKEND_PLUGIN(ClassName)                                                                                                                     \
    class ClassName##Factory : public AbstractResourcesBackendFactory                                                                                          \
    {                                                                                                                                                          \
        Q_OBJECT                                                                                                                                               \
        Q_PLUGIN_METADATA(IID DISCOVER_PLUGIN_IID)                                                                                                             \
        Q_INTERFACES(AbstractResourcesBackendFactory)                                                                                                          \
    public:                                                                                                                                                    \
        QVector<AbstractResourcesBackend *> newInstance(QObject *parent, const QString &name) const override                                                   \
        {                                                                                                                                                      \
            auto c = new ClassName(parent);                                                                                                                    \
            c->setName(name);                                                                                                                                  \
            return {c};                                                                                                                                        \
        }                                                                                                                                                      \
    };

Q_DECLARE_INTERFACE(AbstractResourcesBackendFactory, DISCOVER_PLUGIN_IID)
