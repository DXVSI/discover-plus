/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2

// This whole wrapper around PageIndicator component exists because
// PageIndicator lacks some features such as basic horizontal centering
// and tapping on the left/right side of dots to change pages by one.
// See QTBUG-117864
MouseArea {
    id: root

    property alias interactive: indicator.interactive
    property alias currentIndex: indicator.currentIndex
    property alias count: indicator.count

    property alias topPadding: indicator.topPadding
    property alias bottomPadding: indicator.bottomPadding

    implicitHeight: indicator.implicitHeight
    implicitWidth: indicator.implicitWidth

    // Note: Instances should override this function instead of binding
    // directly, because assignments in pure JavaScript handler of MouseArea
    // will breaks any bindings, requiring re-setting them.
    function bindCurrentIndex() {
        // This is just an example of how a binding should look like. This
        // base implementation shouldn't actually be ever called.
        console.warn("Clients should override bindCurrentIndex()");
        currentIndex = Qt.binding(() => -1);
    }

    Component.onCompleted: bindCurrentIndex()
    // Let client code handle the change before we re-bind it back
    onCurrentIndexChanged: Qt.callLater(bindCurrentIndex)

    QQC2.PageIndicator {
        id: indicator

        focusPolicy: root.focusPolicy
        LayoutMirroring.enabled: root.LayoutMirroring.enabled

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    onPressed: event => {
        let direction = 0;
        if (event.x < indicator.x) {
            direction = LayoutMirroring.enabled ? 1 : -1;
        } else if (event.x > indicator.x + indicator.width) {
            direction = LayoutMirroring.enabled ? -1 : 1;
        } else {
            return;
        }
        // Careful: assignment breaks binding! Read the note above.
        if (direction < 0) {
            if (indicator.currentIndex > 0) {
                indicator.currentIndex -= 1;
            }
        } else if (direction > 0) {
            if (indicator.currentIndex < indicator.count - 1) {
                indicator.currentIndex += 1;
            }
        }
    }
}
