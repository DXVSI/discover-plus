import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

ShapePath {
    id: root
    strokeWidth: 2
    strokeColor: "black"
    fillColor: "transparent"
    fillRule: ShapePath.WindingFill
    property int width
    property int height
    property MD.corners corners

    startX: root.corners.topLeft
    startY: 0

    PathLine {
        x: root.width - root.corners.topRight
        y: 0
    }
    PathArc {
        x: root.width
        y: root.corners.topRight
        radiusX: root.corners.topRight
        radiusY: root.corners.topRight
    }

    PathLine {
        x: root.width
        y: root.height - root.corners.bottomRight
    }
    PathArc {
        x: root.width - root.corners.bottomRight
        y: root.height
        radiusX: root.corners.bottomRight
        radiusY: root.corners.bottomRight
    }

    PathLine {
        x: root.corners.bottomLeft
        y: root.height
    }
    PathArc {
        x: 0
        y: root.height - root.corners.bottomLeft
        radiusX: root.corners.bottomLeft
        radiusY: root.corners.bottomLeft
    }

    PathLine {
        x: 0
        y: root.corners.topLeft
    }
    PathArc {
        x: root.corners.topLeft
        y: 0
        radiusX: root.corners.topLeft
        radiusY: root.corners.topLeft
    }
}
