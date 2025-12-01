import QtQuick
import QtQuick.Shapes

ShapePath {
    id: root
    strokeWidth: 2
    strokeColor: "black"
    fillColor: "transparent"
    fillRule: ShapePath.WindingFill

    property int width
    property int height

    startX: 0
    startY: 0

    PathLine {
        x: root.width
        y: 0
    }
    PathLine {
        x: root.width
        y: root.height
    }
    PathLine {
        x: 0
        y: root.height
    }
    PathLine {
        x: 0
        y: 0
    }
}
