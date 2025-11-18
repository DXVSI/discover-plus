/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.Material
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami
import "." as Local

Kirigami.GlobalDrawer {
    id: drawer

    // Material Design 3 Dark Theme
    Material.theme: Material.Dark
    Material.background: "#1C1B1F"
    Material.foreground: "#E6E1E5"

    property bool wideScreen: false

    function suggestSearchText(text) {
        // Function kept for compatibility
    }

    function forceSearchFieldFocus() {
        // Function kept for compatibility
    }

    interactiveResizeEnabled: true
    onPreferredSizeChanged: app.sidebarWidth = preferredSize

    padding: 0
    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    resetMenuOnTriggered: false
    modal: !drawer.wideScreen

    onCurrentSubMenuChanged: {
        if (currentSubMenu) {
            currentSubMenu.trigger()
        }
    }


    // Main content - simplified navigation only
    contentItem: ColumnLayout {
        spacing: 0

        // Main Navigation
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            spacing: 4

            // Home
            Rectangle {
                Layout.fillWidth: true
                height: 56
                color: window.currentTopLevel === window.topBrowsingComp ? "#4F378B" : (homeMA.containsMouse ? "#36343B" : "transparent")
                radius: 28

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Kirigami.Icon {
                        source: "go-home"
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        color: window.currentTopLevel === window.topBrowsingComp ? "#EADDFF" : "#CAC4D0"
                    }

                    QQC2.Label {
                        text: i18n("Home")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        font.weight: window.currentTopLevel === window.topBrowsingComp ? Font.DemiBold : Font.Normal
                        color: window.currentTopLevel === window.topBrowsingComp ? "#EADDFF" : "#E6E1E5"
                    }
                }

                MouseArea {
                    id: homeMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        window.currentTopLevel = window.topBrowsingComp
                    }
                }
            }

            // Installed
            Rectangle {
                Layout.fillWidth: true
                height: 56
                color: window.currentTopLevel === window.topInstalledComp ? "#4F378B" : (installedMA.containsMouse ? "#36343B" : "transparent")
                radius: 28

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Kirigami.Icon {
                        source: "view-list-details"
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        color: window.currentTopLevel === window.topInstalledComp ? "#EADDFF" : "#CAC4D0"
                    }

                    QQC2.Label {
                        text: i18n("Installed")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        font.weight: window.currentTopLevel === window.topInstalledComp ? Font.DemiBold : Font.Normal
                        color: window.currentTopLevel === window.topInstalledComp ? "#EADDFF" : "#E6E1E5"
                    }
                }

                MouseArea {
                    id: installedMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        window.currentTopLevel = window.topInstalledComp
                    }
                }
            }

            // COPR
            Rectangle {
                Layout.fillWidth: true
                height: 56
                color: window.currentTopLevel === window.topCoprComp ? "#4F378B" : (coprMA.containsMouse ? "#36343B" : "transparent")
                radius: 28

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Kirigami.Icon {
                        source: "package"
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        color: window.currentTopLevel === window.topCoprComp ? "#EADDFF" : "#CAC4D0"
                    }

                    QQC2.Label {
                        text: i18n("COPR")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        font.weight: window.currentTopLevel === window.topCoprComp ? Font.DemiBold : Font.Normal
                        color: window.currentTopLevel === window.topCoprComp ? "#EADDFF" : "#E6E1E5"
                    }
                }

                MouseArea {
                    id: coprMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        window.currentTopLevel = window.topCoprComp
                    }
                }
            }

            // Updates
            Rectangle {
                Layout.fillWidth: true
                height: 56
                color: window.currentTopLevel === window.topUpdateComp ? "#4F378B" : (updatesMA.containsMouse ? "#36343B" : "transparent")
                radius: 28

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Kirigami.Icon {
                        source: Discover.ResourcesModel.updatesCount <= 0 ? "update-none" : "update-low"
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        color: window.currentTopLevel === window.topUpdateComp ? "#EADDFF" : "#CAC4D0"
                    }

                    QQC2.Label {
                        text: Discover.ResourcesModel.updatesCount > 0 ?
                              i18n("Updates (%1)", Discover.ResourcesModel.updatesCount) :
                              i18n("Updates")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        font.weight: window.currentTopLevel === window.topUpdateComp ? Font.DemiBold : Font.Normal
                        color: window.currentTopLevel === window.topUpdateComp ? "#EADDFF" : "#E6E1E5"
                    }

                    // Update badge
                    Rectangle {
                        visible: Discover.ResourcesModel.updatesCount > 0
                        width: 24
                        height: 24
                        radius: 12
                        color: "#CF6679"

                        QQC2.Label {
                            anchors.centerIn: parent
                            text: Discover.ResourcesModel.updatesCount
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: "#FFFFFF"
                        }
                    }
                }

                MouseArea {
                    id: updatesMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        window.currentTopLevel = window.topUpdateComp
                    }
                }
            }

            // Search field with history
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 8
                height: 48
                color: "#2B2A2E"
                radius: 24

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Kirigami.Icon {
                        source: "search"
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        color: "#CAC4D0"
                    }

                    Local.SearchField {
                        id: searchField
                        Layout.fillWidth: true

                        placeholderText: i18n("Search...")
                        font.pixelSize: 13

                        // Custom background to fit the design
                        background: Rectangle {
                            color: "transparent"
                        }

                        onAccepted: {
                            if (text.length > 0) {
                                // Check if we're on COPR page and search there
                                if (window.currentTopLevel === window.topCoprComp) {
                                    Local.Navigation.clearStack()
                                    Local.Navigation.openApplicationList({
                                        search: text,
                                        originFilter: "COPR",
                                        allBackends: true,
                                        title: i18n("COPR Search: %1", text)
                                    })
                                } else if (window.currentTopLevel === window.topInstalledComp) {
                                    // Search in installed apps
                                    Local.Navigation.clearStack()
                                    Local.Navigation.openApplicationList({
                                        search: text,
                                        stateFilter: Discover.AbstractResource.Installed,
                                        title: i18n("Installed: %1", text)
                                    })
                                } else {
                                    // Default search in all
                                    Local.Navigation.clearStack()
                                    Local.Navigation.openApplicationList({ search: text })
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Divider
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#49454F"
            }

            // Settings
            Rectangle {
                Layout.fillWidth: true
                height: 48
                Layout.topMargin: 8
                color: window.currentTopLevel === window.topSourcesComp ? "#4F378B" : (settingsMA.containsMouse ? "#36343B" : "transparent")
                radius: 24

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Kirigami.Icon {
                        source: "configure"
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        color: window.currentTopLevel === window.topSourcesComp ? "#EADDFF" : "#CAC4D0"
                    }

                    QQC2.Label {
                        text: i18n("Settings")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        color: window.currentTopLevel === window.topSourcesComp ? "#EADDFF" : "#E6E1E5"
                    }
                }

                MouseArea {
                    id: settingsMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        window.currentTopLevel = window.topSourcesComp
                    }
                }
            }

            // About
            Rectangle {
                Layout.fillWidth: true
                height: 48
                Layout.bottomMargin: 8
                color: window.currentTopLevel === window.topAboutComp ? "#4F378B" : (aboutMA.containsMouse ? "#36343B" : "transparent")
                radius: 24

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Kirigami.Icon {
                        source: "help-about"
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        color: window.currentTopLevel === window.topAboutComp ? "#EADDFF" : "#CAC4D0"
                    }

                    QQC2.Label {
                        text: i18n("About")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        color: window.currentTopLevel === window.topAboutComp ? "#EADDFF" : "#E6E1E5"
                    }
                }

                MouseArea {
                    id: aboutMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        window.currentTopLevel = window.topAboutComp
                    }
                }
            }
        }
    }

    footer: ColumnLayout {
        readonly property int transactions: Discover.TransactionModel.count
        readonly property bool currentPageShowsTransactionProgressInline:
               applicationWindow().pageStack.currentItem instanceof Local.ApplicationPage
            || applicationWindow().pageStack.currentItem instanceof Local.ApplicationsListPage
            || applicationWindow().pageStack.currentItem instanceof Local.UpdatesPage

        spacing: 0
        visible: transactions > 1 || (transactions === 1 && !currentPageShowsTransactionProgressInline)

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#49454F"
        }

        Local.ProgressView {
            Layout.fillWidth: true
        }
    }

    // Handle wide screen vs compact mode
    Component.onCompleted: {
        if (app.sidebarWidth > 0) {
            preferredSize = app.sidebarWidth
        } else {
            preferredSize = Kirigami.Units.gridUnit * 16
        }

        if (drawer.wideScreen) {
            drawer.drawerOpen = true
        } else {
            drawer.drawerOpen = false
        }
    }

    onWideScreenChanged: {
        if (drawer.wideScreen) {
            drawer.drawerOpen = true
        } else {
            drawer.drawerOpen = false
        }
    }
}