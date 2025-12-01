import QtQuick

import Qcm.Material as MD

Item {
    id: root
    property color color
    property int size: 24
    property bool checked: false

    implicitHeight: size
    implicitWidth: size

    signal clicked

    Rectangle {
        anchors.centerIn: parent
        color: root.color
        height: width
        radius: width / 2
        width: parent.implicitWidth

        Rectangle {
            anchors.centerIn: parent
            border.color: root.MD.MProp.color.surface
            border.width: 0.25 * 0.5 * root.size
            color: parent.color
            height: width
            radius: width / 2
            visible: root.checked
            width: root.size - 2 * border.width
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true

            onClicked: root.clicked()
        }
    }
}
