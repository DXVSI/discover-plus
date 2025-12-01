import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.MenuItem item
    property bool selected: false
   

    elevation: MD.Token.elevation.level2
    textColor: root.ctx.color.on_surface
    backgroundColor: root.selected ? root.ctx.color.secondary_container : "transparent"
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"
    property color leadingIconColor: root.ctx.color.on_surface
    property color trailingIconColor: root.ctx.color.on_surface

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.item.contentItem.opacity: 0.38
            }
        },
        State {
            name: "Pressed"
            when: root.item.down
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Focused"
            when: root.item.visualFocus
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
