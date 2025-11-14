/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects

import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

QQC2.ItemDelegate {
    id: item

    Layout.fillWidth: true
    Layout.leftMargin: Kirigami.Units.smallSpacing
    Layout.rightMargin: Kirigami.Units.smallSpacing
    Layout.topMargin: 2
    Layout.bottomMargin: 2

    highlighted: checked
    visible: enabled
    activeFocusOnTab: true

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

    property string subtitle
    property string stateIconName

    // Modern background with rounded corners
    background: Rectangle {
        radius: Kirigami.Units.smallSpacing
        color: {
            if (item.checked) {
                return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.2)
            } else if (item.hovered) {
                return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.1)
            } else {
                return "transparent"
            }
        }

        border.width: item.checked ? 1 : 0
        border.color: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.4)

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.InOutQuad }
        }

        // Subtle left indicator for active item
        Rectangle {
            visible: item.checked
            width: 3
            height: parent.height - Kirigami.Units.smallSpacing * 2
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            radius: width / 2
            color: Kirigami.Theme.highlightColor
        }
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        // Icon with background circle
        Rectangle {
            Layout.preferredWidth: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing
            Layout.preferredHeight: Layout.preferredWidth
            radius: width / 2
            color: item.checked ? Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.15) : "transparent"

            Kirigami.Icon {
                anchors.centerIn: parent
                source: item.icon.name || item.icon.source || ""
                width: Kirigami.Units.iconSizes.small
                height: width
                color: item.checked ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
            }
        }

        KD.IconTitleSubtitle {
            Layout.fillWidth: true
            title: item.text
            subtitle: item.subtitle
            selected: item.highlighted || item.pressed
            font.bold: item.checked
        }

        // State icon with animation
        Kirigami.Icon {
            Layout.fillHeight: true
            visible: item.stateIconName.length > 0
            source: item.stateIconName
            selected: item.highlighted || item.pressed
            implicitWidth: Kirigami.Units.iconSizes.sizeForLabels
            implicitHeight: Kirigami.Units.iconSizes.sizeForLabels

            RotationAnimation on rotation {
                running: item.stateIconName === "view-refresh"
                from: 0
                to: 360
                duration: 1000
                loops: Animation.Infinite
            }
        }
    }

    Kirigami.MnemonicData.enabled: item.enabled && item.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.label: action.text

    // Note changing text here does not affect the action.text
    text: Kirigami.MnemonicData.richTextLabel

    QQC2.ToolTip.text: shortcut.nativeText
    QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? down : hovered) && QQC2.ToolTip.text.length > 0
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    Shortcut {
        id: shortcut
        sequence: item.Kirigami.MnemonicData.sequence
        onActivated: item.trigger()
    }

    onFocusChanged: {
        if (focus) {
            drawer.ensureVisible(item)
        }
    }

    // Using the generic onPressed so individual instances can override
    // behaviour using Keys.on{Up,Down}Pressed
    Keys.onPressed: event => {
        if (event.accepted) {
            return
        }

        // Using forceActiveFocus here since the item may be in a focus scope
        // and just setting focus won't focus the scope.
        if (event.key === Qt.Key_Up) {
            nextItemInFocusChain(false).forceActiveFocus()
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            nextItemInFocusChain(true).forceActiveFocus()
            event.accepted = true
        }
    }
}
