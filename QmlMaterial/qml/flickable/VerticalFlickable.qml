import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Flickable {
    id: root

    // flickable not set contentHeight
    contentHeight: contentItem.childrenRect.height
    contentWidth: width - rightMargin - leftMargin
    implicitHeight: contentHeight + topMargin + bottomMargin

    T.ScrollBar.vertical: MD.ScrollBar {}
}
