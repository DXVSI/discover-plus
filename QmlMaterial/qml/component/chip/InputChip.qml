import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQml.Models
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateInputChip mdState: MD.StateInputChip {
        item: control
    }

    property bool edit: false
    property alias elevated: control.mdState.elevated
    property Item leadingItem: null
    property Component editDelegate: null

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    checkable: false
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: leadingItem ? 8 : 16
    rightPadding: icon.name ? 8 : 16
    spacing: 8

    icon.width: 18
    icon.height: 18

    action: null
    contentItem: contentItemRO

    property Item contentItemRO: Row {
        opacity: control.mdState.contentOpacity
        spacing: control.spacing
        Row {
            id: m_leading
            anchors.verticalCenter: parent.verticalCenter
            data: control.leadingItem ? [control.leadingItem] : []
        }
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            MD.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.label_large
            }
            Item {
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: control.icon.width
                implicitHeight: control.icon.height
                visible: control.icon.name

                MD.StandardIconButton {
                    anchors.centerIn: parent

                    implicitBackgroundSize: 0
                    icon: control.icon
                    action: control.action
                }
            }
        }
    }

    background: Rectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.checked ? 0 : 1
        border.color: control.mdState.outlineColor

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

    onFocusChanged: {
        if (focus)
            control.contentItem.focus = true;
    }

    Instantiator {
        model: control.edit ? 1 : null
        delegate: control.editDelegate
        onObjectAdded: (idx, obj) => {
            control.contentItem = obj;
            obj.focus = true;
        }
        onObjectRemoved: (idx, obj) => {
            control.contentItem = control.contentItemRO;
            control.contentItem.visible = true;
        }
    }
}
