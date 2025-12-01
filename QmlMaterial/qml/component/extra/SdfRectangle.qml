import QtQuick
import Qcm.Material as MD

Item {
    id: root
    property int radius 
    property MD.corners corners: MD.Util.corners(radius)
    property alias color: inner.color

    ShaderEffect {
        id: inner
        anchors.centerIn: parent
        width: root.width
        height: root.height

        property vector4d radius: root.corners.toVector4D()
        property vector4d size: Qt.vector4d(width, height, root.width - (radius.length() ? 1 : 1), root.height - (radius.length() ? 1 : 1)) // -2 for better between round and straight
        property color color: "#00000000"
        property real smoothing: 1.0
        property real offset: 0.0
        vertexShader: 'qrc:/Qcm/Material/assets/shader/default.vert.qsb'
        fragmentShader: 'qrc:/Qcm/Material/assets/shader/sdf_rectangle.frag.qsb'
    }
}
