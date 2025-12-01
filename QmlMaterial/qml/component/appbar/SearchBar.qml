import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property bool leading_input: false
    property MD.Action leading_action: MD.Action {
        icon.name: MD.Token.icon.search
    }
    property bool trailing_input: control.text
    property MD.Action trailing_action: MD.Action {
        icon.name: control.text ? MD.Token.icon.close : ''
        onTriggered: {
            item_input.text = '';
        }
    }
    property MD.StateSearchBar mdState: MD.StateSearchBar {
        item: control
    }

    signal accepted

    text: item_input.text

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    leftPadding: 0
    rightPadding: item_trailing.visible ? 0 : 16

    contentItem: RowLayout {
        spacing: 0

        MD.IconButton {
            id: item_leading
            action: control.leading_action
            MD.InputBlock {
                when: !control.leading_input
                target: item_leading
            }
        }

        MD.TextFieldEmbed {
            id: item_input
            Layout.fillWidth: true
            color: control.mdState.textColor

            Connections {
                target: item_input
                function onAccepted() {
                    control.accepted();
                }
            }
        }

        MD.IconButton {
            id: item_trailing
            action: control.trailing_action
            visible: icon.name
            MD.InputBlock {
                when: !control.trailing_input
                target: item_trailing
            }
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 200
        implicitHeight: 56

        radius: height / 2
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

}
