/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <qdbusargument.h>
#include <qdbusextratypes.h>
#include <qtypes.h>

namespace Sysupdate
{
Q_NAMESPACE

enum class JobType {
    JOB_LIST,
    JOB_DESCRIBE,
    JOB_CHECK_NEW,
    JOB_UPDATE,
    JOB_VACUUM,
};

struct Job {
    quint64 id;
    JobType type;
    quint32 progressPercent;
    QDBusObjectPath objectPath;
};

struct Target {
    QString targetClass;
    QString name;
    QDBusObjectPath objectPath;
};

struct TargetInfo {
    QString installedVersion;
    QString availableVersion;
    bool partial;
};

typedef QList<Job> JobList;
typedef QList<Target> TargetList;
}

QDBusArgument &operator<<(QDBusArgument &argument, const Sysupdate::Target &target);
const QDBusArgument &operator>>(const QDBusArgument &argument, Sysupdate::Target &target);
