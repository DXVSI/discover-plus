/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018-2021 Nate Graham <nate@kde.org>
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

    required property int index
    required property Discover.AbstractResource application

    property bool compact: false
    property bool showRating: true
    property bool showSize: false

    readonly property bool appIsFromNonDefaultBackend: Discover.ResourcesModel.currentApplicationBackend !== application.backend && application.backend.hasApplications

    // Force dark theme for consistency
    Material.theme: Material.Dark
    Material.background: "#1C1B1F"
    Material.foreground: "#E6E1E5"

    function trigger() {
        ListView.currentIndex = index
        Local.Navigation.openApplication(application)
    }
    elevation: 1
    interactive: true

    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    RowLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        // App icon
        Rectangle {
            Layout.preferredWidth: root.compact ? 48 : 64
            Layout.preferredHeight: Layout.preferredWidth
            radius: 8
            color: "#2B2A2E"

            Kirigami.Icon {
                anchors.centerIn: parent
                source: root.application ? root.application.icon : ""
                animated: false
                width: parent.width - 8
                height: parent.height - 8
                // Prevent icon from becoming monochrome
                isMask: false
                // Disable theme color application
                Kirigami.Theme.inherit: false
                color: "transparent"
            }
        }

        // Container for everything but the app icon
        ColumnLayout {
            id: columnLayout
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            // Container for app name and backend name labels
            RowLayout {
                spacing: Kirigami.Units.largeSpacing

                // App name label
                QQC2.Label {
                    id: head
                    Layout.fillWidth: true
                    font.pixelSize: root.compact ? 14 : 16
                    font.weight: Font.DemiBold
                    text: root.application.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: Material.foreground
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
                        text: root.application.origin
                        font: Kirigami.Theme.smallFont
                    }
                }
            }

            // Description/"Comment" label
            QQC2.Label {
                id: description
                Layout.fillWidth: true
                text: root.application.comment || ""
                elide: Text.ElideRight
                maximumLineCount: 2
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
                font.pixelSize: 12
                opacity: 0.7
                color: Material.foreground
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

                    Local.Rating {
                        Layout.alignment: Qt.AlignVCenter
                        value: root.application.rating.sortableRating
                        starSize: root.compact ? description.font.pointSize : head.font.pointSize
                        precision: Local.Rating.Precision.HalfStar
                        padding: 0
                    }
                    QQC2.Label {
                        Layout.alignment: Qt.AlignVCenter
                        visible: root.application.backend.reviewsBackend?.isResourceSupported(root.application) ?? false
                        text: root.application.rating.ratingCount > 0 ? i18np("%1 rating", "%1 ratings", root.application.rating.ratingCount) : i18n("No ratings yet")
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        color: Material.foreground
                    }
                }

                // Size label
                QQC2.Label {
                    id: sizeInfo
                    Layout.alignment: Qt.AlignVCenter
                    visible: !root.compact && root.showSize
                    text: visible ? root.application.sizeDescription : ""
                    opacity: 0.6
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: Material.foreground
                }

                Item {
                    Layout.fillWidth: true
                }

                // Install button
                Local.InstallApplicationButton {
                    id: installButton
                    Layout.alignment: Qt.AlignVCenter
                    visible: true
                    application: root.application
                    installOrRemoveButtonDisplayStyle: root.compact ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextBesideIcon
                }
            }
        }
    }

    Accessible.name: application.name
    Accessible.onPressAction: trigger()
}
