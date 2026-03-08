/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <resources/AbstractResourcesBackend.h>

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariantList>

#ifdef FWUPD_EXTERNC_REQUIRED
extern "C" {
#endif

#include <fwupd.h>
#ifdef FWUPD_EXTERNC_REQUIRED
}
#endif
#include <glib-2.0/glib-object.h>

class DiscoverAction;
class StandardBackendUpdater;
class FwupdResource;
class FwupdBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    Q_PROPERTY(int startElements MEMBER m_startElements)
public:
    explicit FwupdBackend(QObject *parent = nullptr);
    ~FwupdBackend();

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);
    QHash<QString, FwupdResource *> resources() const
    {
        return m_resources;
    }
    bool isValid() const override
    {
        return m_isValid;
    }

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;
    FwupdClient *client;
    void handleError(GError *perror);

    static QString cacheFile(const QString &kind, const QString &baseName);
    void setDevices(GPtrArray *);
    void setRemotes(GPtrArray *);

    int fetchingUpdatesProgress() const override
    {
        return m_fetching > 0 ? 42 : 100;
    }
    uint fetchingUpdatesProgressWeight() const override
    {
        return 60;
    }

Q_SIGNALS:
    void initialized();

private:
    ResultsStream *resourceForFile(const QUrl &);
    void addUpdates();
    void addResource(FwupdResource *res);

    static QMap<GChecksumType, QCryptographicHash::Algorithm> gchecksumToQChryptographicHash();
    static QByteArray getChecksum(const QString &filename, QCryptographicHash::Algorithm hashAlgorithm);

    FwupdResource *createRelease(FwupdDevice *device);
    FwupdResource *createApp(FwupdDevice *device);

    QHash<QString, FwupdResource *> m_resources;
    StandardBackendUpdater *m_updater;
    bool m_fetching = false;
    int m_startElements;
    QList<AbstractResource *> m_toUpdate;
    GCancellable *m_cancellable;
    bool m_isValid = true;
};
