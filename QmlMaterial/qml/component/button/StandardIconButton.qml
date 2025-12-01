import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateStandardIconButton mdState: MD.StateStandardIconButton {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    topInset: 4
    bottomInset: 4
    leftInset: 4
    rightInset: 4

    padding: 8
    spacing: 0

    icon.width: 24
    icon.height: 24

    property int implicitBackgroundSize: 40
    property int backgroundRadius: background.height / 2
    property alias iconFill: m_icon.fill

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        opacity: control.mdState.contentOpacity

        MD.Icon {
            id: m_icon

            anchors.centerIn: parent
            name: control.icon.name
            size: Math.min(control.icon.width, control.icon.height)
            color: control.mdState.textColor
        }
    }

    background: Rectangle {
        implicitWidth: control.implicitBackgroundSize
        implicitHeight: control.implicitBackgroundSize

        radius: control.backgroundRadius
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.type == MD.Enum.IBtOutlined ? 1 : 0
        border.color: control.mdState.ctx.color.outline

        MD.Ripple2 {
            anchors.fill: parent
            radius: control.backgroundRadius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

}
