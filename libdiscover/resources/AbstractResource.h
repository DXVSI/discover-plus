/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QCollatorSortKey>
#include <QColor>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QScopedPointer>
#include <QSet>
#include <QSize>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include <ReviewsBackend/Rating.h>

#include "Category/Category.h"
#include "PackageState.h"
#include "discovercommon_export.h"

class Category;
class Rating;
class AbstractResourcesBackend;

struct Screenshot {
    Screenshot(const QUrl &screenshot)
        : thumbnail(screenshot)
        , screenshot(screenshot)
    {
    }

    Screenshot(const QUrl &thumbnail, const QUrl &screenshot, bool isAnimated, const QSize &thumbnailSize)
        : thumbnail(thumbnail)
        , screenshot(screenshot)
        , isAnimated(isAnimated)
        , thumbnailSize(thumbnailSize)
    {
    }

    QUrl thumbnail;
    QUrl screenshot;
    bool isAnimated = false;
    QSize thumbnailSize;
};

using Screenshots = QVector<Screenshot>;

/**
 * \class AbstractResource  AbstractResource.h "AbstractResource.h"
 *
 * \brief This is the base class of all resources.
 *
 * Each backend must reimplement its own resource class which needs to derive from this one.
 */
class DISCOVERCOMMON_EXPORT AbstractResource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString packageName READ packageName CONSTANT)
    Q_PROPERTY(QString comment READ comment CONSTANT)
    Q_PROPERTY(QVariant icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(bool canExecute READ canExecute CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QUrl homepage READ homepage CONSTANT)
    Q_PROPERTY(QUrl helpURL READ helpURL CONSTANT)
    Q_PROPERTY(QUrl bugURL READ bugURL CONSTANT)
    Q_PROPERTY(QUrl donationURL READ donationURL CONSTANT)
    Q_PROPERTY(QUrl contributeURL READ contributeURL CONSTANT)
    Q_PROPERTY(bool canUpgrade READ canUpgrade NOTIFY stateChanged)
    Q_PROPERTY(bool isInstalled READ isInstalled NOTIFY stateChanged)
    Q_PROPERTY(QJsonArray licenses READ licenses NOTIFY licensesChanged)
    Q_PROPERTY(QString longDescription READ longDescription NOTIFY longDescriptionChanged)
    Q_PROPERTY(QString origin READ origin CONSTANT)
    Q_PROPERTY(QString displayOrigin READ displayOrigin CONSTANT)
    Q_PROPERTY(quint64 size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString sizeDescription READ sizeDescription NOTIFY sizeChanged)
    Q_PROPERTY(QString installedVersion READ installedVersion NOTIFY versionsChanged)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY versionsChanged)
    Q_PROPERTY(QString section READ section CONSTANT)
    Q_PROPERTY(QStringList mimetypes READ mimetypes CONSTANT)
    Q_PROPERTY(AbstractResourcesBackend *backend READ backend CONSTANT)
    Q_PROPERTY(Rating rating READ rating NOTIFY ratingFetched)
    Q_PROPERTY(QString appstreamId READ appstreamId CONSTANT)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    Q_PROPERTY(QString executeLabel READ executeLabel CONSTANT)
    Q_PROPERTY(QString sourceIcon READ sourceIcon CONSTANT)
    Q_PROPERTY(QString author READ author CONSTANT)
    Q_PROPERTY(QDate releaseDate READ releaseDate NOTIFY versionsChanged)
    Q_PROPERTY(QString upgradeText READ upgradeText NOTIFY versionsChanged)
    Q_PROPERTY(bool isRemovable READ isRemovable CONSTANT)
    Q_PROPERTY(QString versionString READ versionString NOTIFY versionsChanged)
    Q_PROPERTY(QString contentRatingDescription READ contentRatingDescription CONSTANT)
    Q_PROPERTY(uint contentRatingMinimumAge READ contentRatingMinimumAge CONSTANT)
    Q_PROPERTY(QStringList topObjects READ topObjects CONSTANT)
    Q_PROPERTY(QStringList bottomObjects READ bottomObjects CONSTANT)
    Q_PROPERTY(QString verifiedMessage READ verifiedMessage CONSTANT)
    Q_PROPERTY(QString verifiedIconName READ verifiedIconName CONSTANT)
    Q_PROPERTY(Type type READ type CONSTANT)

    // Resolve circular dependency for QObject* properties in both classes
    Q_MOC_INCLUDE("resources/AbstractResourcesBackend.h")

