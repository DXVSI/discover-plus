import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Dialog {
    id: control

    property int titleCapitalization: Font.Capitalize
    property MD.MState mdState: MD.MState {
        target: control

        elevation: MD.Token.elevation.level3
        textColor: ctx.color.primary
        backgroundColor: ctx.color.surface_container_high
        supportTextColor: ctx.color.on_surface_variant
    }
    property MD.typescale typescale: MD.Token.typescale.body_medium

    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    width: Math.min(implicitWidth, 560)
    height: Math.min(implicitHeight, parent.height - 48 * 2)

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    spacing: 0
    topPadding: 16
    bottomPadding: 24
    horizontalPadding: 24

    modal: true

    enter: Transition {
        // grow_fade_in
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    exit: Transition {
        // shrink_fade_out
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.9
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 280
        // FullScale doesn't make sense for Dialog.
        radius: MD.Token.shape.corner.extra_large
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }

    header: MD.Control {
        topPadding: 24
        horizontalPadding: 24
        contentItem: ColumnLayout {
            spacing: 0
            MD.Text {
                visible: control.title
                text: control.title
                typescale: MD.Token.typescale.headline_small
                color: control.mdState.ctx.color.on_surface
                font.capitalization: control.titleCapitalization
            }
        }
    }

    footer: MD.DialogButtonBox {
        bottomPadding: 24
        visible: count > 0
    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(control.mdState.ctx.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(control.mdState.ctx.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }
}
