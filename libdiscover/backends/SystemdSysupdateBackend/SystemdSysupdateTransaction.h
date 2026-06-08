/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QtTypes>
#include <Transaction/Transaction.h>
#include <sysupdate1.h>

constexpr auto SYSUPDATE1_SERVICE = QLatin1String("org.freedesktop.sysupdate1");
using SystemdSysupdateUpdateReply = QDBusPendingReply<QString, qulonglong, QDBusObjectPath>;

class SystemdSysupdateTransaction : public Transaction
{
public:
    SystemdSysupdateTransaction(AbstractResource *resource, org::freedesktop::sysupdate1::Target *target);

    void cancel() override;

private Q_SLOTS:
    // Called when downloading the files is done, and runs the install process if there is no errors
    void acquireDone(qulonglong jobId, const QDBusObjectPath &jobPath, int status);
    // Called when installing files is done, marks the transaction done or done with errors
    void installDone(qulonglong jobId, const QDBusObjectPath &jobPath, int status);

private:
    const AbstractResource *m_resource;
    org::freedesktop::sysupdate1::Target *m_target;
    org::freedesktop::sysupdate1::Job *m_job;
};
