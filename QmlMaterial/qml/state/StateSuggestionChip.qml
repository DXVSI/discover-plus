import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Button item
   

    property bool elevated: false
    property color iconColor: ctx.color.primary
    elevation: elevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
    textColor: ctx.color.on_surface_variant
    outlineColor: ctx.color.outline_variant
    backgroundColor: elevated ? ctx.color.surface_container_low : 'transparent'

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.iconColor: root.ctx.color.on_surface
                root.textColor: root.ctx.color.on_surface
                root.outlineColor: ctx.color.on_surface
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.down
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface_variant
                root.textColor: root.ctx.color.on_surface_variant
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: root.elevated ? MD.Token.elevation.level2 : MD.Token.elevation.level0
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface_variant
                root.textColor: root.ctx.color.on_surface_variant
            }
        },
        State {
            name: "Focus"
            when: root.item.visualFocus
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface_variant
                root.textColor: root.ctx.color.on_surface_variant
                root.outlineColor: root.ctx.color.on_surface_variant
            }
        }
    ]
}
