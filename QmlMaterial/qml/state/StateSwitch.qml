import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Switch item
   

    elevation: MD.Token.elevation.level1
    textColor: root.item.checked ? root.ctx.color.on_primary_container : root.ctx.color.surface_container_highest
    backgroundColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.surface_container_highest
    stateLayerColor: "transparent"
    property color handleColor: root.item.checked ? root.ctx.color.on_primary : root.ctx.color.outline
    property int handleSize: root.item.checked ? 24 : 16

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.down
            PropertyChanges {
                root.textColor: root.item.checked ? root.ctx.color.on_primary_container : root.ctx.color.surface_container_highest
                root.backgroundColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.surface_container_highest
                root.handleColor: root.item.checked ? root.ctx.color.primary_container : root.ctx.color.on_surface_variant
                root.handleSize: 28
                root.stateLayerColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.textColor: root.item.checked ? root.ctx.color.on_primary_container : root.ctx.color.surface_container_highest
                root.backgroundColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.surface_container_highest
                root.handleColor: root.item.checked ? root.ctx.color.primary_container : root.ctx.color.on_surface_variant
                root.stateLayerColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
