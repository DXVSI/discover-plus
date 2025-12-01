import QtQuick
import Qcm.Material as MD

MD.Label {
    id: root
    typescale: MD.Token.typescale.body_medium
    verticalAlignment: Qt.AlignVCenter
    color: m_area.containsMouse ? MD.MProp.color.primary : MD.MProp.color.on_background
    text: action ? action.text : ""

    property MD.Action action: null

    Component.onCompleted: {
        if (action) {
            data.push(action);
        }
    }

    MouseArea {
        id: m_area
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: {
            if (root.action) {
                root.action.trigger();
            }
        }
    }
}
