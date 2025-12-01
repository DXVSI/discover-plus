import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.CheckBox item
    property bool error: false
   

    outlineColor: {
        if (root.error) {
            return ctx.color.error;
        } else {
            return ctx.color.on_surface_variant;
        }
    }
    property color iconColor: root.error ? ctx.color.on_primary : ctx.color.on_error
    property color iconBackgroundColor: root.error ? ctx.color.error : ctx.color.primary
    textColor: ctx.color.on_surface
    backgroundColor: "transparent"

    states: [
        State {
            name: "disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.outlineColor: ctx.color.on_surface
                root.iconBackgroundColor: ctx.color.on_surface
                root.iconColor: root.ctx.color.on_surface
                root.contentOpacity: 0.38
            }
        },
        State {
            name: "pressed"
            when: root.item.pressed
            PropertyChanges {
                root.stateLayerColor: root.error ? ctx.color.error : (root.item.checked ? ctx.color.primary : ctx.color.on_surface)
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.outlineColor: {
                    if (root.error) {
                        return ctx.color.error;
                    } else {
                        return ctx.color.on_surface;
                    }
                }
            }
        },
        State {
            name: "hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.error ? ctx.color.error : (root.item.checked ? ctx.color.primary : ctx.color.on_surface)
                root.outlineColor: {
                    if (root.error) {
                        return ctx.color.error;
                    } else {
                        return ctx.color.on_surface;
                    }
                }
            }
        },
        State {
            name: "focus"
            when: root.item.visualFocus
            PropertyChanges {
                root.stateLayerColor: root.error ? ctx.color.error : (root.item.checked ? ctx.color.primary : ctx.color.on_surface)
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.outlineColor: {
                    if (root.error) {
                        return ctx.color.error;
                    } else {
                        return ctx.color.on_surface;
                    }
                }
            }
        }
    ]
}
