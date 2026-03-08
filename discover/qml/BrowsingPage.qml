/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *   SPDX-FileCopyrightText: 2025 Discover Plus Contributors
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

// Use plain Page to avoid ScrollablePage padding
Kirigami.Page {
    id: page

    title: i18nc("@title:window the name of a top-level 'home' page", "Home")
    objectName: "featured"

    // Remove all padding
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    readonly property bool isHome: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    signal clearSearch()

    // State for animation phases
    property bool showAppsSection: false
    property bool initialLayoutDone: false

    // Timer to mark initial layout as done (prevents y animation on load)
    Timer {
        id: initialLayoutTimer
        interval: 100
        running: true
        onTriggered: page.initialLayoutDone = true
    }

    // Timer to trigger the transition (4 seconds)
    Timer {
        id: transitionTimer
        interval: 4000
        running: true
        onTriggered: page.showAppsSection = true
    }

    // Popular apps list (using icons available in hicolor/breeze)
    ListModel {
        id: popularAppsModel
        ListElement { name: "Firefox"; icon: "firefox" }
        ListElement { name: "LibreOffice Writer"; icon: "libreoffice-writer" }
        ListElement { name: "Kate"; icon: "kate" }
        ListElement { name: "Kdenlive"; icon: "kdenlive" }
        ListElement { name: "Telegram"; icon: "org.telegram.desktop" }
        ListElement { name: "Steam"; icon: "steam" }
        ListElement { name: "Gwenview"; icon: "gwenview" }
        ListElement { name: "Okular"; icon: "okular" }
        ListElement { name: "Elisa"; icon: "elisa" }
        ListElement { name: "Kolourpaint"; icon: "kolourpaint" }
    }

    // Full screen gradient background
    Rectangle {
        id: heroSection
        anchors.fill: parent

        // Animated gradient background - Fedora colors
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#3C6EB4" }
            GradientStop { position: 0.5; color: "#294172" }
            GradientStop { position: 1.0; color: "#79519E" }
        }

        // Subtle breathing animation
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: true
            NumberAnimation { to: 0.97; duration: 4000; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 4000; easing.type: Easing.InOutSine }
        }

        // ========== DECORATIVE FLOATING CIRCLES ==========
        Repeater {
            model: 7
            Rectangle {
                id: decorCircle
                required property int index
                property real baseY: heroSection.height * (0.2 + Math.random() * 0.6)
                property real baseX: heroSection.width * (0.05 + index * 0.14)
                x: baseX - width/2
                y: baseY
                width: 60 + index * 25
                height: width
                radius: width / 2
                color: "transparent"
                border.color: Qt.rgba(1, 1, 1, 0.08 + index * 0.01)
                border.width: 1.5

                // Floating animation
                SequentialAnimation on y {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation {
                        to: decorCircle.baseY - 25 - decorCircle.index * 5
                        duration: 3000 + decorCircle.index * 400
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: decorCircle.baseY + 25 + decorCircle.index * 5
                        duration: 3000 + decorCircle.index * 400
                        easing.type: Easing.InOutSine
                    }
                }

                // Horizontal drift
                SequentialAnimation on x {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation {
                        to: decorCircle.baseX - 10
                        duration: 4000 + decorCircle.index * 300
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: decorCircle.baseX + 10
                        duration: 4000 + decorCircle.index * 300
                        easing.type: Easing.InOutSine
                    }
                }

                // Fade in
                SequentialAnimation on opacity {
                    running: true
                    PauseAnimation { duration: decorCircle.index * 150 }
                    NumberAnimation { from: 0; to: 1; duration: 1000; easing.type: Easing.OutCubic }
                }
            }
        }

        // ========== SMALL FLOATING DOTS ==========
        Repeater {
            model: 12
            Rectangle {
                id: dot
                required property int index
                property real baseX: Math.random() * heroSection.width
                property real baseY: Math.random() * heroSection.height
                x: baseX
                y: baseY
                width: 4 + Math.random() * 6
                height: width
                radius: width / 2
                color: Qt.rgba(1, 1, 1, 0.15 + Math.random() * 0.1)

                // Floating
                SequentialAnimation on y {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation {
                        to: dot.baseY - 30 - Math.random() * 20
                        duration: 2500 + Math.random() * 2000
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: dot.baseY + 30 + Math.random() * 20
                        duration: 2500 + Math.random() * 2000
                        easing.type: Easing.InOutSine
                    }
                }

                // Twinkle effect
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: true
                    PauseAnimation { duration: dot.index * 200 }
                    NumberAnimation { to: 0.3; duration: 1500; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.6; duration: 1500; easing.type: Easing.InOutSine }
                }
            }
        }

        // ========== FLOATING APP ICONS ==========
        Repeater {
            model: ListModel {
                // Icons that exist in Breeze/hicolor themes
                ListElement { iconName: "firefox"; posX: 0.08; posY: 0.15; size: 48; rot: -12 }
                ListElement { iconName: "system-file-manager"; posX: 0.92; posY: 0.20; size: 40; rot: 15 }
                ListElement { iconName: "accessories-text-editor"; posX: 0.05; posY: 0.75; size: 36; rot: -8 }
                ListElement { iconName: "accessories-calculator"; posX: 0.88; posY: 0.80; size: 42; rot: 10 }
                ListElement { iconName: "utilities-terminal"; posX: 0.15; posY: 0.45; size: 38; rot: -15 }
                ListElement { iconName: "kate"; posX: 0.85; posY: 0.50; size: 44; rot: 8 }
                ListElement { iconName: "libreoffice-writer"; posX: 0.03; posY: 0.35; size: 40; rot: -5 }
                ListElement { iconName: "okular"; posX: 0.95; posY: 0.65; size: 36; rot: 12 }
                ListElement { iconName: "internet-mail"; posX: 0.12; posY: 0.88; size: 42; rot: -10 }
                ListElement { iconName: "gwenview"; posX: 0.90; posY: 0.12; size: 38; rot: 7 }
            }

            delegate: Kirigami.Icon {
                id: appIcon
                required property string iconName
                required property real posX
                required property real posY
                required property int size
                required property int rot
                required property int index

                source: iconName
                width: size
                height: size

                property real baseX: heroSection.width * posX
                property real baseY: heroSection.height * posY

                x: baseX - width/2
                y: baseY - height/2

                rotation: rot
                opacity: 0

                // Fade in with delay
                SequentialAnimation on opacity {
                    running: true
                    PauseAnimation { duration: 800 + appIcon.index * 150 }
                    NumberAnimation { from: 0; to: 0.15; duration: 1200; easing.type: Easing.OutCubic }
                }

                // Floating animation Y
                SequentialAnimation on y {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation {
                        to: appIcon.baseY - 15 - appIcon.index * 2
                        duration: 3500 + appIcon.index * 300
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: appIcon.baseY + 15 + appIcon.index * 2
                        duration: 3500 + appIcon.index * 300
                        easing.type: Easing.InOutSine
                    }
                }

                // Slight horizontal drift
                SequentialAnimation on x {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation {
                        to: appIcon.baseX - 8
                        duration: 4500 + appIcon.index * 200
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: appIcon.baseX + 8
                        duration: 4500 + appIcon.index * 200
                        easing.type: Easing.InOutSine
                    }
                }

                // Gentle rotation wobble
                SequentialAnimation on rotation {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation {
                        to: appIcon.rot - 3
                        duration: 4000 + appIcon.index * 250
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: appIcon.rot + 3
                        duration: 4000 + appIcon.index * 250
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }

        // ========== MAIN CONTENT ==========
        ColumnLayout {
            id: mainContent
            anchors.horizontalCenter: parent.horizontalCenter
            y: page.showAppsSection ? Kirigami.Units.largeSpacing * 3 : (parent.height - height) / 2
            spacing: Kirigami.Units.largeSpacing * 1.5

            Behavior on y {
                enabled: page.initialLayoutDone
                NumberAnimation {
                    duration: 800
                    easing.type: Easing.InOutQuad
                }
            }

            // Logos row
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Kirigami.Units.largeSpacing * 2

                // Fedora logo
                Kirigami.Icon {
                    id: fedoraLogo
                    source: "fedora-logo-icon"
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    opacity: 0

                    SequentialAnimation on opacity {
                        running: true
                        NumberAnimation { from: 0; to: 1; duration: 700; easing.type: Easing.OutCubic }
                    }

                    // Gentle pulse
                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        running: true
                        NumberAnimation { to: 1.06; duration: 2000; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.0; duration: 2000; easing.type: Easing.InOutSine }
                    }
                }

                // Plus sign with glow
                QQC2.Label {
                    text: "+"
                    font.pixelSize: 36
                    font.weight: Font.Light
                    color: "white"
                    opacity: 0

                    SequentialAnimation on opacity {
                        running: true
                        PauseAnimation { duration: 300 }
                        NumberAnimation { from: 0; to: 0.8; duration: 500; easing.type: Easing.OutCubic }
                    }
                }

                // Discover/KDE logo
                Kirigami.Icon {
                    id: kdeLogo
                    source: "plasmadiscover"
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    opacity: 0

                    SequentialAnimation on opacity {
                        running: true
                        PauseAnimation { duration: 250 }
                        NumberAnimation { from: 0; to: 1; duration: 700; easing.type: Easing.OutCubic }
                    }

                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        running: true
                        NumberAnimation { to: 1.06; duration: 2200; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.0; duration: 2200; easing.type: Easing.InOutSine }
                    }
                }
            }

            // Welcome text
            QQC2.Label {
                id: welcomeText
                Layout.alignment: Qt.AlignHCenter
                text: "Welcome to Discover"
                font.pixelSize: 48
                font.weight: Font.Bold
                color: "white"
                opacity: 0

                SequentialAnimation on opacity {
                    running: true
                    PauseAnimation { duration: 500 }
                    NumberAnimation { from: 0; to: 1; duration: 900; easing.type: Easing.OutCubic }
                }

                // Text shadow/glow
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: Qt.rgba(0, 0, 0, 0.4)
                    shadowBlur: 1.0
                    shadowVerticalOffset: 3
                }
            }

            // Subtitle
            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                text: i18n("Your gateway to thousands of applications")
                font.pixelSize: 18
                color: Qt.rgba(1, 1, 1, 0.9)
                opacity: 0

                SequentialAnimation on opacity {
                    running: true
                    PauseAnimation { duration: 700 }
                    NumberAnimation { from: 0; to: 1; duration: 800; easing.type: Easing.OutCubic }
                }
            }

            // Decorative line
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                width: 80
                height: 3
                radius: 1.5
                color: Qt.rgba(1, 1, 1, 0.4)
                opacity: 0

                SequentialAnimation on opacity {
                    running: true
                    PauseAnimation { duration: 900 }
                    NumberAnimation { from: 0; to: 1; duration: 600; easing.type: Easing.OutCubic }
                }

                // Width animation
                SequentialAnimation on width {
                    running: true
                    PauseAnimation { duration: 900 }
                    NumberAnimation { from: 0; to: 80; duration: 600; easing.type: Easing.OutCubic }
                }
            }

            // Version info
            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.smallSpacing
                text: "Discover Plus for Fedora"
                font.pixelSize: 13
                color: Qt.rgba(1, 1, 1, 0.6)
                opacity: 0

                SequentialAnimation on opacity {
                    running: true
                    PauseAnimation { duration: 1100 }
                    NumberAnimation { from: 0; to: 1; duration: 600; easing.type: Easing.OutCubic }
                }
            }

        }

        // ========== APPS SECTION ==========
        ColumnLayout {
            id: appsSection
            anchors {
                top: mainContent.bottom
                topMargin: Kirigami.Units.largeSpacing * 3
                horizontalCenter: parent.horizontalCenter
            }
            spacing: Kirigami.Units.largeSpacing * 2
            opacity: page.showAppsSection ? 1 : 0
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 600
                    easing.type: Easing.OutCubic
                }
            }

            // "What shall we install today?" text
            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                text: "What shall we install today?"
                font.pixelSize: 22
                font.weight: Font.DemiBold
                color: Qt.rgba(1, 1, 1, 0.9)

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: Qt.rgba(0, 0, 0, 0.3)
                    shadowBlur: 0.8
                    shadowVerticalOffset: 2
                }
            }

            // Apps grid
            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: 5
                rowSpacing: Kirigami.Units.largeSpacing * 2
                columnSpacing: Kirigami.Units.largeSpacing * 2

                Repeater {
                    model: popularAppsModel

                    delegate: Item {
                        id: appDelegate
                        required property int index
                        required property string name
                        required property string icon

                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 110
                        opacity: 0

                        // Staggered fade in animation
                        SequentialAnimation on opacity {
                            running: page.showAppsSection
                            PauseAnimation { duration: appDelegate.index * 100 }
                            NumberAnimation {
                                from: 0
                                to: 1
                                duration: 400
                                easing.type: Easing.OutCubic
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: Kirigami.Units.smallSpacing

                            // App icon with hover effect
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 72
                                Layout.preferredHeight: 72
                                radius: 16
                                color: appMouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.08)
                                border.color: Qt.rgba(1, 1, 1, 0.2)
                                border.width: 1

                                Behavior on color {
                                    ColorAnimation { duration: 150 }
                                }

                                Kirigami.Icon {
                                    id: appIconImage
                                    anchors.centerIn: parent
                                    width: 48
                                    height: 48
                                    source: appDelegate.icon
                                    fallback: "application-x-executable"
                                }

                                MouseArea {
                                    id: appMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        Navigation.openApplicationList({ search: appDelegate.name })
                                    }
                                }
                            }

                            // App name
                            QQC2.Label {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignHCenter
                                text: appDelegate.name
                                font.pixelSize: 11
                                color: Qt.rgba(1, 1, 1, 0.85)
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                maximumLineCount: 2
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }

        // ========== FOOTER ==========
        RowLayout {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                bottomMargin: Kirigami.Units.largeSpacing * 2
            }
            spacing: Kirigami.Units.smallSpacing
            opacity: 0

            SequentialAnimation on opacity {
                running: true
                PauseAnimation { duration: 1300 }
                NumberAnimation { from: 0; to: 0.6; duration: 600; easing.type: Easing.OutCubic }
            }

            QQC2.Label {
                text: "Powered by"
                font.pixelSize: 12
                color: "white"
            }

            Kirigami.Icon {
                source: "kde"
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                color: "white"
            }

            QQC2.Label {
                text: "KDE Plasma"
                font.pixelSize: 12
                color: "white"
            }
        }
    }
}
