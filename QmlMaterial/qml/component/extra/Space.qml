import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

Item {
    id: root
    property int spacing: 0
    property bool fill: true

    Binding {
        root.Layout.fillWidth: root.fill
        root.implicitHeight: root.spacing
        root.height: root.spacing
        when: (root.parent instanceof ColumnLayout)
    }
    Binding {
        root.width: root.fill ? root.parent.width : root.implicitWidth
        root.implicitHeight: root.spacing
        root.height: root.spacing
        when: (root.parent instanceof Column)
    }
    Binding {
        root.Layout.fillHeight: root.fill
        root.implicitWidth: root.spacing
        root.width: root.spacing
        when: (root.parent instanceof RowLayout)
    }
    Binding {
        root.height: root.fill ? root.parent.height : root.implicitHeight
        root.implicitWidth: root.spacing
        root.width: root.spacing
        when: (root.parent instanceof Row)
    }
}
