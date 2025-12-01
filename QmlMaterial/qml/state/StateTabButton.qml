import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.TabButton item
    property int type
   

    property color baseTextColor: root.type == MD.Enum.PrimaryTab ? (root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface) : (root.item.checked ? root.ctx.color.on_surface : root.ctx.color.on_surface_variant)
    elevation: MD.Token.elevation.level0
    textColor: baseTextColor
    backgroundColor: ctx.color.surface
    stateLayerColor: "transparent"

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.textColor: root.type == MD.Enum.PrimaryTab ? root.baseTextColor : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.type == MD.Enum.PrimaryTab ? root.ctx.color.primary : root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.textColor: root.type == MD.Enum.PrimaryTab ? root.baseTextColor : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.type == MD.Enum.PrimaryTab ? root.ctx.color.primary : root.ctx.color.on_surface
            }
        }
    ]
}
