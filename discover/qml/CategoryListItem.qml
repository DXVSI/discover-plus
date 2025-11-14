/*
 *   SPDX-FileCopyrightText: 2025
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

QQC2.ItemDelegate {
    id: item

    property alias subtitle: subtitleLabel.text
    property bool isCategory: true

    Layout.fillWidth: true
    Layout.leftMargin: Kirigami.Units.smallSpacing
    Layout.rightMargin: Kirigami.Units.smallSpacing
    Layout.topMargin: 2
    Layout.bottomMargin: 2

    highlighted: checked
    visible: enabled

    // Modern category background
    background: Rectangle {
        id: bg
        radius: Kirigami.Units.mediumSpacing
        color: "transparent"

        // Gradient background for hover/selected states
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: item.hovered || item.checked

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: {
                        if (item.checked) {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r,
                                         Kirigami.Theme.highlightColor.g,
                                         Kirigami.Theme.highlightColor.b, 0.15)
                        } else if (item.hovered) {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r,
                                         Kirigami.Theme.highlightColor.g,
                                         Kirigami.Theme.highlightColor.b, 0.08)
                        }
                        return "transparent"
                    }
                }
                GradientStop {
                    position: 1.0
                    color: {
                        if (item.checked) {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r,
                                         Kirigami.Theme.highlightColor.g,
                                         Kirigami.Theme.highlightColor.b, 0.25)
                        } else if (item.hovered) {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r,
                                         Kirigami.Theme.highlightColor.g,
                                         Kirigami.Theme.highlightColor.b, 0.12)
                        }
                        return "transparent"
                    }
                }
            }

            Behavior on opacity {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
        }

        // Active indicator bar
        Rectangle {
            visible: item.checked
            width: 4
            height: parent.height - Kirigami.Units.smallSpacing
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            radius: width / 2

            gradient: Gradient {
                GradientStop { position: 0.0; color: Kirigami.Theme.highlightColor }
                GradientStop { position: 1.0; color: Qt.lighter(Kirigami.Theme.highlightColor, 1.2) }
            }

            // Glow effect for active indicator
            layer.enabled: true
            layer.effect: MultiEffect {
                blurEnabled: true
                blur: 0.3
                blurMax: 8
                saturation: 1.5
            }
        }

        // Border
        border.width: item.checked ? 1 : 0
        border.color: Qt.rgba(Kirigami.Theme.highlightColor.r,
                             Kirigami.Theme.highlightColor.g,
                             Kirigami.Theme.highlightColor.b, 0.3)

        Behavior on color {
            ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
        Behavior on border.width {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        // Icon container with colored background
        Item {
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Layout.preferredWidth
            Layout.alignment: Qt.AlignVCenter

            // Colored icon background
            Rectangle {
                anchors.fill: parent
                radius: Kirigami.Units.smallSpacing

                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: {
                            var baseColor = Kirigami.Theme.highlightColor
                            var cleanText = item.text ? item.text.replace(/&([^&])/g, "$1").toLowerCase() : ""
                            if (cleanText.includes("игр") || cleanText.includes("game")) {
                                baseColor = "#e74c3c"
                            } else if (cleanText.includes("разработ") || cleanText.includes("development")) {
                                baseColor = "#3498db"
                            } else if (cleanText.includes("графи") || cleanText.includes("graphics")) {
                                baseColor = "#9b59b6"
                            } else if (cleanText.includes("интернет") || cleanText.includes("internet") || cleanText.includes("network")) {
                                baseColor = "#1abc9c"
                            } else if (cleanText.includes("мультимед") || cleanText.includes("multimedia") || cleanText.includes("audio") || cleanText.includes("video")) {
                                baseColor = "#f39c12"
                            } else if (cleanText.includes("офис") || cleanText.includes("office") || cleanText.includes("productivity")) {
                                baseColor = "#2ecc71"
                            } else if (cleanText.includes("наук") || cleanText.includes("образован") || cleanText.includes("science") || cleanText.includes("education")) {
                                baseColor = "#16a085"
                            } else if (cleanText.includes("систем") || cleanText.includes("утилит") || cleanText.includes("system") || cleanText.includes("utilities")) {
                                baseColor = "#95a5a6"
                            } else if (cleanText.includes("copr")) {
                                baseColor = "#3498db"
                            }
                            return Qt.rgba(baseColor.r || 0, baseColor.g || 0, baseColor.b || 0, item.checked ? 0.2 : 0.1)
                        }
                    }
                    GradientStop {
                        position: 1.0
                        color: {
                            var baseColor = Kirigami.Theme.highlightColor
                            var cleanText = item.text ? item.text.replace(/&([^&])/g, "$1").toLowerCase() : ""
                            if (cleanText.includes("игр") || cleanText.includes("game")) {
                                baseColor = "#c0392b"
                            } else if (cleanText.includes("разработ") || cleanText.includes("development")) {
                                baseColor = "#2980b9"
                            } else if (cleanText.includes("графи") || cleanText.includes("graphics")) {
                                baseColor = "#8e44ad"
                            } else if (cleanText.includes("интернет") || cleanText.includes("internet") || cleanText.includes("network")) {
                                baseColor = "#16a085"
                            } else if (cleanText.includes("мультимед") || cleanText.includes("multimedia") || cleanText.includes("audio") || cleanText.includes("video")) {
                                baseColor = "#e67e22"
                            } else if (cleanText.includes("офис") || cleanText.includes("office") || cleanText.includes("productivity")) {
                                baseColor = "#27ae60"
                            } else if (cleanText.includes("наук") || cleanText.includes("образован") || cleanText.includes("science") || cleanText.includes("education")) {
                                baseColor = "#138871"
                            } else if (cleanText.includes("систем") || cleanText.includes("утилит") || cleanText.includes("system") || cleanText.includes("utilities")) {
                                baseColor = "#7f8c8d"
                            } else if (cleanText.includes("copr")) {
                                baseColor = "#2980b9"
                            }
                            return Qt.rgba(baseColor.r || 0, baseColor.g || 0, baseColor.b || 0, item.checked ? 0.25 : 0.15)
                        }
                    }
                }

                Behavior on opacity {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
            }

            // Icon
            Kirigami.Icon {
                anchors.centerIn: parent
                source: item.icon.name || item.icon.source || ""
                width: Kirigami.Units.iconSizes.smallMedium
                height: width
                color: {
                    if (item.checked) {
                        var cleanText = item.text ? item.text.replace(/&([^&])/g, "$1").toLowerCase() : ""
                        if (cleanText.includes("игр") || cleanText.includes("game")) {
                            return "#e74c3c"
                        } else if (cleanText.includes("разработ") || cleanText.includes("development")) {
                            return "#3498db"
                        } else if (cleanText.includes("графи") || cleanText.includes("graphics")) {
                            return "#9b59b6"
                        } else if (cleanText.includes("интернет") || cleanText.includes("internet") || cleanText.includes("network")) {
                            return "#1abc9c"
                        } else if (cleanText.includes("мультимед") || cleanText.includes("multimedia") || cleanText.includes("audio") || cleanText.includes("video")) {
                            return "#f39c12"
                        } else if (cleanText.includes("офис") || cleanText.includes("office") || cleanText.includes("productivity")) {
                            return "#2ecc71"
                        } else if (cleanText.includes("наук") || cleanText.includes("образован") || cleanText.includes("science") || cleanText.includes("education")) {
                            return "#16a085"
                        } else if (cleanText.includes("систем") || cleanText.includes("утилит") || cleanText.includes("system") || cleanText.includes("utilities")) {
                            return "#95a5a6"
                        } else if (cleanText.includes("copr")) {
                            return "#3498db"
                        }
                        return Kirigami.Theme.highlightColor
                    }
                    return Kirigami.Theme.textColor
                }

                // Scale animation on hover
                scale: item.hovered ? 1.1 : 1.0
                Behavior on scale {
                    NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                }
            }
        }

        // Text content
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            QQC2.Label {
                Layout.fillWidth: true
                text: {
                    // Remove ampersands that are used for mnemonics
                    var cleanText = item.text || ""
                    return cleanText.replace(/&([^&])/g, "$1")
                }
                font.weight: item.checked ? Font.DemiBold : Font.Normal
                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.05
                elide: Text.ElideRight
                color: item.checked ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }

            QQC2.Label {
                id: subtitleLabel
                Layout.fillWidth: true
                visible: text.length > 0
                font: Kirigami.Theme.smallFont
                opacity: 0.6
                elide: Text.ElideRight
            }
        }

        // Chevron for expandable items
        Kirigami.Icon {
            visible: item.action && item.action.children && item.action.children.length > 0
            source: "arrow-right"
            width: Kirigami.Units.iconSizes.small
            height: width
            opacity: 0.4

            rotation: item.action && item.action.expanded ? 90 : 0
            Behavior on rotation {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
        }
    }

    // Hover animation
    HoverHandler {
        id: hoverHandler
    }
}