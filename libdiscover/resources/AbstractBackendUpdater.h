/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"
#include <QObject>

class QDateTime;
class AbstractResource;
class AbstractResourcesBackend;

/**
 * \class AbstractBackendUpdater  AbstractBackendUpdater.h "AbstractBackendUpdater.h"
 *
 * \brief This is the base class for all abstract classes, which handle system upgrades.
 *
 * While implementing this is not mandatory for all backends (you can also use the
 * StandardBackendUpdater, which just uses the functions in the ResourcesBackend to
 * update the packages), it is recommended for many.
 *
 * Before starting the update, the AbstractBackendUpdater will have to keep a list of
 * packages, which are about to be upgraded. First, all packages have to be inserted
 * into this list in the \prepare method and they can then be changed by the user through
 * the \addResources and \removeResources functions.
 *
 * When \start is called, the AbstractBackendUpdater should start the update and report its
 * progress through the rest of methods outlined in this API documentation.
 *
 * @see addResources
 * @see removeResources
 * @see start
 * @see prepare
 */
class DISCOVERCOMMON_EXPORT AbstractBackendUpdater : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isCancelable READ isCancelable NOTIFY cancelableChanged)
    Q_PROPERTY(bool isProgressing READ isProgressing NOTIFY progressingChanged)
    Q_PROPERTY(bool needsReboot READ needsReboot NOTIFY needsRebootChanged)
    Q_PROPERTY(quint64 downloadSpeed READ downloadSpeed NOTIFY downloadSpeedChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool isFetchingUpdates READ isFetchingUpdates NOTIFY fetchingChanged)
public:
    enum State {
        None,
        Downloading,
        Installing,
        Done,
    };
    Q_ENUM(State)

    /**
     * Constructs an AbstractBackendUpdater
     */
    explicit AbstractBackendUpdater(QObject *parent = nullptr);

    /**
     * This method is called, when Discover switches to the updates view.
     * Here the backend should mark all upgradeable packages as to be upgraded.
     */
    virtual void prepare() = 0;

    /**
     * @returns true if the backend contains packages which can be updated
     */
    virtual bool hasUpdates() const = 0;
    /**
     * @returns the progress of the update in percent
     */
    virtual qreal progress() const = 0;

    /**
     * This method is used to remove resources from the list of packages
     * marked to be upgraded. It will potentially be called before \start.
     */
    virtual void removeResources(const QList<AbstractResource *> &apps) = 0;
    /**
     * This method is used to add resource to the list of packages marked to be upgraded.
     * It will potentially be called before \start.
     */
    virtual void addResources(const QList<AbstractResource *> &apps) = 0;

    /**
     * @returns the list of updateable resources in the system
     */
    virtual QList<AbstractResource *> toUpdate() const = 0;

    /**
     * @returns the QDateTime when the last update happened
     */
    virtual QDateTime lastUpdate() const = 0;

    /**
     * @returns whether the updater can currently be canceled or not
     * @see cancelableChanged
     */
    virtual bool isCancelable() const = 0;
    /**
     * @returns whether the updater is currently running or not
     * this property decides, if there will be progress reporting in the GUI.
     * This has to stay true during the whole transaction!
     * @see progressingChanged
     */
    virtual bool isProgressing() const = 0;

    /**
     * @returns whether @p res is marked for update
     */
    virtual bool isMarked(AbstractResource *res) const = 0;

    virtual void fetchChangelog() const;

    /**
     * @returns the size of all the packages set to update combined
     */
    virtual double updateSize() const = 0;

    /**
     * @returns the speed at which we are downloading
     */
    virtual quint64 downloadSpeed() const = 0;

    void setNeedsReboot(bool enabled);
    void enableReadyToReboot();

    bool isReadyToReboot() const;
    bool needsReboot() const;

    void setErrorMessage(const QString &errorMessage);
    QString errorMessage() const
    {
        return m_errorMessage;
    }

    virtual void setOfflineUpdates(bool useOfflineUpdates);
    virtual bool isFetchingUpdates() const = 0;

public Q_SLOTS:
    /**
     * If \isCancelable is true during the transaction, this method has
     * to be implemented and will potentially be called when the user
     * wants to cancel the update.
     */
    virtual void cancel();
    /**
     * This method starts the update. All packages which are in \toUpdate
     * are going to be updated.
     *
     * From this moment on the AbstractBackendUpdater should continuously update
     * the other methods to show its progress.
     *
     * @see progress
     * @see progressChanged
     * @see isProgressing
     * @see progressingChanged
     */
    virtual void start() = 0;

    /**
     * Answers a proceed request
     */
    virtual void proceed()
    {
    }

Q_SIGNALS:
    /**
     * The AbstractBackendUpdater should Q_EMIT this signal when the progress changed.
     * @see progress
     */
    void progressChanged(qreal progress);
    /**
     * The AbstractBackendUpdater should Q_EMIT this signal when the cancelable property changed.
     * @see isCancelable
     */
    void cancelableChanged(bool cancelable);
    /**
     * The AbstractBackendUpdater should Q_EMIT this signal when the progressing property changed.
     * @see isProgressing
     */
    void progressingChanged(bool progressing);
    /**
     * The AbstractBackendUpdater should Q_EMIT this signal when the status detail changed.
     * @see statusDetail
     */
    void statusDetailChanged(const QString &msg);
    /**
     * The AbstractBackendUpdater should Q_EMIT this signal when the status message changed.
     * @see statusMessage
     */
    void statusMessageChanged(const QString &msg);
    /**
     * The AbstractBackendUpdater should Q_EMIT this signal when the download speed changed.
     * @see downloadSpeed
     */
    void downloadSpeedChanged(quint64 downloadSpeed);

    /**
     * Provides the @p progress of a specific @p resource in a percentage.
     */
    void resourceProgressed(AbstractResource *resource, qreal progress, AbstractBackendUpdater::State state);

    void passiveMessage(const QString &message);

    /**
     * Provides a message to be shown to the user
     *
     * The user gets to acknowledge and proceed or cancel the transaction.
     *
     * @sa proceed(), cancel()
     */
    void proceedRequest(const QString &title, const QString &description);

    /**
     * A fatal error was found on distro packaging. Provide a @p message to show
     * in a modal dialog that should lead the user towards reporting the problem..
     */
    void distroErrorMessage(const QString &message);

    /**
     * emitted when the updater decides it needs to reboot
     */
    void needsRebootChanged();

    /** emitted when we find a new errorMessage to display */
    void errorMessageChanged();

    /// emitted when the backend is fetching updates
    void fetchingChanged();

private:
    bool m_needsReboot = false;
    bool m_readyToReboot = false;
    QString m_errorMessage;
};
