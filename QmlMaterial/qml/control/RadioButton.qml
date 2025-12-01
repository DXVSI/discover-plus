import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.RadioButton {
    id: control
    property MD.StateRadioButton mdState: MD.StateRadioButton {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: 8
    padding: 8
    verticalPadding: padding + 6

    indicator: Item {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitHeight: 20
        implicitWidth: 20
        MD.RadioIndicatorShape {
            anchors.fill: parent
            opacity: control.mdState.contentOpacity
            color: control.mdState.iconColor
            outlineColor: color
            focusWidth: control.visualFocus ? 2.5 : 0
            fillFactor: control.checked ? 1.0 : 0.0

            Behavior on fillFactor {
                NumberAnimation {
                    duration: MD.Token.duration.short3
                }
            }
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
        opacity: control.mdState.contentOpacity
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
}
