import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int type: MD.Enum.BtElevated
   

    elevation: MD.Token.elevation.level1
    textColor: {
        switch (root.type) {
        case MD.Enum.BtFilled:
            return ctx.color.on_primary;
        case MD.Enum.BtFilledTonal:
            return ctx.color.on_secondary_container;
        case MD.Enum.BtOutlined:
        case MD.Enum.BtText:
        case MD.Enum.BtElevated:
        default:
            return ctx.color.primary;
        }
    }
    backgroundColor: {
        switch (root.type) {
        case MD.Enum.BtFilled:
            return ctx.color.primary;
        case MD.Enum.BtFilledTonal:
            return ctx.color.secondary_container;
        case MD.Enum.BtOutlined:
        case MD.Enum.BtText:
            return "transparent";
        case MD.Enum.BtElevated:
        default:
            return ctx.color.surface_container_low;
        }
    }
    stateLayerColor: "transparent"

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: {
                    switch (root.type) {
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                        return "transparent";
                    default:
                        return root.ctx.color.on_surface;
                    }
                }
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.12
            }
        },
        State {
            name: "Pressed"
            when: root.item.down
            PropertyChanges {
                root.elevation: MD.Token.elevation.level1
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.BtFilled:
                    case MD.Enum.BtFilledTonal:
                        c = root.ctx.color.getOn(root.backgroundColor);
                        break;
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                    case MD.Enum.BtElevated:
                    default:
                        c = root.ctx.color.primary;
                    }
                    return c;
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: MD.Token.elevation.level2
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.BtFilled:
                    case MD.Enum.BtFilledTonal:
                        c = root.ctx.color.getOn(root.backgroundColor);
                        break;
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                    case MD.Enum.BtElevated:
                    default:
                        c = root.ctx.color.primary;
                    }
                    return c;
                }
            }
        },
        State {
            name: "Focus"
            when: root.item.visualFocus
            PropertyChanges {
                root.elevation: MD.Token.elevation.level1
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.BtFilled:
                    case MD.Enum.BtFilledTonal:
                        c = root.ctx.color.getOn(root.backgroundColor);
                        break;
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                    case MD.Enum.BtElevated:
                    default:
                        c = root.ctx.color.primary;
                    }
                    return c;
                }
            }
        }
    ]
}