public:
    /**
     * This describes the state of the resource
     */
    enum State {
        /**
         * When the resource is somehow broken
         */
        Broken,
        /**
         * This means that the resource is neither installed nor broken
         */
        None,
        /**
         * The resource is installed and up-to-date
         */
        Installed,
        /**
         * The resource is installed and an update is available
         */
        Upgradeable,
    };
    Q_ENUM(State)

    /**
     * Constructs the AbstractResource with its corresponding backend
     */
    explicit AbstractResource(AbstractResourcesBackend *parent);
    ~AbstractResource() override;

    /// used as internal identification of a resource
    virtual QString packageName() const = 0;

    /// resource name to be displayed
    virtual QString name() const = 0;

    /// short description of the resource
    virtual QString comment() = 0;

    /// xdg-compatible icon name to represent the resource, url or QIcon
    virtual QVariant icon() const = 0;

    ///@returns whether invokeApplication makes something
    /// false if not overridden
    virtual bool canExecute() const = 0;

    /// executes the resource, if applies.
    Q_SCRIPTABLE virtual void invokeApplication() const = 0;

    virtual State state() = 0;

    virtual bool hasCategory(const QString &category) const = 0;
    ///@returns a URL that points to the app's website
    virtual QUrl homepage();
    ///@returns a URL that points to the app's online documentation
    virtual QUrl helpURL();
    ///@returns a URL that points to the place where you can file a bug
    virtual QUrl bugURL();
    ///@returns a URL that points to the place where you can donate money to the app developer
    virtual QUrl donationURL();
    ///@returns a URL that points to the place where you can contribute to develop the app
    virtual QUrl contributeURL();

    enum Type {
        Application,
        Addon,
        /**
         * This is where Flatpak runtimes would go
         */
        ApplicationSupport,
        /**
         * This is where system updates would go
         */
        System,
    };
    Q_ENUM(Type)
    virtual Type type() const = 0;

    virtual quint64 size() = 0;
    virtual QString sizeDescription();

    ///@returns a list of pairs with the name of the license and a URL pointing at it
    virtual QJsonArray licenses() = 0;

    virtual QString installedVersion() const = 0;
    virtual QString availableVersion() const = 0;
    virtual QString longDescription() = 0;

    virtual QString origin() const = 0;
    virtual QString disambiguatedOrigin() const;
    virtual QString displayOrigin() const;
    virtual QString section() = 0;
    virtual QString author() const = 0;

    ///@returns what kind of mime types the resource can consume
    virtual QStringList mimetypes() const;

    virtual QList<PackageState> addonsInformation() = 0;

    virtual QStringList extends() const;

    virtual QString appstreamId() const;

    void addMetadata(const QLatin1StringView &key, const QJsonValue &value);
    QJsonValue getMetadata(const QLatin1StringView &key);

    bool canUpgrade();
    bool isInstalled();

    ///@returns a user-readable explanation of the resource status
    /// by default, it will specify what state() is returning
    virtual QString status();

    AbstractResourcesBackend *backend() const;

    /**
     * @returns a name sort key for faster sorting
     */
    QCollatorSortKey nameSortKey();

    /**
     * Convenience method to fetch the resource's rating
     *
     * @returns the rating for the resource or null if not available
     */
    virtual Rating rating() const;

    bool categoryMatches(const std::shared_ptr<Category> &cat);

    QSet<std::shared_ptr<Category>> categoryObjects(const QList<std::shared_ptr<Category>> &cats) const;

    /**
     * @returns a url that uniquely identifies the application
     */
    virtual QUrl url() const;

    virtual QString executeLabel() const;
    virtual QString sourceIcon() const = 0;
    /**
     * @returns the date of the resource's most recent release
     */
    virtual QDate releaseDate() const = 0;

    virtual QSet<QString> alternativeAppstreamIds() const
    {
        return {};
    }

    virtual QString upgradeText() const;

    /**
     * @returns whether the package can ever be removed
     */
    virtual bool isRemovable() const
    {
        return true;
    }

    virtual QString versionString();

    virtual QString contentRatingDescription() const;
    virtual uint contentRatingMinimumAge() const;

    /**
     * @returns List of component URLs to display at the top of application page.
     */
    virtual QStringList topObjects() const;
    /**
     * @returns List of component URLs to display at the bottom of application page.
     */
    virtual QStringList bottomObjects() const;
    [[nodiscard]] virtual bool hasResolvedIcon() const;
    virtual void resolveIcon();

    /**
     * @returns whether the verification message of the resource
     *
     * Normally app stores will have extra trust checks on certain assets.
     * If the string is empty it means that the asset isn't verified.
     * The text will contain the explanation of what the verification entails.
     */
    virtual QString verifiedMessage() const
    {
        return {};
    }

    /**
     * @returns the icon name of the verification badge appropriate to the verification
     */
    virtual QString verifiedIconName() const
    {
        return QStringLiteral("checkmark");
    }

    virtual bool updateNeedsAttention()
    {
        return false;
    }

public Q_SLOTS:
    virtual void fetchScreenshots();
    virtual void fetchChangelog() = 0;
    virtual void fetchUpdateDetails()
    {
        fetchChangelog();
    }

Q_SIGNALS:
    void iconChanged();
    void sizeChanged();
    void stateChanged();
    void licensesChanged();
    void ratingFetched();
    void longDescriptionChanged();
    void versionsChanged();

    /// response to the fetchScreenshots method
    void screenshotsFetched(const Screenshots &screenshots);
    void changelogFetched(const QString &changelog);

private:
    void reportNewState();

    std::optional<QCollatorSortKey> m_collatorKey;
    QJsonObject m_metadata;
};

Q_DECLARE_METATYPE(QVector<AbstractResource *>)
