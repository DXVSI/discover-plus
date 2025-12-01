import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property color strokeColor: MD.MProp.color.primary
    property real strokeWidth: 4
    readonly property int radius: height / 2
    readonly property vector2d center: Qt.vector2d(radius, radius)
    property real startAngle: 0
    property real endAngle: 0
    asynchronous: false

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: root.radius
        startY: 0
        strokeColor: root.strokeColor
        strokeWidth: root.strokeWidth

        PathAngleArc {
            centerX: root.center.x
            centerY: root.center.y
            radiusX: root.radius
            radiusY: root.radius
            startAngle: root.startAngle
            sweepAngle: root.endAngle - root.startAngle
        }
    }
}
