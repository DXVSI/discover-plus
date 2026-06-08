/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OdrsReviewsJob.h"
#include <ReviewsBackend/Review.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <resources/AbstractResource.h>

#include <KLocalizedString>
#include <QJsonDocument>

#include "libdiscover_debug.h"

OdrsSubmitReviewsJob::OdrsSubmitReviewsJob(QNetworkReply *reply, AbstractResource *resource)
    : OdrsReviewsJob(reply, resource)
{
    connect(reply, &QNetworkReply::finished, this, &OdrsSubmitReviewsJob::reviewSubmitted);
}

void OdrsSubmitReviewsJob::reviewSubmitted()
{
    const auto networkError = m_reply->error();
    if (networkError == QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Review submitted for" << m_resource;
        if (m_resource) {
            const QJsonDocument document({m_resource->getMetadata(QLatin1StringView("ODRS::review_map")).toObject()});
            parseReviews(document);
        } else {
            qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Failed to submit review: missing object";
        }
    } else {
        qCWarning(LIBDISCOVER_LOG).noquote() << "OdrsReviewsBackend: Failed to submit review:" << m_reply->error() << m_reply->errorString()
                                             << m_reply->rawHeaderPairs();
        Q_EMIT errorMessage(i18n("Error while submitting review: %1", m_reply->errorString()));
    }
}

OdrsReviewsJob::OdrsReviewsJob(QNetworkReply *reply, AbstractResource *resource)
    : m_reply(reply)
    , m_resource(resource)
{
    Q_ASSERT(m_reply);
    Q_ASSERT(m_resource);
}

OdrsReviewsJob *OdrsReviewsJob::create(QNetworkReply *reply, AbstractResource *resource)
{
    auto r = new OdrsReviewsJob(reply, resource);
    connect(reply, &QNetworkReply::finished, r, &OdrsReviewsJob::reviewsFetched);
    return r;
}

OdrsReviewsJob::~OdrsReviewsJob()
{
    delete m_reply;
}

void OdrsReviewsJob::reviewsFetched()
{
    const QByteArray data = m_reply->readAll();
    const auto networkError = m_reply->error();
    if (networkError != QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Error fetching reviews:" << m_reply->errorString() << data;
        Q_EMIT errorMessage(i18n("Technical error message: %1", m_reply->errorString()));
        Q_EMIT reviewsReady({}, false);
        deleteLater();
        return;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if (error.error) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Error parsing reviews:" << m_reply->url() << error.errorString();
    }
    parseReviews(document);
    deleteLater();
}

void OdrsReviewsJob::parseReviews(const QJsonDocument &document)
{
    const auto reviews = document.array();
    QList<ReviewPtr> reviewsList;
    for (const auto &it : reviews) {
        const QJsonObject review = it.toObject();
        if (!review.isEmpty()) {
            // Same ranking algorithm Gnome Software uses
            const int usefulFavorable = review.value(QLatin1StringView("karma_up")).toInt();
            const int usefulNegative = review.value(QLatin1StringView("karma_down")).toInt();
            const int usefulTotal = usefulFavorable + usefulNegative;

            qreal usefulWilson = 0.0;

            /* from http://www.evanmiller.org/how-not-to-sort-by-average-rating.html */
            if (usefulFavorable > 0 || usefulNegative > 0) {
                usefulWilson =
                    ((usefulFavorable + 1.9208) / (usefulFavorable + usefulNegative)
                     - 1.96 * sqrt((usefulFavorable * usefulNegative) / qreal(usefulFavorable + usefulNegative) + 0.9604) / (usefulFavorable + usefulNegative))
                    / (1 + 3.8416 / (usefulFavorable + usefulNegative));
                usefulWilson *= 100.0;
            }

            QDateTime dateTime;
            dateTime.setSecsSinceEpoch(review.value(QLatin1StringView("date_created")).toInt());

            // If there is no score or the score is the same, base on the age
            const auto currentDateTime = QDateTime::currentDateTime();
            const auto totalDays = static_cast<qreal>(dateTime.daysTo(currentDateTime));

            // use also the longest common subsequence of the version string to compute relevance
            const auto reviewVersion = review.value(QLatin1StringView("version")).toString();
            const auto availableVersion = m_resource->availableVersion();
            qreal versionScore = 0;
            const int minLength = std::min(reviewVersion.length(), availableVersion.length());
            if (minLength > 0) {
                for (int i = 0; i < minLength; ++i) {
                    if (reviewVersion[i] != availableVersion[i] || i == minLength - 1) {
                        versionScore = i;
                        break;
                    }
                }
                // Normalize
                versionScore = versionScore / qreal(std::max(reviewVersion.length(), availableVersion.length()) - 1);
            }

            // Very random heuristic which weights usefulness with age and version similarity. Don't penalize usefulness more than 6 months
            usefulWilson = versionScore + 1.0 / std::max(1.0, totalDays) + usefulWilson / std::clamp(totalDays, 1.0, 93.0);

            const bool shouldShow = usefulFavorable >= usefulNegative * 2 && review.value(QLatin1StringView("reported")).toInt() < 4;
            ReviewPtr r(new Review(review.value(QLatin1StringView("app_id")).toString(),
                                   m_resource->packageName(),
                                   review.value(QLatin1StringView("locale")).toString(),
                                   review.value(QLatin1StringView("summary")).toString(),
                                   review.value(QLatin1StringView("description")).toString(),
                                   review.value(QLatin1StringView("user_display")).toString(),
                                   dateTime,
                                   shouldShow,
                                   review.value(QLatin1StringView("review_id")).toInt(),
                                   review.value(QLatin1StringView("rating")).toInt() / 10,
                                   usefulTotal,
                                   usefulFavorable,
                                   usefulWilson,
                                   reviewVersion));
            // We can also receive just a json with app name and user info so filter these out as there is no review
            if (!r->summary().isEmpty() && !r->reviewText().isEmpty()) {
                reviewsList.append(r);
                // Needed for submitting usefulness
                r->addMetadata(QLatin1StringView("ODRS::user_skey"), review.value(QLatin1StringView("user_skey")).toString());
            }

            // We should get at least user_skey needed for posting reviews
            m_resource->addMetadata(QLatin1StringView("ODRS::user_skey"), review.value(QLatin1StringView("user_skey")).toString());
        }
    }

    Q_EMIT reviewsReady(reviewsList, false);
}

#include "moc_OdrsReviewsJob.cpp"
