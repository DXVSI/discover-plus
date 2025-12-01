import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Qcm.Material as MD

T.Pane {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    property alias text: label.text
    property MD.StateSnakeBar mdState: MD.StateSnakeBar {
        item: control
    }
    property MD.StateSnakeBar mdStateAction: mdState
    property MD.StateSnakeBar mdStateIcon: mdState
    readonly property alias actionControl: btn_action
    readonly property alias iconControl: btn_icon

    property MD.Action action: null
    property bool showClose: false
    property real radius: MD.Token.shape.corner.extra_small

    signal closed

    padding: 0
    leftPadding: 16
    rightPadding: showClose ? 8 : (action ? 8 : 16)

    background: MD.ElevationRectangle {
        implicitHeight: 48
        color: control.mdState.backgroundColor
        radius: control.radius
        elevation: control.mdState.elevation
    }
    RowLayout {
        id: icon_layout
        anchors.fill: parent
        spacing: 8

        MD.Text {
            id: label

            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
            typescale: MD.Token.typescale.body_medium
            color: control.mdState.supportTextColor
        }

        T.Button {
            id: btn_action
            flat: true
            font.capitalization: Font.Capitalize
            visible: !!action
            action: control.action

            implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
            implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

            topInset: 0
            bottomInset: 0
            verticalPadding: 0
            leftPadding: 0
            rightPadding: 0
            spacing: 8

            contentItem: MD.IconLabel {
                text: btn_action.text
                color: control.mdState.textColor
                label.typescale: MD.Token.typescale.label_large
                label.useTypescale: true
            }

            background: Rectangle {
                implicitWidth: 64
                implicitHeight: 48 - 8 * 2
                radius: height / 2
                border.width: 0
                color: "transparent"

                MD.Ripple2 {
                    anchors.fill: parent
                    radius: parent.radius
                    pressX: btn_action.pressX
                    pressY: btn_action.pressY
                    pressed: btn_action.pressed
                    stateOpacity: control.mdState.stateLayerOpacity
                    color: control.mdState.stateLayerColor
                }
            }
        }

        T.Button {
            id: btn_icon
            flat: true
            visible: control.showClose

            implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
            implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

            topInset: 0
            bottomInset: 0
            rightInset: 0
            leftInset: 0
            padding: 0
            spacing: 8

            contentItem: Item {
                implicitWidth: 24
                implicitHeight: 24
                MD.Icon {
                    anchors.centerIn: parent
                    name: MD.Token.icon.close
                    size: 24
                    color: control.mdState.iconColor
                }
            }

            background: Rectangle {
                implicitWidth: 48 - 8 * 2
                implicitHeight: 48 - 8 * 2
                radius: height / 2
                border.width: 0
                color: "transparent"

                MD.Ripple2 {
                    anchors.fill: parent
                    radius: parent.radius
                    pressX: btn_icon.pressX
                    pressY: btn_icon.pressY
                    pressed: btn_icon.pressed
                    stateOpacity: control.mdState.iconStateLayerOpacity
                    color: control.mdState.iconStateLayerColor
                }
            }

            onClicked: control.closed()
        }
    }
}
