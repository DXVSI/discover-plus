/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.Material
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami
import "." as Local

Local.MaterialCard {
    id: root

    required property Discover.AbstractResource application
    required property int index
    required property int count

    property int numberItemsOnPreviousLastRow: 0
    property int columns: 2
    property int maxUp: columns*2
    property int maxDown: columns*2

    elevation: 1
    interactive: true

    // Force dark theme for consistency
    Material.theme: Material.Dark
    Material.background: "#1C1B1F"
    Material.foreground: "#E6E1E5"

    // Don't let RowLayout affect parent GridLayout's decisions, or else it
    // would resize cells proportionally to their label text length.
    implicitWidth: 0

    activeFocusOnTab: true
    Accessible.name: application ? application.name : ""
    Accessible.role: Accessible.Link
    Keys.onPressed: (event) => {
        if (((Qt.application.layoutDirection == Qt.LeftToRight && event.key == Qt.Key_Left) ||
             (Qt.application.layoutDirection == Qt.RightToLeft && event.key == Qt.Key_Right)) &&
             (index % columns > 0)){
            nextItemInFocusChain(false).forceActiveFocus()
            event.accepted = true
        } else if (((Qt.application.layoutDirection == Qt.LeftToRight && event.key == Qt.Key_Right) ||
                   (Qt.application.layoutDirection == Qt.RightToLeft && event.key == Qt.Key_Left))  &&
                   (index % columns != columns -1) && (index +1 != count)) {
            nextItemInFocusChain(true).forceActiveFocus()
            event.accepted = true
        }
    }
    Keys.onUpPressed: {
        var target = this
        var extramoves = 0
        if (index < columns) {
            extramoves = (index < numberItemsOnPreviousLastRow)
                         ? numberItemsOnPreviousLastRow - columns
                         : numberItemsOnPreviousLastRow
        }

        for (var i = 0; i<Math.min(columns+extramoves,index+maxUp); i++) {
            target = target.nextItemInFocusChain(false)
        }
        target.forceActiveFocus(Qt.TabFocusReason)
    }
    Keys.onDownPressed: {
        var target = this
        var extramoves = 0
        if (index + columns >= count) {
            extramoves = ((index % columns) < (count % columns) )
                         ? (count % columns) - columns // directly up
                         : (count % columns) // skip a line
        }
        for (var i = 0; i<Math.min(columns+extramoves, count - index + maxDown -1); i++) {
            target = target.nextItemInFocusChain(true)
        }
        target.forceActiveFocus(Qt.TabFocusReason)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 80

            Rectangle {
                anchors.centerIn: parent
                width: 64
                height: 64
                radius: 12
                color: "#2B2A2E"

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: 48
                    height: 48
                    source: root.application ? root.application.icon : ""
                    animated: false
                    // Prevent icon from becoming monochrome
                    isMask: false
                    // Disable theme color application
                    Kirigami.Theme.inherit: false
                    color: "transparent"
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            QQC2.Label {
                id: head
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
                font.weight: Font.Medium
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: root.application ? root.application.name : ""
            }

            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
                opacity: 0.7
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: root.application ? (root.application.comment || "") : ""
            }

            // Source/Origin info
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 16
                spacing: 4

                Item { Layout.fillWidth: true }

                Kirigami.Icon {
                    visible: root.application && root.application.origin
                    source: root.application ? root.application.sourceIcon : ""
                    Layout.preferredWidth: 14
                    Layout.preferredHeight: 14
                    opacity: 0.6
                }

                QQC2.Label {
                    visible: root.application && root.application.origin
                    text: root.application ? root.application.origin : ""
                    font.pixelSize: 10
                    opacity: 0.6
                    elide: Text.ElideRight
                }

                Item { Layout.fillWidth: true }
            }

            // Install/Remove button
            Local.InstallApplicationButton {
                Layout.fillWidth: true
                Layout.topMargin: 4
                visible: root.application
                application: root.application
                installOrRemoveButtonDisplayStyle: QQC2.AbstractButton.IconOnly
            }
        }
    }

    onClicked: {
        if (root.application) {
            console.log("GridApplicationDelegate clicked:", root.application.name)
            Local.Navigation.openApplication(root.application)
        }
    }

    onFocusChanged: {
        if (focus && typeof page !== 'undefined' && page && page.ensureVisible) {
            page.ensureVisible(root)
        }
    }
}
