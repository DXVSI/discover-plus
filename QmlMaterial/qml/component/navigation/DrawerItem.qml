import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.ItemDelegate {
    id: control

    property alias mdState: item_state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    verticalPadding: 0
    leftPadding: 16
    rightPadding: 24
    spacing: 0
    checked: false

    icon.width: 24
    icon.height: 24

    font.capitalization: Font.Capitalize

    property alias trailing: item_holder_trailing.contentItem

    contentItem: RowLayout {
        spacing: 12

        MD.Icon {
            id: item_holder_leader
            name: control.icon.name
            size: Math.min(control.icon.width, control.icon.height)
            fill: control.checked
        }
        MD.Text {
            Layout.fillWidth: true
            font: control.font
            text: control.text
            typescale: MD.Token.typescale.label_large
            maximumLineCount: control.maximumLineCount
            verticalAlignment: Qt.AlignVCenter
        }
        MD.Control {
            id: item_holder_trailing
            Layout.alignment: Qt.AlignVCenter
            visible: contentItem
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 336
        implicitHeight: 56

        radius: 28
        color: control.mdState.backgroundColor

        elevationVisible: control.enabled && color.a > 0
        elevation: control.mdState.elevation

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: item_state.stateLayerOpacity
            color: item_state.stateLayerColor
        }
    }

    MD.StateDrawerItem {
        id: item_state
        item: control
    }
}
