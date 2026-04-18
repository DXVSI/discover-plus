/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OdrsReviewsBackend.h"
#include "AppStreamIntegration.h"
#include "CachedNetworkAccessManager.h"
#include "OdrsReviewsJob.h"

#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>

#include <qnumeric.h>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>

#include <KIO/FileCopyJob>
#include <KLocalizedString>
#include <KUser>

#include "libdiscover_debug.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStandardPaths>

#include <QFutureWatcher>
#include <QtConcurrentRun>

// #define APIURL "http://127.0.0.1:5000/1.0/reviews/api"
#define APIURL "https://odrs.gnome.org/1.0/reviews/api"

QSharedPointer<OdrsReviewsBackend> OdrsReviewsBackend::global()
{
    static QSharedPointer<OdrsReviewsBackend> var = nullptr;
    if (!var) {
        var = QSharedPointer<OdrsReviewsBackend>(new OdrsReviewsBackend());
    }

    return var;
}

OdrsReviewsBackend::OdrsReviewsBackend()
    : AbstractReviewsBackend(nullptr)
{
    fetchRatings();
}

OdrsReviewsBackend::~OdrsReviewsBackend()
{
}

void OdrsReviewsBackend::fetchRatings()
{
    bool fetchRatings = false;
    const QUrl ratingsUrl(QLatin1StringView(APIURL "/ratings"));
    const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1StringView("/ratings/ratings"));
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

    // Create $HOME/.cache/discover/ratings folder
    cacheDir.mkpath(QLatin1StringView("ratings"));

    if (QFileInfo::exists(fileUrl.toLocalFile())) {
        QFileInfo file(fileUrl.toLocalFile());
        // Refresh the cached ratings if they are older than one day
        if (file.lastModified().msecsTo(QDateTime::currentDateTime()) > 1000 * 60 * 60 * 24) {
            fetchRatings = true;
        }
    } else {
        fetchRatings = true;
    }

    qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Fetch ratings:" << fetchRatings;
    if (fetchRatings) {
        setFetching(true);
        auto getJob = KIO::file_copy(ratingsUrl, fileUrl, -1, KIO::Overwrite | KIO::HideProgressInfo);
        connect(getJob, &KIO::FileCopyJob::result, this, &OdrsReviewsBackend::ratingsFetched);
    } else {
        parseRatings();
    }
}

void OdrsReviewsBackend::setFetching(bool fetching)
{
    if (fetching == m_isFetching) {
        return;
    }
    m_isFetching = fetching;
    Q_EMIT fetchingChanged(fetching);
}

void OdrsReviewsBackend::ratingsFetched(KJob *job)
{
    setFetching(false);
    if (job->error()) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Failed to fetch ratings:" << job->errorString();
    } else {
        parseRatings();
    }
}

static QString osName()
{
    return AppStreamIntegration::global()->osRelease()->name();
}

static QString userHash()
{
    QByteArray machineId;
    QFile file(QLatin1StringView("/etc/machine-id"));
    if (file.open(QIODevice::ReadOnly)) {
        machineId = file.readAll();
        file.close();
    }

    if (machineId.isEmpty()) {
        return QString();
    }

    const QByteArray salted = QByteArray("gnome-software[") + KUser().loginName().toUtf8() + ':' + machineId + ']';
    return QString::fromUtf8(QCryptographicHash::hash(salted, QCryptographicHash::Sha1).toHex());
}

