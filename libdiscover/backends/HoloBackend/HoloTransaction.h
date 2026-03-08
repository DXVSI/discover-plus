/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef HOLOTRANSACTION_H
#define HOLOTRANSACTION_H

#include <QPointer>
#include <Transaction/Transaction.h>

#include "atomupd1.h"

class QTimer;
class HoloResource;
class HoloTransaction : public Transaction
{
    Q_OBJECT
public:
    HoloTransaction(HoloResource *app, Role role, ComSteampoweredAtomupd1Interface *interface);

    void cancel() override;

Q_SIGNALS:
    void needReboot();

private Q_SLOTS:
    void refreshStatus();

private:
    void finishTransaction(bool installed);

    HoloResource *const m_app;
    QPointer<ComSteampoweredAtomupd1Interface> m_interface; // Interface to atomupd dbus api
};

#endif // HOLOTRANSACTION_H
