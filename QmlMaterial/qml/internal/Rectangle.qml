import QtQuick
import Qcm.Material as MD

Rectangle {
    property MD.corners corners: radius

    bottomLeftRadius: corners.bottomLeft
    bottomRightRadius: corners.bottomRight
    topLeftRadius: corners.topLeft
    topRightRadius: corners.topRight
}
