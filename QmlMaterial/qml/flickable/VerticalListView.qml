pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.ListView {
    id: root
    contentWidth: width - rightMargin - leftMargin
    implicitHeight: expand ? contentHeight + topMargin + bottomMargin : 0

    footer: MD.ListBusyFooter {
        running: root.busy
        width: ListView.view.width
        height: implicitHeight
    }

    T.ScrollBar.vertical: MD.ScrollBar {}

    Connections {
        target: root
        function onHeaderItemChanged() {
            if (root.headerItem) {
                let old = 0;
                // keep top position
                const slot = function () {
                    const header = root.headerItem;
                    const v = root;
                    if (Math.abs(v.contentY + (old + v.topMargin)) < Number.EPSILON) {
                        v.contentY = -(v.topMargin + header.implicitHeight);
                    }
                    old = header.implicitHeight;
                };
                root.headerItem.implicitHeightChanged.connect(slot);
                slot();
            }
        }
    }
}
