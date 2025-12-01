import QtQuick
import Qcm.Material as MD

MD.Shape {
    id: root
    property real radius: 0
    property alias borderWidth: m_p.strokeWidth
    property alias borderColor: m_p.strokeColor
    property real floatX: 0
    property real floatWidth: 0

    property real openX: floatX + floatWidth / 2
    property real openWidth: 0
    property bool open: false

    state: open ? 'open' : 'close'
    states: [
        State {
            name: 'open'
            PropertyChanges {
                root.openX: root.floatX
                root.openWidth: root.floatWidth
            }
        }
    ]
    transitions: [
        Transition {
            NumberAnimation {
                properties: "openX,openWidth"
                duration: MD.Token.duration.medium3
            }
        }
    ]

    MD.RoundPath {
        id: m_p
        strokeWidth: 1
        strokeColor: "black"
        fillColor: "transparent"
        width: root.width
        height: root.height
        corners: MD.Util.corners(root.radius)
        startX: Math.max(m_p.corners.topLeft, root.openX) + root.openWidth

        // last is top left
        PathLine {
            x: Math.max(m_p.corners.topLeft, root.openX)
            y: 0
        }
    }
}
