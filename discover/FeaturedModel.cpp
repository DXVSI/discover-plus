/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FeaturedModel.h"

#include "discover_debug.h"
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KSharedConfig>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QtGlobal>

#include <resources/ResourcesModel.h>
#include <utils.h>

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC(QString, featuredCache)

static QUrl featuredURL()
{
    QString config = QStringLiteral("/usr/share/discover/featuredurlrc");
    KConfigGroup grp(KSharedConfig::openConfig(config), u"Software"_s);
    if (grp.hasKey("FeaturedListingURL")) {
        return grp.readEntry("FeaturedListingURL", QUrl());
    }
    const auto baseURL = QLatin1StringView("https://autoconfig.kde.org/discover/");

    static const bool isMobile = QByteArrayList{"1", "true"}.contains(qgetenv("QT_QUICK_CONTROLS_MOBILE"));
    const QLatin1StringView fileName(isMobile ? "featured-mobile-5.9.json" : "featured-5.9.json");

    return QUrl(baseURL + fileName);
}

FeaturedModel::FeaturedModel()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);

    const static QUrl url = featuredURL();
    const QString fileName = url.fileName();
    *featuredCache = dir + QLatin1Char('/') + fileName;
    const bool shouldBlock = !QFileInfo::exists(*featuredCache);
    auto *fetchJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    if (shouldBlock) {
        acquireFetching(true);
    }
    connect(fetchJob, &KIO::StoredTransferJob::result, this, [this, fetchJob, shouldBlock]() {
        const auto dest = qScopeGuard([this, shouldBlock] {
            if (shouldBlock) {
                acquireFetching(false);
            }
            refresh();
        });
        if (fetchJob->error() != 0)
            return;

        QFile f(*featuredCache);
        if (!f.open(QIODevice::WriteOnly))
            qCWarning(DISCOVER_LOG) << "could not open" << *featuredCache << f.errorString();
        f.write(fetchJob->data());
        f.close();
    });
    if (!shouldBlock) {
        refresh();
    }
}

void FeaturedModel::refresh()
{
    // usually only useful if launching just fwupd or kns backends
    if (!currentApplicationBackend())
        return;

    acquireFetching(true);
    const auto dest = qScopeGuard([this] {
        acquireFetching(false);
    });
    QFile f(*featuredCache);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(DISCOVER_LOG) << "couldn't open file" << *featuredCache << f.errorString();
        return;
    }
    QJsonParseError error;
    const auto array = QJsonDocument::fromJson(f.readAll(), &error).array();
    if (error.error) {
        qCWarning(DISCOVER_LOG) << "couldn't parse" << *featuredCache << ". error:" << error.errorString();
        return;
    }

    const auto uris = kTransform<QVector<QUrl>>(array, [](const QJsonValue &uri) {
        return QUrl(uri.toString());
    });
    setUris(uris);
}

#include "moc_FeaturedModel.cpp"
