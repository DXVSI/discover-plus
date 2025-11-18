/*
 *   SPDX-FileCopyrightText: 2024 KDE Developers
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami

QQC2.ItemDelegate {
    id: root

    default property alias content: contentLoader.sourceComponent
    property int elevation: 1
    property bool interactive: true

    // Material Design 3 styling
    Material.elevation: hovered ? elevation + 2 : elevation

    padding: 0

    background: Rectangle {
        radius: 12
        color: {
            // Always use dark theme colors
            switch(root.elevation) {
                case 0: return "#1C1B1F"  // Surface
                case 1: return "#201F23"  // Surface Container
                case 2: return "#242329"  // Surface Container + tint
                case 3: return "#27262B"  // Surface Container High
                case 4: return "#2B2A2E"  // Surface Container High
                default: return "#36343B" // Surface Container Highest
            }
        }

        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.05)

        layer.enabled: root.elevation > 0
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: Qt.rgba(0, 0, 0, 0.4)
            shadowBlur: root.elevation * 2
            shadowVerticalOffset: root.elevation
            shadowScale: 1.0
        }

        // Ripple effect on click
        Rectangle {
            id: ripple
            anchors.fill: parent
            radius: parent.radius
            color: "#D0BCFF"  // Always use dark theme ripple color
            opacity: 0

            NumberAnimation on opacity {
                id: rippleIn
                from: 0
                to: 0.1
                duration: 150
            }

            NumberAnimation on opacity {
                id: rippleOut
                from: 0.1
                to: 0
                duration: 300
            }
        }
    }

    contentItem: Loader {
        id: contentLoader
    }
}