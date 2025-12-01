import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.RadioButton item
   

    property color iconColor: item.checked ? ctx.color.primary : ctx.color.on_surface_variant
    textColor: ctx.color.on_surface
    backgroundColor: "transparent"

    states: [
        State {
            name: "disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.backgroundColor: ctx.color.on_surface
                root.iconColor: root.ctx.color.on_surface
                root.contentOpacity: 0.38
            }
        },
        State {
            name: "pressed"
            when: root.item.pressed
            PropertyChanges {
                root.iconColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerColor: root.item.checked ? root.ctx.color.on_surface : root.ctx.color.primary
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "hovered"
            when: root.item.hovered
            PropertyChanges {
                root.iconColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.iconColor
            }
        },
        State {
            name: "focus"
            when: root.item.visualFocus
            PropertyChanges {
                root.iconColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerColor: root.iconColor
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
            }
        }
    ]
}