ReviewsJob *OdrsReviewsBackend::fetchReviews(AbstractResource *resource, int page)
{
    if (resource->appstreamId().isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Fetching reviews for an invalid object";
        auto ret = new ReviewsJob();
        ret->deleteLater();
        return ret;
    }
    Q_UNUSED(page)
    QString version = resource->isInstalled() ? resource->installedVersion() : resource->availableVersion();
    if (version.isEmpty()) {
        version = QLatin1StringView("unknown");
    }

    const QJsonDocument document(QJsonObject{
        {QLatin1StringView("app_id"), resource->appstreamId()},
        {QLatin1StringView("distro"), osName()},
        {QLatin1StringView("user_hash"), userHash()},
        {QLatin1StringView("version"), version},
        {QLatin1StringView("locale"), QLocale::system().name()},
        {QLatin1StringView("limit"), -1},
    });

    const auto json = document.toJson(QJsonDocument::Compact);
    auto &job = m_jobs[json];
    if (job) {
        // If we already have it issued, reused. It happens if a query reset was made e.g. because the state changed
        return job;
    }
    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/fetch")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1StringView("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, json.size());
    job = OdrsReviewsJob::create(nam()->post(request, json), resource);
    connect(job, &ReviewsJob::reviewsReady, this, [this, json] {
        m_jobs.remove(json);
    });
    return job;
}

Rating OdrsReviewsBackend::ratingForApplication(AbstractResource *resource) const
{
    if (resource->appstreamId().isEmpty()) {
        return {};
    }

    return m_current.ratings[resource->appstreamId().toLower()];
}

