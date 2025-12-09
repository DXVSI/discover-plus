/*
 *   SPDX-FileCopyrightText: 2025 Discover Plus Contributors
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.ItemDelegate {
    id: root

    property color iconColor: Kirigami.Theme.highlightColor
    property int badge: 0

    Layout.fillWidth: true
    Layout.leftMargin: Kirigami.Units.smallSpacing
    Layout.rightMargin: Kirigami.Units.smallSpacing

    highlighted: checked
    visible: enabled
    activeFocusOnTab: true

    implicitHeight: 44

    Keys.onEnterPressed: trigger()
    Keys.onReturnPressed: trigger()

    function trigger() {
        if (enabled) {
            if (typeof drawer !== "undefined") {
                drawer.resetMenu()
            }
            action.trigger()
        }
    }

    background: Rectangle {
        radius: 10
        color: {
            if (root.highlighted) {
                return root.iconColor
            } else if (root.hovered) {
                return Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, root.iconColor, 0.12)
            }
            return "transparent"
        }

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        // Colored icon background
        Rectangle {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            radius: 8
            color: root.highlighted ? Qt.rgba(1, 1, 1, 0.2) : root.iconColor
            opacity: root.highlighted ? 1 : 0.15

            Kirigami.Icon {
                anchors.centerIn: parent
                width: 18
                height: 18
                source: root.icon.name
                color: root.highlighted ? "white" : root.iconColor
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: root.text
            font.weight: root.highlighted ? Font.DemiBold : Font.Normal
            font.pixelSize: 14
            color: root.highlighted ? "white" : Kirigami.Theme.textColor
            elide: Text.ElideRight
        }

        // Badge for updates count
        Rectangle {
            visible: root.badge > 0
            Layout.preferredWidth: Math.max(20, badgeLabel.implicitWidth + 10)
            Layout.preferredHeight: 20
            radius: 10
            color: root.highlighted ? Qt.rgba(1, 1, 1, 0.3) : "#E74C3C"

            QQC2.Label {
                id: badgeLabel
                anchors.centerIn: parent
                text: root.badge > 99 ? "99+" : root.badge
                font.pixelSize: 11
                font.weight: Font.Bold
                color: "white"
            }
        }
    }

    Kirigami.MnemonicData.enabled: root.enabled && root.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.label: action ? action.text : ""

    text: Kirigami.MnemonicData.richTextLabel

    QQC2.ToolTip.text: action ? action.text : ""
    QQC2.ToolTip.visible: hovered && QQC2.ToolTip.text.length > 0
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    onFocusChanged: {
        if (focus && typeof drawer !== "undefined") {
            drawer.ensureVisible(root)
        }
    }

    Keys.onPressed: event => {
        if (event.accepted) return
        if (event.key === Qt.Key_Up) {
            nextItemInFocusChain(false).forceActiveFocus()
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            nextItemInFocusChain(true).forceActiveFocus()
            event.accepted = true
        }
    }
}
