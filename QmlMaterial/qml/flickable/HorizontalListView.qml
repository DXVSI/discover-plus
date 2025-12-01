pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ListView {
    id: root
    contentHeight: height - topMargin - bottomMargin
    orientation: ListView.Horizontal
    implicitWidth: contentWidth + leftMargin + rightMargin
}