void OdrsReviewsBackend::submitUsefulness(Review *review, bool useful)
{
    const QJsonDocument document(QJsonObject{
        {QLatin1StringView("app_id"), review->applicationName()},
        {QLatin1StringView("user_skey"), review->getMetadata(QLatin1StringView("ODRS::user_skey")).toString()},
        {QLatin1StringView("user_hash"), userHash()},
        {QLatin1StringView("distro"), osName()},
        {QLatin1StringView("review_id"), QJsonValue(double(review->id()))}, // if we really need uint64 we should get it in QJsonValue
    });

    QNetworkRequest request(QUrl(QStringLiteral(APIURL) + (useful ? QLatin1String("/upvote") : QLatin1String("/downvote"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1StringView("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    auto reply = nam()->post(request, document.toJson());
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::usefulnessSubmitted);
}

void OdrsReviewsBackend::usefulnessSubmitted()
{
    const auto reply = qobject_cast<QNetworkReply *>(sender());
    const auto networkError = reply->error();
    if (networkError == QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Usefulness submitted successfully";
    } else {
        qCWarning(LIBDISCOVER_LOG).noquote() << "OdrsReviewsBackend: Failed to submit usefulness:" << reply->errorString();
        Q_EMIT error(i18n("Error while submitting usefulness: %1", reply->errorString()));
    }
    reply->deleteLater();
}

QString OdrsReviewsBackend::userName() const
{
    return KUser().property(KUser::FullName).toString();
}

ReviewsJob *
OdrsReviewsBackend::sendReview(AbstractResource *resource, const QString &summary, const QString &reviewText, const QString &rating, const QString &userName)
{
    Q_ASSERT(resource);
    QJsonObject map = {
        {QLatin1StringView("app_id"), resource->appstreamId()},
        {QLatin1StringView("user_skey"), resource->getMetadata(QLatin1StringView("ODRS::user_skey")).toString()},
        {QLatin1StringView("user_hash"), userHash()},
        {QLatin1StringView("version"), resource->isInstalled() ? resource->installedVersion() : resource->availableVersion()},
        {QLatin1StringView("locale"), QLocale::system().name()},
        {QLatin1StringView("distro"), osName()},
        {QLatin1StringView("user_display"), QJsonValue::fromVariant(userName)},
        {QLatin1StringView("summary"), summary},
        {QLatin1StringView("description"), reviewText},
        {QLatin1StringView("rating"), rating.toInt() * 10},
    };

    const QJsonDocument document(map);

    const auto accessManager = nam();
    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/submit")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1StringView("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    // Store what we need so we can immediately show our review once it is submitted
    // Use review_id 0 for now as odrs starts numbering from 1 and once reviews are re-downloaded we get correct id
    map.insert(QLatin1StringView("review_id"), 0);
    resource->addMetadata(QLatin1StringView("ODRS::review_map"), map);
    request.setOriginatingObject(resource);

    auto reply = accessManager->post(request, document.toJson());
    return new OdrsSubmitReviewsJob(reply, resource);
}

void OdrsReviewsBackend::parseRatings()
{
    auto fw = new QFutureWatcher<State>(this);
    connect(fw, &QFutureWatcher<State>::finished, this, [this, fw] {
        fw->deleteLater();
        m_current = fw->result();
        Q_EMIT ratingsReady();
    });
    fw->setFuture(QtConcurrent::run([]() -> State {
        QFile ratingsDocument(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1StringView("/ratings/ratings"));
        if (!ratingsDocument.open(QIODevice::ReadOnly)) {
            qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Could not open file" << ratingsDocument.fileName();
            return {};
        }

        QJsonParseError error;
        const auto jsonDocument = QJsonDocument::fromJson(ratingsDocument.readAll(), &error);
        if (error.error) {
            qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Error parsing ratings:" << ratingsDocument.errorString() << error.errorString();
        }

        State state;
        const auto jsonObject = jsonDocument.object();
        state.ratings.reserve(jsonObject.size());
        constexpr uint topSize = 25;
        for (auto it = jsonObject.begin(); it != jsonObject.end(); it++) {
            const auto appJsonObject = it.value().toObject();
            const auto packageName = it.key().toLower();

            const int ratingCount = appJsonObject.value(QLatin1String("total")).toInt();
            int ratingMap[] = {
                appJsonObject.value(QLatin1String("star0")).toInt(),
                appJsonObject.value(QLatin1String("star1")).toInt(),
                appJsonObject.value(QLatin1String("star2")).toInt(),
                appJsonObject.value(QLatin1String("star3")).toInt(),
                appJsonObject.value(QLatin1String("star4")).toInt(),
                appJsonObject.value(QLatin1String("star5")).toInt(),
            };

            const auto rating = Rating(packageName, ratingCount, ratingMap);
            const auto finder = [&rating](const Rating &review) {
                return review.ratingCount() < rating.ratingCount();
            };
            const auto topIt = std::find_if(state.top.begin(), state.top.end(), finder);
            if (topIt == state.top.end()) {
                if (state.top.size() < topSize) {
                    state.top.append(rating);
                }
            } else {
                state.top.insert(topIt, rating);
            }
            if (state.top.size() > topSize) {
                state.top.resize(topSize);
            }

            state.ratings.insert(packageName, rating);
        }

        // Filter out non-apps, to match behavior of lists backed by ResourcesProxyModel
        AppStream::Pool appstreamData;
        appstreamData.load();

        QMutableListIterator<Rating> iterator(state.top);
        while (iterator.hasNext()) {
            const auto components = appstreamData.componentsById(iterator.next().packageName());

            for (const auto &component : components) {
                const bool isDesktopApp = component.kind() == AppStream::Component::KindDesktopApp;
                const bool isLaunchable = component.launchable(AppStream::Launchable::KindDesktopId).entries().count() > 0;
                if (!isDesktopApp || !isLaunchable) {
                    iterator.remove();
                }
            }
        }

        return state;
    }));
}

bool OdrsReviewsBackend::isResourceSupported(AbstractResource *resource) const
{
    return !resource->appstreamId().isEmpty();
}

void OdrsReviewsBackend::emitRatingFetched(AbstractResourcesBackend *backend, const QList<AbstractResource *> &resources) const
{
    backend->emitRatingsReady();
    for (const auto resource : resources) {
        if (m_current.ratings.contains(resource->appstreamId())) {
            Q_EMIT resource->ratingFetched();
        }
    }
}

QNetworkAccessManager *OdrsReviewsBackend::nam()
{
    if (!m_delayedNam) {
        m_delayedNam = new CachedNetworkAccessManager(QLatin1StringView("odrs"), this);
    }
    return m_delayedNam;
}

#include "moc_OdrsReviewsBackend.cpp"
