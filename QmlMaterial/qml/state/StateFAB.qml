import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int color: MD.Enum.FABColorPrimary

   

    elevation: MD.Token.elevation.level3
    textColor: {
        switch (root.color) {
        case MD.Enum.FABColorSurfaec:
            return root.ctx.color.primary;
        case MD.Enum.FABColorSecondary:
            return root.ctx.color.on_secondary_container;
        case MD.Enum.FABColorTertiary:
            return root.ctx.color.on_tertiary_container;
        case MD.Enum.FABColorPrimary:
        default:
            return root.ctx.color.on_primary_container;
        }
    }
    backgroundColor: {
        switch (root.color) {
        case MD.Enum.FABColorSurfaec:
            return root.ctx.color.surface_container_high;
        case MD.Enum.FABColorSecondary:
            return root.ctx.color.secondary_container;
        case MD.Enum.FABColorTertiary:
            return root.ctx.color.tertiary_container;
        case MD.Enum.FABColorPrimary:
        default:
            return root.ctx.color.primary_container;
        }
    }
    stateLayerColor: textColor

    states: [
        State {
            name: "Pressed"
            when: root.item.pressed || root.item.visualFocus
            PropertyChanges {
                root.elevation: MD.Token.elevation.level3
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: MD.Token.elevation.level4
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
