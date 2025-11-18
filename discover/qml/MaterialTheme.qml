/*
 *   SPDX-FileCopyrightText: 2024 KDE Developers
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma Singleton

import QtQuick
import QtQuick.Controls.Material

QtObject {
    id: theme

    // Material Design 3 Dark Theme Colors
    readonly property color primary: "#D0BCFF"
    readonly property color onPrimary: "#381E72"
    readonly property color primaryContainer: "#4F378B"
    readonly property color onPrimaryContainer: "#EADDFF"

    readonly property color secondary: "#CCC2DC"
    readonly property color onSecondary: "#332D41"
    readonly property color secondaryContainer: "#4A4458"
    readonly property color onSecondaryContainer: "#E8DEF8"

    readonly property color tertiary: "#EFB8C8"
    readonly property color onTertiary: "#492532"
    readonly property color tertiaryContainer: "#633B48"
    readonly property color onTertiaryContainer: "#FFD8E4"

    readonly property color error: "#F2B8B5"
    readonly property color onError: "#601410"
    readonly property color errorContainer: "#8C1D18"
    readonly property color onErrorContainer: "#F9DEDC"

    readonly property color surface: "#1C1B1F"
    readonly property color onSurface: "#E6E1E5"
    readonly property color surfaceVariant: "#49454F"
    readonly property color onSurfaceVariant: "#CAC4D0"

    readonly property color outline: "#938F99"
    readonly property color outlineVariant: "#49454F"

    readonly property color background: "#1C1B1F"
    readonly property color onBackground: "#E6E1E5"

    readonly property color surfaceContainer: "#201F23"
    readonly property color surfaceContainerLow: "#1C1B1F"
    readonly property color surfaceContainerHigh: "#2B2A2E"
    readonly property color surfaceContainerHighest: "#36343B"

    // Elevation colors for dark theme
    readonly property var elevationColors: [
        "transparent",           // 0dp
        Qt.rgba(208/255, 188/255, 255/255, 0.05), // 1dp
        Qt.rgba(208/255, 188/255, 255/255, 0.08), // 3dp
        Qt.rgba(208/255, 188/255, 255/255, 0.11), // 6dp
        Qt.rgba(208/255, 188/255, 255/255, 0.12), // 8dp
        Qt.rgba(208/255, 188/255, 255/255, 0.14)  // 12dp
    ]

    // Shape radiuses
    readonly property int radiusSmall: 8
    readonly property int radiusMedium: 12
    readonly property int radiusLarge: 16
    readonly property int radiusExtraLarge: 28

    // Spacing
    readonly property int spacingSmall: 4
    readonly property int spacingMedium: 8
    readonly property int spacingLarge: 16
    readonly property int spacingExtraLarge: 24

    function applyTheme(target) {
        if (target) {
            target.Material.theme = Material.Dark
            target.Material.primary = primary
            target.Material.accent = primary
            target.Material.background = background
            target.Material.foreground = onBackground
        }
    }
}