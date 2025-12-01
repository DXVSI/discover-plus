import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T
import Qcm.Material as MD

T.ProgressBar {
    id: root
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: 0

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32
        MD.CircleProgressShape {
            anchors.fill: parent
            progress: root.value / root.to
        }
    }
    background: null
}
