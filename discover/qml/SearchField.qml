/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2019 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Kirigami.SearchField {
    id: root

    // for appium tests
    objectName: "searchField"

    // Search operations are network-intensive, so we can't have search-as-you-type.
    // This means we should turn off auto-accept entirely, rather than having it on
    // with a delay. The result just isn't good. See Bug 445142.
    autoAccept: false

    property QtObject page
    property string currentSearchText

    placeholderText: (!enabled || !page || page.hasOwnProperty("isHome") || window.leftPage.name.length === 0) ? i18n("Search…") : i18n("Search in '%1'…", window.leftPage.name)

    // Modern search field styling
    background: Rectangle {
        radius: height / 2
        color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.05)
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Kirigami.Theme.highlightColor : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)

        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    onAccepted: {
        text = text.trim();
        currentSearchText = text;
    }

    function clearText() {
        text = "";
        accepted();
    }

    Connections {
        ignoreUnknownSignals: true
        target: root.page

        function onClearSearch() {
            root.clearText();
        }
    }

    Connections {
        target: applicationWindow()
        function onCurrentTopLevelChanged() {
            if (applicationWindow().currentTopLevel.length > 0) {
                root.clearText();
            }
        }
    }
}
