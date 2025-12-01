import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD
import QtQuick.Window

T.Drawer {
    id: control

    parent: T.Overlay.overlay

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    topPadding: edge !== Qt.TopEdge ? 16 : 0
    bottomPadding: edge !== Qt.BottomEdge ? 16 : 0

    width: implicitWidth
    height: parent.height

    enter: Transition {
        SmoothedAnimation {
            velocity: 5
        }
    }
    exit: Transition {
        SmoothedAnimation {
            velocity: 5
        }
    }

    property int elevation: !interactive && !dim ? MD.Token.elevation.level0 : MD.Token.elevation.level1

    background: Item {
        implicitWidth: Math.min(360, control.Window.window?.width ?? 360)
        Rectangle {
            anchors.fill: parent
            color: MD.Token.color.surface
            layer.enabled: true
            layer.effect: MD.RoundClip {
                corners: MD.Util.corners(0, 16, 0, 16)
                size: Qt.vector2d(parent.width, parent.height)
            }
        }
        //    elevation: control.elevation
    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }
}
