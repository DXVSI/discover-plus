// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include "RefreshNotifier.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <UpdateModel/RefreshNotifierDBus.h>
#include <resources/ResourcesModel.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

RefreshNotifier::RefreshNotifier(QObject *parent)
    : QObject(parent)
{
    // Listen to broadcasts from the notifier about cache changes.
    // Note that this is only sent when the user clicks Refresh on the Notifier. Not when the cache actually changes. This is to prevent automatic cache
    // changes from trashing the current UpdateModel view.
    QDBusConnection::sessionBus()
        .connect(QString(), RefreshNotifierDBus::path, RefreshNotifierDBus::interface, RefreshNotifierDBus::notifyApp, this, SLOT(onNotify()));

    m_timer.setInterval(2s); // arbitrary short time within which we compress notifications
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [] {
        RefreshNotifierDBus::notify(RefreshNotifierDBus::notifyNotifier);
    });
    connect(ResourcesModel::global(), &ResourcesModel::updatesCountChanged, this, [this] {
        m_timer.start();
    });
}

RefreshNotifier::~RefreshNotifier()
{
    // Force an update of the notifier on destruction to ensure any changes have been picked up
    RefreshNotifierDBus::notify(RefreshNotifierDBus::notifyNotifier);
}

void RefreshNotifier::onNotify()
{
    ResourcesModel::global()->checkForUpdates();
}

#include "moc_RefreshNotifier.cpp"
