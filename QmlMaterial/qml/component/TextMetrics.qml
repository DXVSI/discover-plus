import QtQuick

import Qcm.Material as MD

TextMetrics {
    id: root
    property MD.typescale typescale: MD.Token.typescale.label_medium
    property bool prominent: false
    font.pixelSize: root.typescale ? root.typescale?.size : 16
    font.weight: root.typescale ? (root.prominent ? root.typescale.weight_prominent : typescale.weight) : Font.Normal
    font.letterSpacing: root.typescale ? root.typescale.tracking : 1
    elide: Text.ElideRight
}
