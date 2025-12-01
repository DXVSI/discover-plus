pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Slider {
    id: control

    property MD.StateSlider mdState: MD.StateSlider {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitHandleHeight + topPadding + bottomPadding)

    horizontalPadding: 0
    topInset: 0
    bottomInset: 0
    topPadding: 0
    bottomPadding: 0
    clip: false

    property vector2d overlay: Qt.vector2d(0, 0)

    // The Slider is discrete if all of the following requirements are met:
    // * stepSize is positive
    // * snapMode is set to SnapAlways
    // * the difference between to and from is cleanly divisible by the stepSize
    // * the number of tick marks intended to be rendered is less than the width to height ratio, or vice versa for vertical sliders.
    readonly property real __steps: Math.abs(to - from) / stepSize
    readonly property bool __isDiscrete: stepSize >= Number.EPSILON && snapMode === Slider.SnapAlways && Math.abs(Math.round(__steps) - __steps) < Number.EPSILON && Math.floor(__steps) < (horizontal ? background.width / background.height : background.height / background.width)

    handle: MD.SliderHandle {
        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
        value: control.value
        handleHasFocus: control.visualFocus
        handlePressed: control.pressed
        handleHovered: control.hovered
    }

    background: Item {
        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)
        implicitWidth: control.horizontal ? 200 : 4
        implicitHeight: control.horizontal ? 4 : 200
        opacity: control.mdState.backgroundOpacity

        Rectangle {
            anchors.centerIn: parent
            width: control.horizontal ? parent.width - (control.implicitHandleWidth - (control.__isDiscrete ? 4 : 0)) : 4
            height: control.horizontal ? 4 : parent.height - (control.implicitHandleHeight - (control.__isDiscrete ? 4 : 0))
            scale: control.horizontal && control.mirrored ? -1 : 1
            radius: Math.min(width, height) / 2
            color: control.mdState.trackInactiveColor

            Rectangle {
                visible: control.overlay.length() > 0
                x: control.horizontal ? control.overlay.x * parent.width : (parent.width - width) / 2
                y: control.horizontal ? (parent.height - height) / 2 : control.overlay.y * parent.height
                width: control.horizontal ? (control.overlay.y - control.overlay.x) * parent.width : 4
                height: control.horizontal ? 4 : (control.overlay.y - control.overlay.x) * parent.height
                radius: Math.min(width, height) / 2
                color: control.mdState.trackOverlayColor
                opacity: control.mdState.trackOverlayOpacity
            }

            Rectangle {
                x: control.horizontal ? 0 : (parent.width - width) / 2
                y: control.horizontal ? (parent.height - height) / 2 : control.visualPosition * parent.height
                width: control.horizontal ? control.position * parent.width : 4
                height: control.horizontal ? 4 : control.position * parent.height
                radius: Math.min(width, height) / 2
                color: control.mdState.trackColor
            }

            // Declaring this as a property (in combination with the parent binding below) avoids ids,
            // which prevent deferred execution.
            property Repeater repeater: Repeater {
                parent: control.background.children[0]
                model: control.__isDiscrete ? Math.floor(control.__steps) + 1 : 0
                delegate: Rectangle {
                    width: 2
                    height: 2
                    radius: 2
                    x: control.horizontal ? (parent.width - width * 2) * currentPosition + (width / 2) : (parent.width - width) / 2
                    y: control.horizontal ? (parent.height - height) / 2 : (parent.height - height * 2) * currentPosition + (height / 2)
                    color: active ? control.mdState.trackMarkColor : control.mdState.trackMarkInactiveColor

                    required property int index
                    readonly property real currentPosition: index / (parent.repeater.count - 1)
                    readonly property bool active: (control.horizontal && control.visualPosition > currentPosition) || (!control.horizontal && control.visualPosition <= currentPosition)
                }
            }
        }
    }

}
