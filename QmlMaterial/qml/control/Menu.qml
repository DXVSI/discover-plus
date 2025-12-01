pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Window
import QtQml.Models

import Qcm.Material as MD

T.Menu {
    id: control

    property alias mdState: item_state
    property bool autoClose: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    margins: 0
    verticalPadding: 8

    transformOrigin: !cascade ? Item.Top : (mirrored ? Item.TopRight : Item.TopLeft)

    delegate: MD.MenuItem {
        id: m_item

        function clickedCB() {
            if ((action as MD.Action)?.closeMenu || control.autoClose) {
                triggered();
            }
        }

        Component.onCompleted: {
            MD.Util.disconnectAll(m_item, "clicked()");
            m_item.clicked.connect(clickedCB);
        }
    }

    property var model: null
    property alias contentDelegate: m_model.delegate

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            to: 0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
        NumberAnimation {
            property: "scale"
            to: 0.8
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
    }

    DelegateModel {
        id: m_model
        model: control.model
    }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: {
            if (control.model) {
                if (control.contentDelegate) {
                    return m_model;
                } else if (control.model instanceof DelegateModel) {
                    return control.model;
                } else if (control.model != control.contentModel) {
                    return m_model;
                }
            }
            return control.contentModel;
        }
        interactive: Window.window ? contentHeight + control.topPadding + control.bottomPadding > Window.window.height : false
        keyNavigationEnabled: false
        T.ScrollIndicator.vertical: MD.ScrollIndicator {}
    }

    background: MD.ElevationRectangle {
        implicitWidth: 200
        implicitHeight: 48
        radius: MD.Token.shape.corner.extra_small
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    MD.MState {
        id: item_state

        elevation: MD.Token.elevation.level2
        textColor: MD.Token.color.on_surface
        backgroundColor: MD.Token.color.surface_container
        supportTextColor: MD.Token.color.on_surface_variant
    }
}
