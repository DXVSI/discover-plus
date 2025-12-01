import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int type
   

    elevation: MD.Token.elevation.level1
    stateLayerColor: "transparent"

    textColor: {
        switch (root.type) {
        case MD.Enum.IBtOutlined:
            if (root.item.checked)
                return root.ctx.color.inverse_on_surface;
            else
                return root.ctx.color.on_surface_variant;
        case MD.Enum.IBtStandard:
            if (root.item.checked)
                return root.ctx.color.primary;
            else
                return root.ctx.color.on_surface_variant;
        case MD.Enum.IBtFilledTonal:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.on_secondary_container;
            else
                return root.ctx.color.on_surface_variant;
        case MD.Enum.IBtFilled:
        default:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.on_primary;
            else
                return root.ctx.color.primary;
        }
    }
    backgroundColor: {
        switch (root.type) {
        case MD.Enum.IBtStandard:
            return "transparent";
        case MD.Enum.IBtOutlined:
            if (root.item.checked)
                return root.ctx.color.inverse_surface;
            else
                return "transparent";
        case MD.Enum.IBtFilledTonal:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.secondary_container;
            else
                return root.ctx.color.surface_container_highest;
        case MD.Enum.IBtFilled:
        default:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.primary;
            else
                return root.ctx.color.surface_container_highest;
        }
    }
    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: {
                    switch (root.type) {
                    case MD.Enum.IBtOutlined:
                    case MD.Enum.IBtStandard:
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
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.elevation: MD.Token.elevation.level1
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.IBtFilled:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_primary;
                        else
                            c = root.ctx.color.primary;
                        break;
                    case MD.Enum.IBtFilledTonal:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_secondary_container;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    case MD.Enum.IBtOutlined:
                        if (root.item.checked)
                            c = root.ctx.color.inverse_on_surface;
                        else
                            c = root.ctx.color.on_surface;
                        break;
                    case MD.Enum.IBtStandard:
                    default:
                        if (root.item.checked)
                            c = root.ctx.color.primary;
                        else
                            c = root.ctx.color.on_surface_variant;
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
                    case MD.Enum.IBtFilled:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_primary;
                        else
                            c = root.ctx.color.primary;
                        break;
                    case MD.Enum.IBtFilledTonal:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_secondary_container;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    case MD.Enum.IBtOutlined:
                        if (root.item.checked)
                            c = root.ctx.color.inverse_on_surface;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    default:
                    case MD.Enum.IBtStandard:
                        if (root.item.checked)
                            c = root.ctx.color.primary;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    }
                    return c;
                }
            }
        }
    ]
}
