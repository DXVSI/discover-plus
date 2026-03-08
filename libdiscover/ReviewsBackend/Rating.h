/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QVariant>

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT Rating
{
    Q_GADGET
    Q_PROPERTY(double sortableRating READ sortableRating CONSTANT)
    Q_PROPERTY(float rating READ rating CONSTANT)
    Q_PROPERTY(int ratingPoints READ ratingPoints CONSTANT)
    Q_PROPERTY(quint64 ratingCount READ ratingCount CONSTANT)
    Q_PROPERTY(std::vector<int> starCounts READ starCounts CONSTANT)
public:
    Rating()
    {
    }
    explicit Rating(const QString &packageName, quint64 ratingCount, int rating);
    explicit Rating(const QString &packageName, quint64 ratingCount, int data[6]);
    ~Rating();

    QString packageName() const;
    quint64 ratingCount() const;
    // 0.0 - 10.0 ranged rating
    float rating() const;
    int ratingPoints() const;
    // Returns a dampened rating calculated with the Wilson Score Interval algorithm
    double sortableRating() const;

    // Returns std::vector<int> where:
    // - m_starCounts[0] returns total count for "star0" ratings
    // - m_starCounts[1] returns total count for "star1" ratings
    // - And so on.
    // The range of ratings is 0 to 5.
    // "star0" ratings seem to be completely unused (one can inspect the ratings file)
    // and can be ignored.
    const std::vector<int> &starCounts() const;

private:
    QString m_packageName;
    quint64 m_ratingCount = 0;
    float m_rating = 0;
    int m_ratingPoints = 0;
    double m_sortableRating = 0;
    std::vector<int> m_starCounts;
};
