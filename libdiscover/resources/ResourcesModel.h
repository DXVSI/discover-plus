/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "EmitWhenChanged.h"
#include <QSet>
#include <QTimer>
#include <QVector>

#include "AbstractResourcesBackend.h"
#include "discovercommon_export.h"

class DiscoverAction;

class DISCOVERCOMMON_EXPORT AggregatedResultsStream : public ResultsStream
{
    Q_OBJECT
public:
    AggregatedResultsStream(const QSet<ResultsStream *> &streams);
    ~AggregatedResultsStream();

    QSet<QObject *> streams() const
    {
        return m_streams;
    }

Q_SIGNALS:
    void finished();

private:
    void addResults(const QVector<StreamResult> &res);
    void emitResults();
    void streamDestruction(QObject *obj);
    void resourceDestruction(QObject *obj);
    void clear();

    QSet<QObject *> m_streams;
    QVector<StreamResult> m_results;
    QTimer m_delayedEmission;
};

class DISCOVERCOMMON_EXPORT ResourcesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool hasSecurityUpdates READ hasSecurityUpdates NOTIFY updatesCountChanged)
    Q_PROPERTY(AbstractResourcesBackend *currentApplicationBackend READ currentApplicationBackend WRITE setCurrentApplicationBackend NOTIFY
                   currentApplicationBackendChanged)
    Q_PROPERTY(DiscoverAction *updateAction READ updateAction CONSTANT)
    Q_PROPERTY(int fetchingUpdatesProgress READ fetchingUpdatesProgress NOTIFY fetchingUpdatesProgressChanged)
    Q_PROPERTY(QString remainingDescription READ remainingDescription NOTIFY fetchingUpdatesProgressChanged)
    Q_PROPERTY(QString applicationSourceName READ applicationSourceName NOTIFY currentApplicationBackendChanged)
    Q_PROPERTY(InlineMessage *inlineMessage READ inlineMessage NOTIFY inlineMessageChanged)
    Q_PROPERTY(QString distroName READ distroName CONSTANT)
public:
    /** This constructor should be only used by unit tests.
     *  @p backendName defines what backend will be loaded when the backend is constructed.
     */
    explicit ResourcesModel(const QString &backendName, QObject *parent = nullptr);
    static ResourcesModel *global();
    ~ResourcesModel() override;

    QVector<AbstractResourcesBackend *> backends() const;
    int updatesCount() const
    {
        return m_updatesCount.m_value;
    }
    bool hasSecurityUpdates() const;

    bool isInitializing() const;

    Q_SCRIPTABLE bool isExtended(const QString &id);

    AggregatedResultsStream *search(const AbstractResourcesBackend::Filters &search);
    void checkForUpdates();

    QString applicationSourceName() const;

    void setCurrentApplicationBackend(AbstractResourcesBackend *backend, bool writeConfig = true);
    AbstractResourcesBackend *currentApplicationBackend() const;

    DiscoverAction *updateAction() const
    {
        return m_updateAction;
    }
    int fetchingUpdatesProgress() const
    {
        return m_fetchingUpdatesProgress.m_value;
    }

    QString distroName() const;
    Q_INVOKABLE QUrl distroBugReportUrl();

    void setInlineMessage(const QSharedPointer<InlineMessage> &inlineMessage);
    InlineMessage *inlineMessage() const
    {
        return m_inlineMessage.data();
    }
    QString remainingDescription() const;

public Q_SLOTS:
    void installApplication(AbstractResource *app, const AddonList &addons);
    void installApplication(AbstractResource *app);
    void removeApplication(AbstractResource *app);

Q_SIGNALS:
    void fetchingChanged(bool isFetching);
    void backendsChanged();
    void updatesCountChanged(int updatesCount);
    void backendDataChanged(AbstractResourcesBackend *backend, const QVector<QByteArray> &properties);
    void resourceDataChanged(AbstractResource *resource, const QVector<QByteArray> &properties);
    void resourceRemoved(AbstractResource *resource);
    void passiveMessage(const QString &message);
    void currentApplicationBackendChanged(AbstractResourcesBackend *currentApplicationBackend);
    void fetchingUpdatesProgressChanged(int fetchingUpdatesProgress);
    void inlineMessageChanged(const QSharedPointer<InlineMessage> &inlineMessage);
    void switchToUpdates();

private Q_SLOTS:
    void callerContentsChanged();
    void updateCaller(const QVector<QByteArray> &properties);
    void registerAllBackends();

private:
    ///@p initialize tells if all backends load will be triggered on construction
    explicit ResourcesModel(QObject *parent = nullptr);
    void init(bool load);
    void addResourcesBackends(const QList<AbstractResourcesBackend *> &backends);
    bool addResourcesBackend(AbstractResourcesBackend *backend);
    void registerBackendByName(const QString &name);
    void initApplicationsBackend();
    void destroyObject();

    bool m_isInitializing = true;
    QVector<AbstractResourcesBackend *> m_backends;
    bool m_holdBackends = false;
    DiscoverAction *m_updateAction = nullptr;
    AbstractResourcesBackend *m_currentApplicationBackend;

    EmitWhenChanged<int> m_updatesCount;
    EmitWhenChanged<int> m_fetchingUpdatesProgress;
    QSharedPointer<InlineMessage> m_inlineMessage;

    static ResourcesModel *s_self;
    static bool s_quitting;
};
