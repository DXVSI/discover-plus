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

    content: Item {
        implicitHeight: Math.max(columnLayout.implicitHeight, resourceIconFrame.implicitHeight)

        Kirigami.Padding {
            id: resourceIconFrame
            anchors {
                top: parent.top
                left: parent.left
                bottom: parent.bottom
            }
            padding: Kirigami.Units.largeSpacing
            contentItem: Kirigami.Icon {
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
            spacing: 0

            // Container for app name and backend name labels
            RowLayout {
                spacing: Kirigami.Units.largeSpacing

                // App name label
                Kirigami.Heading {
                    id: head
                    Layout.fillWidth: true
                    // We want the heading visually top-aligned with the top margin, the icon background and that in general
                    // it's root.padding away from the border. We can't just align the label to the top for it because internally
                    // everything is aligned respecting the text boundingRect, which includes blank space on the top as a "line" height, called "leading". Instead we need to base ourselves on tightBoundingRect which is a rect only around the
                    // painted area of the label, not including the leading
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

                // Backend name label (always shown except in compact view)
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    visible: !root.compact
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: root.application.sourceIcon
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }
                    QQC2.Label {
                        text: root.application.displayOrigin
                        font: Kirigami.Theme.smallFont
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
                    opacity: 0.6
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
                        elide: Text.ElideRight
                        TextMetrics {
                            id: ratingLabelMetrics
                            font: head.font
                            text: head.text
                        }
                    }
                }

                // Size label
                QQC2.Label {
                    id: sizeInfo
                    Layout.alignment: Qt.AlignBottom
                    visible: !root.compact && root.showSize
                    text: visible ? root.application.sizeDescription : ""
                    horizontalAlignment: Text.AlignRight
                    opacity: 0.6;
                    font: Kirigami.Theme.smallFont
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Item {
                    Layout.fillWidth: true
                }

                // Install button
                InstallApplicationButton {
                    id: installButton
                    Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                    visible: !root.compact
                    application: root.application
                    installOrRemoveButtonDisplayStyle: QQC2.AbstractButton.IconOnly
                }
            }
        }
    }

    Accessible.name: head.text
    Accessible.onPressAction: trigger()
}
