/*
 * SPDX-FileCopyrightText: 2018-2019 Red Hat Inc
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018-2019 Jan Grulich <jgrulich@redhat.com>
 */

#pragma once

#include <QDBusArgument>
#include <QMap>
#include <QString>

/// a{sa{sv}}
using VariantMapMap = QMap<QString, QMap<QString, QVariant>>;
Q_DECLARE_METATYPE(VariantMapMap)

/// (si)
typedef struct s_HTTPProxy {
    QString address;
    qint32 port;
} HTTPProxy;
Q_DECLARE_METATYPE(HTTPProxy)
