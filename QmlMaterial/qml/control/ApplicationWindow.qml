import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.ApplicationWindow {
    color: MD.MProp.backgroundColor

    property MD.typescale typescale: MD.Token.typescale.body_medium
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking
}
