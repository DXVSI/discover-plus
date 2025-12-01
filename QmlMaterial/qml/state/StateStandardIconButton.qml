import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
   

    stateLayerColor: "transparent"
    textColor: {
        if (root.item.checked)
            return root.ctx.color.primary;
        else
            return root.ctx.color.on_surface_variant;
    }
    backgroundColor: "transparent"
    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: {
                    if (root.item.checked)
                        return root.ctx.color.primary;
                    else
                        return root.ctx.color.on_surface_variant;
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: {
                    if (root.item.checked)
                        return root.ctx.color.primary;
                    else
                        return root.ctx.color.on_surface_variant;
                }
            }
        }
    ]
}
