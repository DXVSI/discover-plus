pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.MenuItem {
    id: control

    property bool selected: false
    property MD.StateMenuItem mdState: MD.StateMenuItem {
        item: control
    }
    Binding {
        control.mdState.selected: control.selected
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 16
    verticalPadding: 0
    spacing: 16

    icon.width: 24
    icon.height: 24
    icon.color: mdState.textColor

    /*
    indicator: CheckIndicator {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checkable
        control: control
        checkState: control.checked ? Qt.Checked : Qt.Unchecked
    }
    */

    arrow: MD.Icon {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2

        visible: control.subMenu
        size: 24
        name: MD.Token.icon.arrow_right
    }

    property MD.typescale typescale: MD.Token.typescale.label_large
    font.capitalization: Font.Capitalize
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: MD.IconLabel {
        label.lineHeight: control.typescale.line_height
        horizontalAlignment: Qt.AlignLeft
        spacing: control.spacing

        text: control.text
        color: control.mdState.textColor

        icon.name: control.icon.name
        icon.size: control.icon.width
        icon.color: control.leadingIconColor
        icon.fill: control.checked

        icon_component: m_loading_comp
        icon_component_active: {
            const a = control.action;
            return a instanceof MD.Action && (a as MD.Action).busy;
        }

        Component {
            id: m_loading_comp
            MD.CircularIndicator {
                anchors.centerIn: parent
                running: true
                strokeWidth: 2
                implicitWidth: {
                    const w = Math.min(control.icon.width, control.icon.height);
                    return w - 6;
                }
                implicitHeight: implicitWidth
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 48
        color: control.mdState.backgroundColor

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

    property color leadingIconColor: control.mdState.leadingIconColor
    property color trailingIconColor: control.mdState.trailingIconColor
}
