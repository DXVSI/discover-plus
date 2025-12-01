pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.IBtStandard
    property MD.StateIconButton mdState: MD.StateIconButton {
        item: control
    }

    Binding {
        control.mdState.type: control.type
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: type == MD.Enum.IBtStandard || (type == MD.Enum.IBtOutlined && !control.checked)
    topInset: 4
    bottomInset: 4
    leftInset: 4
    rightInset: 4

    padding: 8
    spacing: 0

    icon.width: action ? action.icon.width : 24
    icon.height: action ? action.icon.height : 24

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        opacity: control.mdState.contentOpacity

        MD.Icon {
            anchors.centerIn: parent
            name: control.icon.name
            size: Math.min(control.icon.width, control.icon.height)
            color: control.mdState.textColor
            fill: control.checked
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 40
        implicitHeight: 40

        radius: Math.max(height / 2, 0)
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.type == MD.Enum.IBtOutlined ? 1 : 0
        border.color: control.mdState.ctx.color.outline

        elevationVisible: elevation && color.a > 0 && !control.flat
        elevation: control.mdState.elevation

        MD.Ripple2 {
            anchors.fill: parent
            radius: Math.max(height / 2, 0)
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

}
