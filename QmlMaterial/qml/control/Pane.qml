import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Pane {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    padding: 0
    property int radius: 0
    property MD.corners corners: MD.Util.corners(radius)
    property int elevation: MD.Token.elevation.level0
    property color backgroundColor: MD.MProp.color.background
    property alias showBackground: control.background.visible

    background: MD.ElevationRectangle {
        color: control.backgroundColor
        corners: control.corners
        elevation: control.elevation
    }
}
