/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "Rating.h"
#include "libdiscover_debug.h"
#include <QStringList>
#include <qmath.h>

inline double fastPow(double a, double b)
{
    union {
        double d;
        int x[2];
    } u = {a};

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
    u.x[0] = 0;
#else
    u.x[1] = 0;
    u.x[0] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
#endif

    return u.d;
}

// Converted from a Ruby example, returns an inverse normal distribution
double pnormaldist(double qn)
{
    double b[] = {1.570796288,
                  0.03706987906,
                  -0.8364353589e-3,
                  -0.2250947176e-3,
                  0.6841218299e-5,
                  0.5824238515e-5,
                  -0.104527497e-5,
                  0.8360937017e-7,
                  -0.3231081277e-8,
                  0.3657763036e-10,
                  0.6936233982e-12};

    if (qn < 0.0 || 1.0 < qn)
        return 0.0;

    if (qn == 0.5)
        return 0.0;

    double w1 = qn;
    if (qn > 0.5)
        w1 = 1.0 - w1;
    double w3 = -qLn(4.0 * w1 * (1.0 - w1));
    w1 = b[0];

    for (int i = 1; i < 11; i++)
        w1 += b[i] * fastPow(w3, i);

    if (qn > 0.5)
        return qSqrt(w1 * w3);
    return -qSqrt(w1 * w3);
}

double wilson_score(int pos, int n, double power = 0.2)
{
    if (n == 0)
        return 0;

    double z = pnormaldist(1 - power / 2);
    double phat = 1.0 * pos / n;
    return (phat + z * z / (2 * n) - z * qSqrt((phat * (1 - phat) + z * z / (4 * n)) / n)) / (1 + z * z / n);
}

double dampenedRating(int ratings[6], double power = 0.1)
{
    int tot_ratings = 0;
    for (int i = 0; i < 6; ++i)
        tot_ratings = ratings[i] + tot_ratings;

    double sum_scores = 0.0;
    for (int i = 0; i < 6; i++) {
        const int rating = ratings[i];
        double ws = wilson_score(rating, tot_ratings, power);
        sum_scores = sum_scores + float((i + 1) - 3) * ws;
    }

    return sum_scores + 3;
}

Rating::Rating(const QString &packageName, quint64 ratingCount, int data[6])
    : m_packageName(packageName)
    , m_ratingCount(ratingCount)
    // We ignore star0 ratings
    , m_rating(((data[1] //
                 + (data[2] * 2) //
                 + (data[3] * 3) //
                 + (data[4] * 4) //
                 + (data[5] * 5)) //
                * 2)
               / qMax<float>(1, ratingCount))
    , m_ratingPoints(0)
    , m_sortableRating(0)
    , m_starCounts({data[0], data[1], data[2], data[3], data[4], data[5]})
{
    int spread[6];
    for (int i = 0; i < 6; ++i) {
        int points = data[i];
        m_ratingPoints += (i + 1) * points;
        spread[i] = points;
    }

    m_sortableRating = dampenedRating(spread) * 2;
}

Rating::Rating(const QString &packageName, quint64 ratingCount, int rating)
    : m_packageName(packageName)
    , m_ratingCount(ratingCount)
    , m_rating(rating)
    , m_ratingPoints(rating)
    , m_sortableRating(rating)
{
}

Rating::~Rating() = default;

QString Rating::packageName() const
{
    return m_packageName;
}

quint64 Rating::ratingCount() const
{
    return m_ratingCount;
}

float Rating::rating() const
{
    return m_rating;
}

int Rating::ratingPoints() const
{
    return m_ratingPoints;
}

double Rating::sortableRating() const
{
    return m_sortableRating;
}

const std::vector<int> &Rating::starCounts() const
{
    return m_starCounts;
}

#include "moc_Rating.cpp"
