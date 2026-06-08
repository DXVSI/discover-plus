/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SysupdateInternal.h"
#include <QMetaEnum>
#include <qdbusargument.h>
#include <qdbusmetatype.h>

QDBusArgument &operator<<(QDBusArgument &argument, const Sysupdate::Job &job)
{
    argument.beginStructure();
    argument << job.id << static_cast<int>(job.type) << job.progressPercent << job.objectPath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Sysupdate::Job &job)
{
    int type;
    argument.beginStructure();
    argument >> job.id >> type >> job.progressPercent >> job.objectPath;
    job.type = static_cast<Sysupdate::JobType>(type);
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(Sysupdate::Job)
Q_DECLARE_METATYPE(Sysupdate::JobList)

QDBusArgument &operator<<(QDBusArgument &argument, const Sysupdate::Target &target)
{
    argument.beginStructure();
    argument << target.targetClass << target.name << target.objectPath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Sysupdate::Target &target)
{
    argument.beginStructure();
    argument >> target.targetClass >> target.name >> target.objectPath;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(Sysupdate::Target)
Q_DECLARE_METATYPE(Sysupdate::TargetList)

#include "moc_SysupdateInternal.cpp"
