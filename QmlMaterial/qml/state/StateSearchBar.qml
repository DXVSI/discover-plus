import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface_container_highest
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"//root.ctx.color.surface_tint
    outlineColor: root.ctx.color.outline

    required property T.Button item
   

    property real placeholderOpacity: 1.0

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.supportTextColor: root.ctx.color.on_surface
                root.placeholderOpacity: 0.38
                root.item.background.opacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.pressed || root.item.visualFocus
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
