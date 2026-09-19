/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

// Qt includes
#include <QObject>

// Own includes
#include "AddonList.h"

#include "discovercommon_export.h"

class AbstractResource;

/**
 * \class Transaction  Transaction.h "Transaction.h"
 *
 * \brief This is the base class of all transactions.
 *
 * When there are transactions running inside Discover, the backends should
 * provide the corresponding Transaction objects with proper information.
 */
class DISCOVERCOMMON_EXPORT Transaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QVariant icon READ icon CONSTANT)
    Q_PROPERTY(AbstractResource *resource READ resource CONSTANT)
    Q_PROPERTY(Role role READ role CONSTANT)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool isCancellable READ isCancellable NOTIFY cancellableChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool visible READ isVisible CONSTANT)
    Q_PROPERTY(quint64 downloadSpeed READ downloadSpeed WRITE setDownloadSpeed NOTIFY downloadSpeedChanged)
    Q_PROPERTY(QString downloadSpeedString READ downloadSpeedString NOTIFY downloadSpeedChanged)
    Q_PROPERTY(QString remainingTimeString READ remainingTimeString NOTIFY remainingTimeChanged)
    Q_PROPERTY(uint remainingTime READ remainingTime NOTIFY remainingTimeChanged)

public:
    enum Status {
        /// Not queued, newly created
        SetupStatus = 0,
        /// Queued, but not yet run
        QueuedStatus,
        /// Transaction is in the downloading phase
        DownloadingStatus,
        /// Transaction is doing an installation/removal
        CommittingStatus,
        /// Transaction is done
        DoneStatus,
        /// Transaction is done, but there was an error during transaction
        DoneWithErrorStatus,
        /// Transaction was canceled
        CancelledStatus,
    };
    Q_ENUM(Status)

    enum Role {
        /// The transaction is going to install a resource
        InstallRole = 0,
        /// The transaction is going to remove a resource
        RemoveRole,
        /// The transaction is going to change the addons of a resource
        ChangeAddonsRole,
    };
    Q_ENUM(Role)

    Transaction(QObject *parent, AbstractResource *resource, Transaction::Role role, const AddonList &addons = {}, bool visible = true);

    ~Transaction() override;

    /**
     * @returns the AbstractResource which this transaction works with
     */
    AbstractResource *resource() const;
    /**
     * @returns the role which this transaction executes
     */
    Role role() const;
    /**
     * @returns the current status
     */
    Status status() const;
    /**
     * @returns the addons which this transaction works on
     */
    AddonList addons() const;
    /**
     * @returns true when the transaction can be canceled
     */
    bool isCancellable() const;
    /**
     * @returns a percentage of how much the transaction is already done
     */
    int progress() const;

    /**
     * Sets the status of the transaction
     * @param status the new status
     */
    void setStatus(Status status);
    /**
     * Sets whether the transaction can be canceled or not
     * @param isCancellable should be true if the transaction can be canceled
     */
    void setCancellable(bool isCancellable);
    /**
     * Sets the progress of the transaction
     * @param progress this should be a percentage of how much of the transaction is already done
     */
    void setProgress(int progress);

    /**
     * Cancels the transaction
     */
    Q_SCRIPTABLE virtual void cancel() = 0;

    /**
     * @returns if the transaction is either downloading or committing
     */
    bool isActive() const;

    Q_SCRIPTABLE virtual void proceed()
    {
    }

    /** @returns a name that identifies the transaction */
    virtual QString name() const;

    /** @returns an icon that describes the transaction */
    virtual QVariant icon() const;

    bool isVisible() const;

    quint64 downloadSpeed() const
    {
        return m_downloadSpeed;
    }
    void setDownloadSpeed(quint64 downloadSpeed);

    uint remainingTime() const
    {
        return m_remainingTime;
    }
    void setRemainingTime(uint seconds);

    QString downloadSpeedString() const;
    QString remainingTimeString() const;

private:
    AbstractResource *const m_resource;
    const Role m_role;
    Status m_status;
    const AddonList m_addons;
    bool m_isCancellable;
    int m_progress;
    bool m_visible = true;
    quint64 m_downloadSpeed = 0;
    uint m_remainingTime = 0;

Q_SIGNALS:
    /**
     * This gets emitted when the status of the transaction changed
     */
    void statusChanged(Transaction::Status status);
    /**
     * This gets emitted when the ability to cancel the transaction or not changed
     */
    void cancellableChanged(bool cancellable);
    /**
     * This gets emitted when the transaction changed the percentage of how much of it is already done
     */
    void progressChanged(int progress);

    /**
     * Provides a message to be shown to the user
     *
     * The user gets to acknowledge and proceed or cancel the transaction.
     *
     * @sa proceed(), cancel()
     */
    void proceedRequest(const QString &title, const QString &description);

    void passiveMessage(const QString &message);

    /**
     * A fatal error was found on distro packaging. Provide a @p message to show
     * in a modal dialog that should lead the user towards reporting the problem..
     */
    void distroErrorMessage(const QString &message);

    void downloadSpeedChanged(quint64 downloadSpeed);

    void remainingTimeChanged(uint remainingTime);

    void webflowStarted(const QUrl &url, int id);
    void webflowDone(int id);
};
