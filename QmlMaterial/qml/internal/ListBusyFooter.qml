import QtQuick
import Qcm.Material as MD

Item {
    property alias running: m_busy.running

    visible: running
    implicitHeight: visible ? m_busy.implicitHeight + 2 * padding : 0
    property int padding: 4

    MD.CircularIndicator {
        id: m_busy

        anchors.centerIn: parent
    }
}
