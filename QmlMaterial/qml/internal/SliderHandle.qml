import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 20
    implicitHeight: 20

    property real value: 0
    property bool handleHasFocus: false
    property bool handlePressed: false
    property bool handleHovered: false
    readonly property int initialSize: 4
    readonly property var control: parent

    MD.ElevationRectangle {
        id: handleRect
        anchors.centerIn: parent
        width: 20
        height: 20
        radius: width / 2
        color: root.control ? root.control.mdState.backgroundColor : "transparent"
        elevation: MD.Token.elevation.level1
    }

    MD.Ripple2 {
        anchors.centerIn: parent
        width: 36
        height: 36
        radius: width / 2
        pressed: root.handlePressed || root.handleHasFocus
        color: root.control.mdState.backgroundColor
        stateOpacity: root.handleHovered ? MD.Token.state.hover.state_layer_opacity : "transparent"
    }
}
