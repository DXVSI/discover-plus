import QtQuick
import Qcm.Material as MD

MD.MState {
    id: root
    required property Item item
   

    textColor: root.ctx.color.outline
    backgroundColor: "transparent"

    property int radius: MD.Token.shape.corner.full
    property int handleWidth: 4
    property int handleHeight: 48
    property int handlePressedWidth: 12
    property int handlePressedHeight: 52

    states: [
        State {
            name: "Pressed"
            when: root.item.pressed
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.handleWidth: root.handlePressedWidth
                root.handleHeight: root.handlePressedHeight
                root.radius: MD.Token.shape.corner.medium
            }
        },
        State {
            name: "Focus"
            when: root.item.focus
            PropertyChanges {
                root.stateLayerColor: root.ctx.color.inverse_on_surface
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerColor: root.ctx.color.inverse_on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
