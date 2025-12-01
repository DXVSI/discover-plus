import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.CheckBox {
    id: control

    property MD.StateCheckBox mdState: MD.StateCheckBox {
        item: control
    }

    property bool error: false
    Binding {
        control.mdState.error: control.error
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: 8
    padding: 8
    verticalPadding: padding + 7

    indicator: Item {
        implicitHeight: 18
        implicitWidth: 18
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        MD.CheckShape {
            checkState: control.checkState
            color: control.mdState.iconColor
            backgroundColor: control.mdState.iconBackgroundColor
            outlineColor: control.mdState.outlineColor
        }
        MD.Ripple2 {
            anchors.centerIn: parent
            width: 40
            height: 40
            radius: 20
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

    contentItem: MD.Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        typescale: MD.Token.typescale.label_large
        text: control.text
        font: control.font
        color: control.mdState.textColor
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
}
