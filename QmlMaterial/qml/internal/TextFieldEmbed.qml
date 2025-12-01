import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TextField {
    id: root
    property MD.typescale typescale: MD.Token.typescale.body_large
    property bool prominent: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    font.capitalization: Font.MixedCase

    Binding {
        when: root.typescale
        root.font.pixelSize: root.typescale.size
        root.font.weight: root.prominent && root.typescale.weight_prominent ? root.typescale.weight_prominent : root.typescale.weight
        root.font.letterSpacing: root.typescale.tracking
        restoreMode: Binding.RestoreNone
    }

    cursorDelegate: MD.CursorDelegate {
    }
    selectionColor: MD.Token.color.primary
    selectedTextColor: MD.Token.color.getOn(selectionColor)

    verticalAlignment: TextInput.AlignVCenter
    color: MD.MProp.textColor
}
