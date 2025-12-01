import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Button item
    property int type: MD.Enum.CardElevated
    property int radius: 12
   

    elevation: {
        switch (root.type) {
        case MD.Enum.CardOutlined:
        case MD.Enum.CardFilled:
            return MD.Token.elevation.level0;
        case MD.Enum.CardElevated:
        default:
            return MD.Token.elevation.level1;
        }
    }
    textColor: root.ctx.color.getOn(backgroundColor)
    backgroundColor: {
        switch (root.type) {
        case MD.Enum.CardOutlined:
            return root.ctx.color.surface;
        case MD.Enum.CardFilled:
            return root.ctx.color.surface_container_highest;
        case MD.Enum.CardElevated:
        default:
            return root.ctx.color.surface_container_low;
        }
    }
    stateLayerColor: "transparent"

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: {
                    switch (root.type) {
                    case MD.Enum.CardFilled:
                        return MD.Token.elevation.level1;
                    case MD.Enum.CardOutlined:
                    case MD.Enum.CardElevated:
                    default:
                        return MD.Token.elevation.level0;
                    }
                }
                root.backgroundColor: root.ctx.color.surface_variant
                root.item.contentItem.opacity: 0.38
                root.item.background.opacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.elevation: {
                    switch (root.type) {
                    case MD.Enum.CardOutlined:
                    case MD.Enum.CardFilled:
                        return MD.Token.elevation.level0;
                    case MD.Enum.CardElevated:
                    default:
                        return MD.Token.elevation.level1;
                    }
                }
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: {
                    switch (root.type) {
                    case MD.Enum.CardOutlined:
                    case MD.Enum.CardFilled:
                        return MD.Token.elevation.level1;
                    case MD.Enum.CardElevated:
                    default:
                        return MD.Token.elevation.level2;
                    }
                }
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
