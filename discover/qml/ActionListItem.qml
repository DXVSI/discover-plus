/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.ItemDelegate {
    id: item

    Layout.fillWidth: true
    Layout.leftMargin: Kirigami.Units.smallSpacing
    Layout.rightMargin: Kirigami.Units.smallSpacing

    highlighted: checked
    visible: enabled
    activeFocusOnTab: true

    implicitHeight: 42

    property string subtitle
    property string stateIconName

    // Category colors based on icon name
    readonly property color categoryColor: {
        var iconName = item.icon.name.toLowerCase()
        if (iconName.indexOf("game") >= 0) return "#E74C3C"
        if (iconName.indexOf("internet") >= 0 || iconName.indexOf("web") >= 0) return "#3498DB"
        if (iconName.indexOf("multimedia") >= 0 || iconName.indexOf("audio") >= 0 || iconName.indexOf("video") >= 0) return "#E91E63"
        if (iconName.indexOf("graphic") >= 0 || iconName.indexOf("image") >= 0) return "#9B59B6"
        if (iconName.indexOf("office") >= 0 || iconName.indexOf("document") >= 0) return "#F39C12"
        if (iconName.indexOf("develop") >= 0 || iconName.indexOf("code") >= 0) return "#1ABC9C"
        if (iconName.indexOf("education") >= 0 || iconName.indexOf("science") >= 0) return "#2ECC71"
        if (iconName.indexOf("system") >= 0 || iconName.indexOf("util") >= 0) return "#6C757D"
        if (iconName.indexOf("add") >= 0 || iconName.indexOf("addon") >= 0) return "#8E44AD"
        return "#5DADE2"
    }

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
            if (item.highlighted) {
                return item.categoryColor
            } else if (item.hovered) {
                return Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, item.categoryColor, 0.12)
            }
            return "transparent"
        }

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        // Colored icon square
        Rectangle {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            radius: 8
            color: item.highlighted ? Qt.rgba(1, 1, 1, 0.2) : item.categoryColor
            opacity: item.highlighted ? 1 : 0.15

            Kirigami.Icon {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: item.icon.name
                color: item.highlighted ? "white" : item.categoryColor
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: item.text
            font.weight: item.highlighted ? Font.DemiBold : Font.Normal
            font.pixelSize: 13
            color: item.highlighted ? "white" : Kirigami.Theme.textColor
            elide: Text.ElideRight
        }

        Kirigami.Icon {
            visible: item.stateIconName.length > 0
            source: item.stateIconName
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            color: item.highlighted ? "white" : Kirigami.Theme.textColor
        }

        // Arrow for expandable
        Kirigami.Icon {
            visible: item.action && item.action.children && item.action.children.length > 0
            source: "arrow-right"
            Layout.preferredWidth: 12
            Layout.preferredHeight: 12
            color: item.highlighted ? "white" : Kirigami.Theme.disabledTextColor
        }
    }

    Kirigami.MnemonicData.enabled: item.enabled && item.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.label: action.text

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
