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
import QtQuick.Effects
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

    // Set proper card size
    implicitWidth: Kirigami.Units.gridUnit * 10
    implicitHeight: Kirigami.Units.gridUnit * 14  // Увеличили высоту карточек

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

    // Modern card background
    background: Rectangle {
        radius: Kirigami.Units.largeSpacing
        color: Kirigami.Theme.backgroundColor

        // Gradient overlay
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            opacity: 0.3
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: {
                        var category = root.application.categoryDisplay?.toLowerCase() || ""
                        if (category.includes("game")) return "#e74c3c"
                        if (category.includes("development")) return "#3498db"
                        if (category.includes("graphics")) return "#9b59b6"
                        if (category.includes("multimedia")) return "#f39c12"
                        return Kirigami.Theme.highlightColor
                    }
                }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // Border
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

        // Shadow
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: hoverHandler.hovered ? 6 : 2
            shadowBlur: hoverHandler.hovered ? 1.0 : 0.5
            shadowOpacity: hoverHandler.hovered ? 0.3 : 0.15
            shadowColor: Qt.rgba(0, 0, 0, 0.5)

            Behavior on shadowVerticalOffset {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on shadowBlur {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
        }

        // Hover effect background
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Kirigami.Theme.hoverColor
            opacity: hoverHandler.hovered ? 0.1 : 0
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }

    // Hover handler for animations
    HoverHandler {
        id: hoverHandler
    }

    // Remove scale animation to prevent jumping

    content: Item {
        anchors.fill: parent

        // Installed indicator
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.smallSpacing
            width: installedIcon.width + Kirigami.Units.smallSpacing * 2
            height: installedIcon.height + Kirigami.Units.smallSpacing * 2
            radius: width / 2
            visible: root.application.isInstalled
            z: 2
            color: "#2ecc71"

            Kirigami.Icon {
                id: installedIcon
                anchors.centerIn: parent
                source: "checkmark"
                width: Kirigami.Units.iconSizes.small
                height: width
                color: "white"
            }
        }

        // Category badge
        Rectangle {
            id: categoryBadge
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.smallSpacing
            width: categoryLabel.width + Kirigami.Units.largeSpacing
            height: categoryLabel.height + Kirigami.Units.smallSpacing
            radius: height / 2
            visible: (root.application.categoryDisplay ? root.application.categoryDisplay.length > 0 : false)
            z: 1

            color: {
                var category = root.application.categoryDisplay?.toLowerCase() || ""
                if (category.includes("game")) return "#e74c3c"
                if (category.includes("development")) return "#3498db"
                if (category.includes("graphics")) return "#9b59b6"
                if (category.includes("multimedia")) return "#f39c12"
                if (category.includes("office")) return "#2ecc71"
                return Kirigami.Theme.highlightColor
            }

            QQC2.Label {
                id: categoryLabel
                anchors.centerIn: parent
                text: {
                    var category = root.application.categoryDisplay || ""
                    if (category.length > 15) {
                        return category.substring(0, 12) + "..."
                    }
                    return category
                }
                color: "white"
                font.pixelSize: Kirigami.Units.gridUnit * 0.5
                font.bold: true
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            // Icon container with background
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge + Kirigami.Units.largeSpacing * 2

                Rectangle {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.huge + Kirigami.Units.largeSpacing
                    height: width
                    radius: width / 2
                    color: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.1)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        implicitWidth: Kirigami.Units.iconSizes.huge
                        implicitHeight: Kirigami.Units.iconSizes.huge
                        source: root.application.icon
                        animated: false
                    }
                }
            }

            // App name
            Kirigami.Heading {
                id: head
                level: 3
                type: Kirigami.Heading.Type.Primary
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignBottom
                wrapMode: Text.Wrap
                maximumLineCount: 2
                text: root.application.name
                font.bold: true
                elide: Text.ElideRight
            }

            // App description
            QQC2.Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: implicitHeight
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                opacity: 0.6
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                text: root.application.comment
                font: Kirigami.Theme.smallFont
            }

            // Rating
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: ratingRow.implicitHeight
                visible: root.application.rating?.ratingCount > 0

                RowLayout {
                    id: ratingRow
                    anchors.centerIn: parent
                    spacing: Kirigami.Units.smallSpacing

                    Rating {
                        value: root.application.rating?.sortableRating || 0
                        starSize: Kirigami.Units.gridUnit * 0.8
                        precision: Rating.Precision.HalfStar
                        padding: 0
                    }

                    QQC2.Label {
                        text: "(" + (root.application.rating?.ratingCount || 0) + ")"
                        opacity: 0.5
                        font: Kirigami.Theme.smallFont
                    }
                }
            }

            // Spacer to push button to bottom
            Item {
                Layout.fillHeight: true
            }

            // Install/Remove button
            InstallApplicationButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: parent.width * 0.8
                Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
                visible: true
                opacity: hoverHandler.hovered || root.application.isInstalled ? 1.0 : 0.3
                application: root.application

                Behavior on opacity {
                    NumberAnimation { duration: 150 }
                }
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
