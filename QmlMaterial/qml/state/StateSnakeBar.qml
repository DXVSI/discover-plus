import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Control item

    elevation: MD.Token.elevation.level3
    textColor: ctx.color.inverse_primary
    supportTextColor: ctx.color.inverse_on_surface
    backgroundColor: ctx.color.inverse_surface
    stateLayerColor: "transparent"
    property color iconColor: ctx.color.inverse_on_surface
    property color iconStateLayerColor: "transparent"
    property real iconStateLayerOpacity: 0.0

    states: [
        State {
            name: "Hovered:Action"
            when: root.item.actionControl.hovered
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.inverse_primary
                root.textColor: root.ctx.color.inverse_primary
            }
        },
        State {
            name: "Focus:Action"
            when: root.item.actionControl.visualFocus
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.ctx.color.inverse_primary
                root.textColor: root.ctx.color.inverse_primary
            }
        },
        State {
            name: "Pressed:Action"
            when: root.item.actionControl.down
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.inverse_primary
                root.textColor: root.ctx.color.inverse_primary
            }
        },
        State {
            name: "Hovered:Icon"
            when: root.item.iconControl.hovered
            PropertyChanges {
                root.iconStateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.iconStateLayerColor: root.ctx.color.inverse_on_surface
                root.iconColor: root.ctx.color.inverse_on_surface
            }
        },
        State {
            name: "Focus:Icon"
            when: root.item.iconControl.visualFocus
            PropertyChanges {
                root.iconStateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.iconStateLayerColor: root.ctx.color.inverse_on_surface
                root.iconColor: root.ctx.color.inverse_on_surface
            }
        },
        State {
            name: "Pressed:Icon"
            when: root.item.iconControl.down
            PropertyChanges {
                root.iconStateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.iconStateLayerColor: root.ctx.color.inverse_on_surface
                root.iconColor: root.ctx.color.inverse_on_surface
            }
        }
    ]
}
