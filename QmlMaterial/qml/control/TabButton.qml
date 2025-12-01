import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TabButton {
    id: control

    property int type: MD.Enum.PrimaryTab
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    property MD.StateTabButton mdState: MD.StateTabButton {
        item: control
    }

    Binding {
        control.mdState.type: control.type
    }

    // use checked instead
    // property bool active: T.TabBar.index === T.TabBar.tabBar.currentIndex

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    spacing: 8

    leftPadding: 12
    rightPadding: 12

    icon.width: 24
    icon.height: 24

    property MD.typescale typescale: MD.Token.typescale.label_medium
    font.capitalization: Font.Capitalize
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: MD.IconLabel {
        text: control.text
        color: control.mdState.textColor
        style: control.iconStyle
        icon.name: control.icon.name
        icon.size: control.icon.width
        opacity: control.mdState.contentOpacity

        label.lineHeight: control.typescale.line_height
    }

    background: MD.Ripple2 {
        implicitHeight: 48

        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: control.mdState.stateLayerOpacity
        color: control.mdState.stateLayerColor
        // opacity: control.mdState.backgroundOpacity
    }
}
