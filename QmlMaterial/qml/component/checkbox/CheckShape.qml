import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    implicitHeight: 18
    implicitWidth: 18
    width: 18
    height: 18
    property int checkState: Qt.Checked
    property color color
    property color backgroundColor
    property color outlineColor

    transform: [
        Translate {
            x: -7 + (root.width - 18) / 2
            y: -7 + (root.height - 18) / 2
        }
    ]

    ShapePath {
        objectName: "svg_path:background-checked"
        strokeColor: "transparent"
        fillRule: ShapePath.WindingFill
        fillColor: root.checkState == Qt.Checked ? root.backgroundColor : "transparent"
        pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
        PathSvg {
            path: "M 23 7 L 9 7 Q 8.175 7 7.5875 7.5875 Q 7 8.175 7 9 L 7 23 Q 7 23.825 7.5875 24.4125 Q 8.175 25 9 25 L 23 25 Q 23.825 25 24.4125 24.4125 Q 25 23.825 25 23 L 25 9 Q 25 8.175 24.4125 7.5875 Q 23.825 7 23 7 "
        }
    }
    ShapePath {
        objectName: "svg_path:background-unchecked"
        strokeColor: "transparent"
        fillRule: ShapePath.WindingFill
        fillColor: root.checkState != Qt.Checked ? root.outlineColor : "transparent"
        pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
        PathSvg {
            path: "M 23 7 L 9 7 Q 8.175 7 7.5875 7.5875 Q 7 8.175 7 9 L 7 23 Q 7 23.825 7.5875 24.4125 Q 8.175 25 9 25 L 23 25 Q 23.825 25 24.4125 24.4125 Q 25 23.825 25 23 L 25 9 Q 25 8.175 24.4125 7.5875 Q 23.825 7 23 7 M 23 23 L 9 23 L 9 9 L 23 9 L 23 23 "
        }
    }
    ShapePath {
        objectName: "svg_path:checked"
        strokeColor: "transparent"
        fillRule: ShapePath.WindingFill
        fillColor: root.checkState == Qt.Checked ? root.color : "transparent"
        pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
        PathSvg {
            path: "M 14 18.2 L 11.4 15.6 L 10 17 L 14 21 L 22 13 L 20.6 11.6 L 14 18.2 "
        }
    }
    ShapePath {
        objectName: "svg_path:partially_checked"
        strokeColor: "transparent"
        fillRule: ShapePath.WindingFill
        fillColor: root.checkState == Qt.PartiallyChecked ? root.color : "transparent"
        pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
        PathSvg {
            path: "M 13.4 15 L 11 15 L 11 17 L 13.4 17 L 21 17 L 21 15 L 13.4 15 "
        }
    }
}
