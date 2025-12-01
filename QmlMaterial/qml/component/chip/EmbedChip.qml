pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateEmbedChip mdState: MD.StateEmbedChip {
        item: control
    }

    property alias elevated: control.mdState.elevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    checkable: false
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: m_leading_comp && MD.Util.hasIcon(icon) ? 8 : 16
    rightPadding: m_trailing_comp && trailingIconName ? 8 : 16
    spacing: 8

    icon.width: 18
    icon.height: 18
    action: null
    font.capitalization: Font.Capitalize

    property string trailingIconName
    property Component leading: m_leading_comp
    property Component trailing: m_trailing_comp

    Component {
        id: m_leading_comp
        MD.Icon {
            visible: name
            size: {
                const ic = control.action ? control.action.icon : control.icon;
                return Math.min(ic.width, ic.height);
            }
            name: {
                const ic = control.action ? control.action.icon : control.icon;
                return ic.name;
            }
        }
    }

    Component {
        id: m_trailing_comp
        MD.Icon {
            visible: name
            name: control.trailingIconName
            size: {
                const ic = control.action ? control.action.icon : control.icon;
                return Math.min(ic.width, ic.height);
            }
        }
    }

    contentItem: Item {
        implicitWidth: m_content_row.implicitWidth
        implicitHeight: m_content_row.implicitHeight

        Row {
            id: m_content_row
            opacity: control.mdState.contentOpacity
            width: Math.min(implicitWidth, parent.width)
            height: Math.min(implicitHeight, parent.height)
            anchors.centerIn: parent
            spacing: 4

            Loader {
                anchors.verticalCenter: parent.verticalCenter
                width: control.leading && item?.visible ? implicitWidth : 0
                height: control.leading && item?.visible ? implicitHeight : 0
                sourceComponent: control.leading
            }

            MD.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.label_large
            }

            Loader {
                anchors.verticalCenter: parent.verticalCenter
                width: control.trailing ? implicitWidth : 0
                height: control.trailing ? implicitHeight : 0
                sourceComponent: control.trailing
            }
        }
    }

    background: Rectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

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
