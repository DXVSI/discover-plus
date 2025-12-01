import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Switch {
    id: control

    property alias mdState: m_sh

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 8
    topInset: 0
    bottomInset: 0
    rightInset: 0
    leftInset: 0

    icon.width: 16
    icon.height: 16
    icon.color: control.mdState.textColor

    indicator: Rectangle {
        id: indicator
        width: 52
        height: 32
        radius: height / 2
        y: parent.height / 2 - height / 2
        color: MD.Util.transparent(control.mdState.backgroundColor, control.mdState.backgroundOpacity)
        border.width: 2
        border.color: MD.Util.transparent(control.mdState.ctx.color.outline, control.mdState.backgroundOpacity)

        Behavior on color {
            ColorAnimation {
                duration: 200
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 200
            }
        }

        Rectangle {
            id: handle
            opacity: control.mdState.contentOpacity
            x: Math.max(offset, Math.min(parent.width - offset - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2
            // We use scale to allow us to enlarge the circle from the center,
            // as using width/height will cause it to jump due to the position x/y bindings.
            // However, a large enough scale on certain displays will show the triangles
            // that make up the circle, so instead we make sure that the circle is always
            // its largest size so that more triangles are used, and downscale instead.
            width: normalSize * largestScale
            height: normalSize * largestScale
            radius: width / 2
            color: control.mdState.handleColor
            scale: normalSize / largestSize

            readonly property int offset: 2
            readonly property real normalSize: control.mdState.handleSize
            readonly property real largestSize: 28
            readonly property real largestScale: largestSize / normalSize
            readonly property bool hasIcon: control.icon.name.length > 0 || control.icon.source.toString().length > 0

            Behavior on x {
                enabled: !control.pressed
                SmoothedAnimation {
                    duration: 300
                }
            }

            Behavior on scale {
                NumberAnimation {
                    duration: 100
                }
            }

            Behavior on color {
                ColorAnimation {
                    duration: 200
                }
            }

            /*
            IconImage {
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                name: control.icon.name
                source: control.icon.source
                sourceSize: Qt.size(control.icon.width, control.icon.height)
                color: control.icon.color
                visible: handle.hasIcon
            }
            */

        }
        MD.Ripple2 {
            x: handle.x + handle.width / 2 - width / 2
            y: handle.y + handle.height / 2 - height / 2
            width: 28
            height: 28
            radius: parent.radius
            pressed: control.pressed
            pressX: control.pressX
            pressY: control.pressY
            color: control.mdState.stateLayerColor
            stateOpacity: control.mdState.stateLayerOpacity
        }
    }

    contentItem: Item {
        implicitWidth: control.indicator.width
        implicitHeight: control.indicator.height
    }

    MD.StateSwitch {
        id: m_sh
        item: control
    }
}
