/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018-2021 Nate Graham <nate@kde.org>
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

    required property int index
    required property Discover.AbstractResource application

    property bool compact: false
    property bool showRating: true
    property bool showSize: false

    readonly property bool appIsFromNonDefaultBackend: Discover.ResourcesModel.currentApplicationBackend !== application.backend && application.backend.hasApplications
    showClickFeedback: true

    function trigger() {
        ListView.currentIndex = index
        Navigation.openApplication(application)
    }

    padding: Kirigami.Units.largeSpacing * 2
    highlighted: ListView.isCurrentItem

    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    // Modern card background with shadow
    background: Rectangle {
        radius: Kirigami.Units.largeSpacing
        color: {
            if (root.highlighted) {
                return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.2)
            } else if (hoverHandler.hovered) {
                return Qt.rgba(Kirigami.Theme.hoverColor.r, Kirigami.Theme.hoverColor.g, Kirigami.Theme.hoverColor.b, 0.1)
            } else {
                return Kirigami.Theme.backgroundColor
            }
        }

        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)
        border.width: 1

        // Smooth color transitions
        Behavior on color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        // Shadow effect
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: hoverHandler.hovered ? 4 : 2
            shadowBlur: hoverHandler.hovered ? 0.8 : 0.4
            shadowOpacity: hoverHandler.hovered ? 0.3 : 0.15
            shadowColor: Qt.rgba(0, 0, 0, 0.5)

            Behavior on shadowVerticalOffset {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on shadowBlur {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on shadowOpacity {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
        }
    }

    // Hover detection for effects
    HoverHandler {
        id: hoverHandler
    }

    // Scale animation on hover
    transform: Scale {
        id: scaleTransform
        origin.x: root.width / 2
        origin.y: root.height / 2
        xScale: hoverHandler.hovered ? 1.02 : 1.0
        yScale: hoverHandler.hovered ? 1.02 : 1.0

        Behavior on xScale {
            NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
        Behavior on yScale {
            NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
    }

    content: Item {
        implicitHeight: Math.max(columnLayout.implicitHeight, resourceIconFrame.implicitHeight)

        // Icon with modern styling
        Rectangle {
            id: resourceIconFrame
            anchors {
                top: parent.top
                left: parent.left
                bottom: parent.bottom
            }
            width: iconLoader.width + Kirigami.Units.largeSpacing * 2
            color: "transparent"
            radius: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                id: iconLoader
                anchors.centerIn: parent
                source: root.application.icon
                animated: false

                implicitHeight: root.compact ? Kirigami.Units.iconSizes.large : Kirigami.Units.iconSizes.huge
                implicitWidth: implicitHeight
            }
        }

        // Container for everything but the app icon
        ColumnLayout {
            id: columnLayout

            anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
                left: resourceIconFrame.right
                leftMargin: Kirigami.Units.gridUnit
            }
            spacing: Kirigami.Units.smallSpacing

            // Container for app name and backend name labels
            RowLayout {
                spacing: Kirigami.Units.largeSpacing

                // App name label with gradient effect
                Kirigami.Heading {
                    id: head
                    Layout.fillWidth: true
                    topPadding: headMetrics.boundingRect.y - headMetrics.tightBoundingRect.y
                    level: root.compact ? 2 : 1
                    type: Kirigami.Heading.Type.Primary
                    text: root.application.name
                    elide: Text.ElideRight
                    maximumLineCount: 1

                    TextMetrics {
                        id: headMetrics
                        font: head.font
                        text: head.text
                    }
                }

                // Backend name label with chip style
                Rectangle {
                    Layout.alignment: Qt.AlignRight
                    visible: !root.compact
                    implicitWidth: backendLayout.implicitWidth + Kirigami.Units.largeSpacing
                    implicitHeight: backendLayout.implicitHeight + Kirigami.Units.smallSpacing
                    radius: height / 2
                    color: {
                        if (root.application.origin === "COPR") {
                            return Qt.rgba(0.2, 0.6, 1, 0.15)
                        } else if (root.application.origin === "RPM Fusion") {
                            return Qt.rgba(0.8, 0.3, 0.3, 0.15)
                        } else {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.15)
                        }
                    }

                    RowLayout {
                        id: backendLayout
                        anchors.centerIn: parent
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: root.application.sourceIcon
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: root.application.origin
                            font: Kirigami.Theme.smallFont
                            color: {
                                if (root.application.origin === "COPR") {
                                    return Qt.rgba(0.2, 0.6, 1, 1)
                                } else if (root.application.origin === "RPM Fusion") {
                                    return Qt.rgba(0.8, 0.3, 0.3, 1)
                                } else {
                                    return Kirigami.Theme.highlightColor
                                }
                            }
                        }
                    }
                }
            }

            // Description/"Comment" label
            QQC2.Label {
                id: description
                Layout.fillWidth: true
                Layout.preferredHeight: descriptionMetrics.height
                text: root.application.comment
                elide: Text.ElideRight
                maximumLineCount: 1
                textFormat: Text.PlainText
                opacity: 0.7

                // reserve space for description even if none is available
                TextMetrics {
                    id: descriptionMetrics
                    font: description.font
                    text: "Sample text"
                }
            }

            // Container for rating, size, and install button
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                // Combined condition of both children items
                visible: root.showRating || (!root.compact && root.showSize) || !root.compact

                // Rating stars + label
                RowLayout {
                    id: rating
                    Layout.alignment: Qt.AlignBottom
                    visible: root.showRating
                    opacity: 0.8
                    spacing: Kirigami.Units.largeSpacing

                    Rating {
                        Layout.alignment: Qt.AlignVCenter
                        value: root.application.rating.sortableRating
                        starSize: root.compact ? description.font.pointSize : head.font.pointSize
                        precision: Rating.Precision.HalfStar
                        padding: 0
                    }
                    QQC2.Label {
                        Layout.alignment: Qt.AlignVCenter
                        topPadding: (ratingLabelMetrics.boundingRect.y - ratingLabelMetrics.tightBoundingRect.y)/2
                        visible: root.application.backend.reviewsBackend?.isResourceSupported(root.application) ?? false
                        text: root.application.rating.ratingCount > 0 ? i18np("%1 rating", "%1 ratings", root.application.rating.ratingCount) : i18n("No ratings yet")
                        font: Kirigami.Theme.smallFont

                        TextMetrics {
                            id: ratingLabelMetrics
                            font: ratingLabelMetrics.parent ? ratingLabelMetrics.parent.font : Kirigami.Theme.smallFont
                            text: ratingLabelMetrics.parent ? ratingLabelMetrics.parent.text : ""
                        }
                    }
                }

                // Size label
                QQC2.Label {
                    Layout.alignment: Qt.AlignBottom
                    visible: !root.compact && root.showSize
                    text: root.application.sizeDescription
                    font: Kirigami.Theme.smallFont
                }

                // Spacer to push button to the right
                Item {
                    Layout.fillWidth: true
                }

                // Install button
                InstallApplicationButton {
                    id: installButton
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    visible: !root.compact
                    application: root.application
                }
            }
        }
    }
}