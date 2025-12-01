import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Button item
   
    property bool elevated: false
    property color leadingIconColor: item.checked ? ctx.color.on_secondary_container : ctx.color.primary
    property color trailingIconColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface_variant

    elevation: elevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
    textColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface_variant
    outlineColor: item.checked ? ctx.color.secondary_container : ctx.color.outline_variant
    backgroundColor: {
        if (item.checked)
            return ctx.color.secondary_container;
        else if (elevated)
            return ctx.color.surface_container_low;
        else
            return 'transparent';
    }
    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface
                root.trailingIconColor: root.ctx.color.on_surface
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
                root.stateLayerColor: root.item.checked ? ctx.color.on_secondary_container : root.ctx.color.on_surface_variant
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: {
                    if (root.elevated)
                        return MD.Token.elevation.level2;
                    else if (root.item.checked)
                        return MD.Token.elevation.level1;
                    else
                        return MD.Token.elevation.level0;
                }
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.item.checked ? ctx.color.on_secondary_container : root.ctx.color.on_surface_variant
            }
        },
        State {
            name: "Focus"
            when: root.item.visualFocus
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.item.checked ? ctx.color.on_secondary_container : root.ctx.color.on_surface_variant
                root.outlineColor: item.checked ? ctx.color.secondary_container : ctx.color.on_surface_variant
            }
        }
    ]
}
