import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

T.DialogButtonBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    spacing: 8
    horizontalPadding: 24
    alignment: Qt.AlignRight
    buttonLayout: T.DialogButtonBox.AndroidLayout

    delegate: MD.Button {
        type: MD.Enum.BtText
    }

    contentItem: ListView {
        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Item {}
}
