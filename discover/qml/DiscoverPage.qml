/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false

    property bool compact: root.width < Kirigami.Units.gridUnit * 28 || !applicationWindow().wideScreen

    Shortcut {
        sequences: [ StandardKey.MoveToNextPage ]
        enabled: root.isCurrentPage
        onActivated: {
            root.flickable.contentY = Math.min(root.flickable.contentHeight - root.flickable.height,
                                               root.flickable.contentY + root.flickable.height);
        }
    }

    Shortcut {
        sequences: [ StandardKey.MoveToPreviousPage ]
        enabled: root.isCurrentPage
        onActivated: {
            root.flickable.contentY = Math.max(0, root.flickable.contentY - root.flickable.height);
        }
    }

    Shortcut {
        sequences: [ StandardKey.Cancel ]
        enabled: root.isCurrentPage && applicationWindow().pageStack.depth > 1
        onActivated: {
            applicationWindow().pageStack.pop()
        }
    }

    Keys.onPressed: event => {
        const readableCharacters = /\w+/;
        if (event.text.length > 0 && event.modifiers === Qt.NoModifier && event.text.match(readableCharacters)) {
            window.globalDrawer.suggestSearchText(event.text)
        }
    }
}
