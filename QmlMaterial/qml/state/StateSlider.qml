import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Slider item
   

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_primary
    backgroundColor: root.ctx.color.primary
    supportTextColor: root.ctx.color.on_primary
    stateLayerColor: "#00000000"

    property color trackColor: root.backgroundColor
    property color trackOverlayColor: root.backgroundColor
    property color trackInactiveColor: root.ctx.color.surface_container_highest
    property color trackMarkInactiveColor: root.ctx.color.on_surface_variant
    property color trackMarkColor: root.supportTextColor

    property real trackOverlayOpacity: 0.12

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.trackInactiveColor: root.ctx.color.on_surface
                root.backgroundOpacity: 0.38
            }
        },
        State {
            name: "Pressed"
            when: root.item.pressed || root.item.visualFocus
            PropertyChanges {
                root.stateLayerColor: {
                    const c = root.ctx.color.primary;
                    return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerColor: {
                    const c = root.ctx.color.primary;
                    return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                }
            }
        }
    ]
}
