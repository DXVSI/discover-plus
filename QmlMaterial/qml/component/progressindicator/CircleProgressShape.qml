import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property int strokeWidth: 4
    readonly property real radius: height / 2
    readonly property real spacingAngle: (8 / (Math.max(radius, 1) * (2 * Math.PI))) * 360
    readonly property vector2d center: Qt.vector2d(radius, radius)
    property real progress: 0
    asynchronous: false

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: root.radius
        startY: 0
        strokeColor: MD.Util.transparent(root.MD.MProp.color.primary, 0.3)
        strokeWidth: root.strokeWidth

        PathAngleArc {
            centerX: root.center.x
            centerY: root.center.x
            radiusX: 16
            radiusY: radiusX
            startAngle: -90 - root.spacingAngle * 0.5
            sweepAngle: {
                const p = m_active_arc.sweepAngle;
                let out = 0;
                if (isNaN(p) || MD.Util.epsilonEqual(p, 0)) {
                    out = -360 + root.spacingAngle;
                } else {
                    out = -360 + 2 * root.spacingAngle + p;
                }
                return Math.min(1, out);
            }
        }
    }

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: root.radius
        startY: 0
        strokeColor: root.MD.MProp.color.primary
        strokeWidth: root.strokeWidth

        PathAngleArc {
            id: m_active_arc
            centerX: root.center.x
            centerY: root.center.y
            radiusX: 16
            radiusY: radiusX
            startAngle: -90 + root.spacingAngle * 0.5
            sweepAngle: (360 + root.spacingAngle) * root.progress
        }
    }
}
