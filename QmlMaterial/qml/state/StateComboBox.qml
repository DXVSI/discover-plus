import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.ComboBox item
   

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: "transparent"
    supportTextColor: root.ctx.color.on_surface_variant
    outlineColor: root.ctx.color.outline

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.supportTextColor: root.ctx.color.on_surface
                root.item.contentItem.opacity: 0.38
                root.item.background.opacity: 0.12
            }
        },
        State {
            name: "Error"
            when: !root.item.acceptableInput && !root.item.hovered
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.error
            }
        },
        State {
            name: "ErrorHover"
            when: !root.item.acceptableInput && root.item.hovered
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.on_error_container
            }
        },
        State {
            name: "Focused"
            when: root.item.visualFocus
            PropertyChanges {
                root.outlineColor: root.ctx.color.primary
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.outlineColor: root.ctx.color.on_surface
            }
        }
    ]
}
