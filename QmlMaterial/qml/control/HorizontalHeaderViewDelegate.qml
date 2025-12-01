import QtQuick
import Qcm.Material as MD

MD.ItemDelegate {
    id: control
    padding: 8

    required property var model
    readonly property TableView headerView: TableView.view
    property bool selected: false

    highlighted: selected

    background: Rectangle {
        border.color: "#e4e4e4"
        color: "#f6f6f6"
    }

    contentItem: Label {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#ff26282a"
        text: control.model[control.headerView.textRole]
    }
}
