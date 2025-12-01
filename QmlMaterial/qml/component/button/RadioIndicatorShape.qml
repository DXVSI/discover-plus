import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property color outlineColor
    property color color
    property real outlineWidth: 2
    property real focusWidth: 3
    property real radius: (Math.min(width, height) - root.outlineWidth) / 2
    property real fillFactor: 1.0
    readonly property real fillRadius: (root.radius - root.outlineWidth - 2) * root.fillFactor

    width: 20
    height: 20
    implicitHeight: 20
    implicitWidth: 20

    ShapePath {
        strokeColor: root.outlineColor
        strokeWidth: root.focusWidth
        fillColor: "transparent"
        PathAngleArc {
            centerX: root.width / 2
            centerY: root.height / 2
            radiusX: root.radius + (root.focusWidth + root.outlineWidth) / 2 + root.focusWidth
            radiusY: root.radius + (root.focusWidth + root.outlineWidth) / 2 + root.focusWidth
            startAngle: 0
            sweepAngle: 360
        }
    }

    ShapePath {
        strokeColor: root.outlineColor
        strokeWidth: root.outlineWidth
        fillColor: "transparent"
        PathAngleArc {
            centerX: root.width / 2
            centerY: root.height / 2
            radiusX: root.radius
            radiusY: root.radius
            startAngle: 0
            sweepAngle: 360
        }
    }

    ShapePath {
        strokeColor: "transparent"
        fillColor: root.color
        PathAngleArc {
            centerX: root.width / 2
            centerY: root.height / 2
            radiusX: root.fillRadius
            radiusY: root.fillRadius
            startAngle: 0
            sweepAngle: 360
        }
    }
}
