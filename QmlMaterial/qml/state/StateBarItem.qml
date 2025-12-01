import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Button item
   

    elevation: MD.Token.elevation.level0
    textColor: root.item.checked ? root.ctx.color.on_surface : root.ctx.color.on_surface_variant
    backgroundColor: root.item.checked ? root.ctx.color.secondary_container : "transparent"
    supportTextColor: root.item.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"

    states: [
        State {
            name: "Pressed"
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.item.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.item.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
