/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QJsonObject>
#include <QSharedPointer>
#include <Snapd/Snap>
#include <resources/AbstractResource.h>

class SnapBackend;
class QAbstractItemModel;
class QSnapdClient;

class SnapResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel WRITE setChannel NOTIFY channelChanged)
public:
    explicit SnapResource(QSharedPointer<QSnapdSnap> snap, AbstractResource::State state, SnapBackend *parent);
    ~SnapResource() override = default;

    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QJsonArray licenses() override;
    quint64 size() override;
    bool hasCategory(const QString &category) const override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    AbstractResource::Type type() const override;
    bool canExecute() const override;

    /**
     * Launches a snap using its desktop file via DBus.
     */
    void invokeApplication() const override;

    void fetchChangelog() override;
    void fetchScreenshots() override;
    QString author() const override;
    QList<PackageState> addonsInformation() override
    {
        return {};
    }
    void setSnap(const QSharedPointer<QSnapdSnap> &snap);

    void setState(AbstractResource::State state);
    QString sourceIcon() const override
    {
        return QStringLiteral("snapdiscover");
    }

    QDate releaseDate() const override;

    Q_SCRIPTABLE QAbstractItemModel *plugs(QObject *parentC);
    Q_SCRIPTABLE QObject *channels(QObject *parent);
    QString appstreamId() const override;

    QStringList topObjects() const override;
    QString verifiedMessage() const override;
    QString verifiedIconName() const override;

    QString channel();
    void setChannel(const QString &channel);

    quint64 installedSize() const;
    quint64 downloadSize() const;
    void updateSizes();

    QUrl homepage() override;

    QSharedPointer<QSnapdSnap> snap() const
    {
        return m_snap;
    }

Q_SIGNALS:
    void channelChanged(const QString &channel);
    void newSnap();

public:
    QSnapdClient *client() const;
    void refreshSnap();
    void gotIcon();
    AbstractResource::State m_state;
    quint64 m_installedSize;
    quint64 m_downloadSize;

    QSharedPointer<QSnapdSnap> m_snap;
    QString m_executableDesktop;
    QString m_channel;
    mutable QVariant m_icon;
    static const QStringList s_topObjects;
};
