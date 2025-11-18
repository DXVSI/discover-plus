/*
 *   SPDX-FileCopyrightText: 2025
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami
import "." as Local

Local.ApplicationsListPage {
    id: page

    title: search.length > 0 ? i18n("COPR Search: %1", search) : i18n("COPR Packages")

    // Show COPR packages from already enabled repos
    originFilter: "COPR"
    allBackends: true

    // Hide ratings for COPR packages
    showRating: false
    showSize: true

    // Don't allow navigation to other categories within COPR page
    // This ensures proper transition when switching from COPR to other categories
    canNavigate: false

    // Custom header with search field and info message
    listHeader: ColumnLayout {
        width: page.width
        spacing: Kirigami.Units.largeSpacing

        // COPR Info Message
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            type: Kirigami.MessageType.Information
            text: i18n("COPR - Community projects. Note: Packages from COPR repositories are not officially supported by Fedora.")
            visible: true
            showCloseButton: false
        }

        // Search field for COPR
        Rectangle {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            Layout.topMargin: 0
            height: 48
            radius: 24
            color: "#2B2A2E"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8

                Kirigami.Icon {
                    source: "search"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    color: "#CAC4D0"
                }

                QQC2.TextField {
                    id: coprSearchField
                    Layout.fillWidth: true
                    placeholderText: i18n("Search in COPR packages...")
                    font.pixelSize: 14
                    selectByMouse: true

                    background: Rectangle {
                        color: "transparent"
                    }

                    onAccepted: {
                        page.search = text.trim()
                    }

                    Component.onCompleted: {
                        // Set initial text if there's a search
                        if (page.search.length > 0) {
                            text = page.search
                        }
                    }

                    Connections {
                        target: page
                        function onSearchChanged() {
                            if (coprSearchField.text !== page.search) {
                                coprSearchField.text = page.search
                            }
                        }
                    }
                }

                // Clear button
                QQC2.ToolButton {
                    visible: coprSearchField.text.length > 0
                    icon.name: "edit-clear"
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    onClicked: {
                        coprSearchField.text = ""
                        page.search = ""
                        coprSearchField.forceActiveFocus()
                    }
                }
            }
        }
    }
}
