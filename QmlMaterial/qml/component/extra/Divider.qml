import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

Rectangle {
    id: root
    property bool full: true
    property int orientation: {
        if (parent instanceof RowLayout)
            return Qt.Vertical;
        else
            return Qt.Horizontal;
    }
    property int size: 1
    property int leftMargin: 0
    property int rightMargin: 0

    states: [
        State {
            name: "layout-h"
            when: root.full && root.isLayout(root.parent) && root.orientation === Qt.Horizontal
            PropertyChanges {
                root.Layout.fillWidth: true
                root.implicitHeight: root.size
                root.height: root.size
            }
        },
        State {
            name: "layout-v"
            when: root.full && root.isLayout(root.parent) && root.orientation !== Qt.Horizontal
            PropertyChanges {
                root.Layout.fillHeight: true
                root.implicitWidth: root.size
                root.width: root.size
            }
        },
        State {
            name: "anchors-h"
            when: root.full && !root.isLayout(root.parent) && root.orientation === Qt.Horizontal
            PropertyChanges {
                root.anchors.left: root.parent.left
                root.anchors.right: root.parent.right
                root.anchors.leftMargin: root.leftMargin
                root.anchors.rightMargin: root.rightMargin
                root.implicitHeight: root.size
                root.height: root.size
            }
        },
        State {
            name: "anchors-v"
            when: root.full && !root.isLayout(root.parent) && root.orientation !== Qt.Horizontal
            PropertyChanges {
                root.anchors.top: root.parent.top
                root.anchors.bottom: root.parent.bottom
                root.anchors.topMargin: root.leftMargin
                root.anchors.bottomMargin: root.rightMargin
                root.implicitWidth: root.size
                root.width: root.size
            }
        }
    ]

    function isLayout(p) {
        return (p instanceof RowLayout) || (p instanceof GridLayout) || (p instanceof ColumnLayout);
    }
    color: MD.Token.color.outline_variant
}
