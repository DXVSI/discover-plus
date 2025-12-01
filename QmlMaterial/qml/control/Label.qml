import QtQuick
import QtQuick.Window
import QtQuick.Templates as T

import Qcm.Material as MD

T.Label {
    id: root

    linkColor: root.palette.link

    property MD.typescale typescale: MD.Token.typescale.label_medium
    property bool prominent: false
    property bool useTypescale: true

    Binding {
        when: root.useTypescale
        root.lineHeight: root.typescale.line_height
        root.font.pixelSize: root.typescale.size
        root.font.weight: root.prominent ? root.typescale.weight_prominent : root.typescale.weight
        root.font.letterSpacing: root.typescale.tracking
    }

    antialiasing: true
    color: MD.MProp.textColor
    lineHeightMode: Text.FixedHeight
    wrapMode: Text.Wrap
    elide: Text.ElideRight
    textFormat: Text.PlainText
}
