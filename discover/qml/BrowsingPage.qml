/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.Material
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import "." as Local

Local.DiscoverPage {
    id: page

    title: i18nc("@title:window the name of a top-level 'home' page", "Discover")
    objectName: "featured"

    actions: window.wideScreen ? [ searchAction ] : []

    header: Local.DiscoverInlineMessage {
        id: message
        inlineMessage: Discover.ResourcesModel.inlineMessage ? Discover.ResourcesModel.inlineMessage : app.homePageMessage
    }

    readonly property bool isHome: true

    // Material Design 3 colors
    Material.theme: Material.Dark
    Material.background: "#1C1B1F"
    Material.foreground: "#E6E1E5"

    DiscoverApp.FeaturedModel {
        id: featuredModel
    }

    Kirigami.LoadingPlaceholder {
        visible: featuredModel.isFetching
        anchors.centerIn: parent
    }

    QQC2.ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 24

            // Hero Section with Featured Apps
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                color: "#2B2A2E"
                radius: 16
                Layout.margins: 16

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    QQC2.Label {
                        text: i18nc("@title:group", "Featured Applications")
                        font.pixelSize: 28
                        font.weight: Font.Bold
                        color: "#E6E1E5"
                    }

                    QQC2.ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOff

                        Row {
                            spacing: 16

                            Repeater {
                                model: DiscoverApp.LimitedRowCountProxyModel {
                                    pageSize: 5
                                    sourceModel: featuredModel
                                }

                                delegate: Rectangle {
                                    required property var model
                                    width: 280
                                    height: 200
                                    radius: 12
                                    color: "#36343B"

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 16
                                        spacing: 12

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 12

                                            Rectangle {
                                                width: 56
                                                height: 56
                                                radius: 12
                                                color: "#2B2A2E"

                                                Kirigami.Icon {
                                                    anchors.centerIn: parent
                                                    width: 40
                                                    height: 40
                                                    source: model.application.icon
                                                    isMask: false
                                                    Kirigami.Theme.inherit: false
                                                    color: "transparent"
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4

                                                QQC2.Label {
                                                    Layout.fillWidth: true
                                                    text: model.application.name
                                                    font.pixelSize: 16
                                                    font.weight: Font.DemiBold
                                                    color: "#E6E1E5"
                                                    elide: Text.ElideRight
                                                }

                                                QQC2.Label {
                                                    Layout.fillWidth: true
                                                    text: model.application.comment || ""
                                                    font.pixelSize: 12
                                                    color: "#CAC4D0"
                                                    maximumLineCount: 2
                                                    wrapMode: Text.WordWrap
                                                    elide: Text.ElideRight
                                                }
                                            }
                                        }

                                        Item { Layout.fillHeight: true }

                                        Local.InstallApplicationButton {
                                            Layout.fillWidth: true
                                            application: model.application
                                            visible: model.application
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: Local.Navigation.openApplication(model.application)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Categories Section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 16
                spacing: 16

                QQC2.Label {
                    text: i18nc("@title:group", "Categories")
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                    color: "#E6E1E5"
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 16

                    // All Apps Category
                    Rectangle {
                        width: 280
                        height: 120
                        radius: 12
                        color: "#2B2A2E"

                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#4F378B" }
                            GradientStop { position: 1.0; color: "#2B2A2E" }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Kirigami.Icon {
                                source: "applications-all"
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                color: "#EADDFF"
                            }

                            QQC2.Label {
                                text: i18n("All Apps")
                                font.pixelSize: 18
                                font.weight: Font.Medium
                                color: "#EADDFF"
                            }

                            QQC2.Label {
                                text: i18n("Browse all applications")
                                font.pixelSize: 12
                                color: "#CAC4D0"
                                opacity: 0.8
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Local.Navigation.openApplicationList({
                                category: "All Applications"
                            })
                        }
                    }

                    // Games Category
                    Rectangle {
                        width: 280
                        height: 120
                        radius: 12

                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#633B48" }
                            GradientStop { position: 1.0; color: "#2B2A2E" }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Kirigami.Icon {
                                source: "applications-games-symbolic"
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                color: "#FFD8E4"
                            }

                            QQC2.Label {
                                text: i18n("Games")
                                font.pixelSize: 18
                                font.weight: Font.Medium
                                color: "#FFD8E4"
                            }

                            QQC2.Label {
                                text: i18n("Entertainment & fun")
                                font.pixelSize: 12
                                color: "#CAC4D0"
                                opacity: 0.8
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Local.Navigation.openApplicationList({
                                category: "Games"
                            })
                        }
                    }

                    // Development Category
                    Rectangle {
                        width: 280
                        height: 120
                        radius: 12

                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#4A4458" }
                            GradientStop { position: 1.0; color: "#2B2A2E" }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Kirigami.Icon {
                                source: "applications-development"
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                color: "#E8DEF8"
                            }

                            QQC2.Label {
                                text: i18n("Development")
                                font.pixelSize: 18
                                font.weight: Font.Medium
                                color: "#E8DEF8"
                            }

                            QQC2.Label {
                                text: i18n("IDEs & tools")
                                font.pixelSize: 12
                                color: "#CAC4D0"
                                opacity: 0.8
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Local.Navigation.openApplicationList({
                                category: "Development"
                            })
                        }
                    }

                    // Multimedia Category
                    Rectangle {
                        width: 280
                        height: 120
                        radius: 12

                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#018786" }
                            GradientStop { position: 1.0; color: "#2B2A2E" }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Kirigami.Icon {
                                source: "applications-multimedia"
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                color: "#C8FFF4"
                            }

                            QQC2.Label {
                                text: i18n("Multimedia")
                                font.pixelSize: 18
                                font.weight: Font.Medium
                                color: "#C8FFF4"
                            }

                            QQC2.Label {
                                text: i18n("Audio & video")
                                font.pixelSize: 12
                                color: "#CAC4D0"
                                opacity: 0.8
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Local.Navigation.openApplicationList({
                                category: "Multimedia"
                            })
                        }
                    }
                }
            }

            // Recently Updated Section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 16
                spacing: 16
                visible: recentlyUpdatedRepeater.count > 0

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        text: i18nc("@title:group", "Recently Updated")
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                        color: "#E6E1E5"
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.Button {
                        text: i18n("See all")
                        Material.background: "transparent"
                        Material.foreground: "#D0BCFF"
                        onClicked: Local.Navigation.openApplicationList({
                            sortRole: Discover.ResourcesProxyModel.ReleaseDateRole,
                            sortOrder: Qt.DescendingOrder,
                            category: "All Applications"
                        })
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 16

                    Repeater {
                        id: recentlyUpdatedRepeater
                        model: DiscoverApp.LimitedRowCountProxyModel {
                            pageSize: 6
                            sourceModel: Discover.ResourcesProxyModel {
                                filteredCategoryName: "All Applications"
                                backendFilter: Discover.ResourcesModel.currentApplicationBackend
                                sortRole: Discover.ResourcesProxyModel.ReleaseDateRole
                                sortOrder: Qt.DescendingOrder
                            }
                        }

                        delegate: Local.MaterialCard {
                            required property var model
                            width: 200
                            height: 220
                            elevation: 1

                            onClicked: Local.Navigation.openApplication(model.application)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 8

                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    width: 64
                                    height: 64
                                    radius: 12
                                    color: "#36343B"

                                    Kirigami.Icon {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        source: model.application.icon
                                        isMask: false
                                        Kirigami.Theme.inherit: false
                                        color: "transparent"
                                    }
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: model.application.name
                                    font.pixelSize: 14
                                    font.weight: Font.Medium
                                    color: "#E6E1E5"
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: model.application.comment || ""
                                    font.pixelSize: 11
                                    color: "#CAC4D0"
                                    horizontalAlignment: Text.AlignHCenter
                                    maximumLineCount: 2
                                    wrapMode: Text.WordWrap
                                    elide: Text.ElideRight
                                    opacity: 0.7
                                }

                                Item { Layout.fillHeight: true }

                                Local.InstallApplicationButton {
                                    Layout.fillWidth: true
                                    application: model.application
                                    visible: model.application
                                    installOrRemoveButtonDisplayStyle: QQC2.AbstractButton.IconOnly
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    signal clearSearch()
}