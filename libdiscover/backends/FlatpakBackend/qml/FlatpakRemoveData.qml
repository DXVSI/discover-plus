/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    id: root

    required property Discover.AbstractResource resource
    Discover.Activatable.active: resource.hasData && query.count.number === 0

    Layout.fillWidth: true
    text: i18nd("libdiscover", "%1 is not installed but it still has user data and settings present.", resource.name)
    height: visible ? implicitHeight : 0

    Discover.ResourcesProxyModel {
        id: query
        backendFilter: root.resource.backend
        resourcesUrl: "flatpak:" + root.resource.ref
        stateFilter: Discover.AbstractResource.Installed

        readonly property var p0: Connections {
            target: root.resource
            function onStateChanged() {
                query.invalidateFilter()
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            actions[0].enabled = true
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "trash-empty-symbolic"
            text: i18nd("libdiscover", "Move to Trash")
            onTriggered: {
                enabled = false
                root.text = i18nd("libdiscover", "Clearing settings and user data…")
                root.resource.clearUserData()
            }
        }
    ]
}
