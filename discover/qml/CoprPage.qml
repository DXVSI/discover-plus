/*
 *   SPDX-FileCopyrightText: 2025
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

ApplicationsListPage {
    id: page

    title: i18n("COPR Packages")

    // Show COPR packages from already enabled repos
    originFilter: "COPR"
    allBackends: true

    // Don't allow navigation to other categories within COPR page
    // This ensures proper transition when switching from COPR to other categories
    canNavigate: false

    header: ColumnLayout {
        width: parent.width
        spacing: 0

        // Modern header with gradient background
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: headerContent.implicitHeight + Kirigami.Units.largeSpacing * 2

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.6, 1, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.3, 0.5, 0.9, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.6, 1, 0.08) }
            }

            Row {
                id: headerContent
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                    leftMargin: Kirigami.Units.largeSpacing
                }
                spacing: 8  // Минимальный отступ между элементами

                // COPR Icon
                Rectangle {
                    width: Kirigami.Units.iconSizes.large
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.2, 0.6, 1, 0.15)
                    border.width: 2
                    border.color: Qt.rgba(0.2, 0.6, 1, 0.4)
                    anchors.verticalCenter: parent.verticalCenter

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "package"
                        width: parent.width * 0.6
                        height: width
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Kirigami.Heading {
                        text: i18n("Community Projects")
                        level: 3
                        font.bold: true
                    }

                    QQC2.Label {
                        text: i18n("Browse and install packages from Fedora COPR repositories")
                        opacity: 0.7
                        font: Kirigami.Theme.smallFont
                    }
                }
            }
        }

        // Warning message
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            type: Kirigami.MessageType.Warning
            text: i18n("COPR repositories are not officially supported by Fedora. Use at your own risk.")
            visible: true
            showCloseButton: false
        }
    }
}
