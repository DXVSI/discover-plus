/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "DiscoverAction.h"
#include "discovercommon_export.h"
#include <QObject>

class QAbstractItemModel;
class AbstractResourcesBackend;

class DISCOVERCOMMON_EXPORT AbstractSourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractResourcesBackend *resourcesBackend READ resourcesBackend CONSTANT)
    Q_PROPERTY(QAbstractItemModel *sources READ sources CONSTANT)
    Q_PROPERTY(QString idDescription READ idDescription CONSTANT)
    Q_PROPERTY(QVariantList actions READ actions CONSTANT) // TODO Make it a QVector<DiscoverAction*> again when we depend on newer than Qt 5.12
    Q_PROPERTY(DiscoverAction *inlineAction READ inlineAction CONSTANT) // TODO Make it a QVector<DiscoverAction*> again when we depend on newer than Qt 5.12
    Q_PROPERTY(bool supportsAdding READ supportsAdding CONSTANT)
    Q_PROPERTY(bool canMoveSources READ canMoveSources CONSTANT)
    Q_PROPERTY(bool canFilterSources READ canFilterSources CONSTANT)
    Q_PROPERTY(QString firstDisambiguatedSourceId READ firstDisambiguatedSourceId NOTIFY firstDisambiguatedSourceIdChanged)
    Q_PROPERTY(QString lastDisambiguatedSourceId READ lastDisambiguatedSourceId NOTIFY lastDisambiguatedSourceIdChanged)
public:
    explicit AbstractSourcesBackend(AbstractResourcesBackend *parent);
    ~AbstractSourcesBackend() override;

    enum Roles {
        IdRole = Qt::UserRole,
        DisambiguatedIdRole,
        LastRole,
    };
    Q_ENUM(Roles)

    virtual QString idDescription() = 0;

    Q_SCRIPTABLE virtual bool addSource(const QString &id) = 0;
    Q_SCRIPTABLE virtual bool removeSource(const QString &id) = 0;

    virtual QAbstractItemModel *sources() = 0;
    virtual QVariantList actions() const = 0;

    virtual bool supportsAdding() const = 0;
    virtual DiscoverAction *inlineAction() const
    {
        return nullptr;
    }

    AbstractResourcesBackend *resourcesBackend() const;

    virtual bool canFilterSources() const
    {
        return false;
    }

    virtual bool canMoveSources() const
    {
        return false;
    }
    Q_SCRIPTABLE virtual bool moveSource(const QString &sourceId, int delta);

    QString firstDisambiguatedSourceId() const;
    QString lastDisambiguatedSourceId() const;

public Q_SLOTS:
    virtual void cancel()
    {
    }

    virtual void proceed()
    {
    }

Q_SIGNALS:
    void firstDisambiguatedSourceIdChanged();
    void lastDisambiguatedSourceIdChanged();
    void passiveMessage(const QString &message);
    void proceedRequest(const QString &title, const QString &description);
};
