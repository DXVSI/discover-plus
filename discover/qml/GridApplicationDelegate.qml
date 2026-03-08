/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

BasicAbstractCard {
    id: root

    required property Discover.AbstractResource application
    required property int index
    required property int count

    property int numberItemsOnPreviousLastRow: 0
    property int columns: 2
    property int maxUp: columns*2
    property int maxDown: columns*2

    showClickFeedback: true

    // Don't let RowLayout affect parent GridLayout's decisions, or else it
    // would resize cells proportionally to their label text length.
    implicitWidth: 0

    activeFocusOnTab: true
    highlighted: focus
    Accessible.name: application.name
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

    content: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignVCenter
            Layout.margins: Kirigami.Units.largeSpacing
            implicitWidth: Kirigami.Units.iconSizes.huge
            implicitHeight: Kirigami.Units.iconSizes.huge
            source: root.application.icon
            animated: false
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Kirigami.Heading {
                id: head
                level: 2
                type: Kirigami.Heading.Type.Primary
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom
                wrapMode: Text.Wrap
                maximumLineCount: 2

                text: root.application.name
            }

            QQC2.Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                maximumLineCount: head.lineCount === 1 ? 3 : 2
                opacity: 0.75
                wrapMode: Text.Wrap

                text: root.application.comment
            }
        }
    }

    onClicked: Navigation.openApplication(root.application)
    onFocusChanged: {
        if (focus) {
            page.ensureVisible(root)
        }
    }
}
