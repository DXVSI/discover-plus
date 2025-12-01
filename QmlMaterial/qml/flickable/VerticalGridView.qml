import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.GridView {
    id: root

    T.ScrollBar.vertical: MD.ScrollBar {}

    signal calMaxCellHeight
    property bool enabledCalMaxCellHeight: false
    property int maxImplicitCellHeight: 0
    property int maxCellHeight: 0
    Timer {
        id: m_cal_timer
        interval: 200
        repeat: false
        running: false
        onTriggered: {
            let implict = -1;
            let max = -1;
            for (let i = 0; i < root.count; i++) {
                const item = root.itemAtIndex(i);
                if (item && item.visible) {
                    implict = Math.max(implict, item.implicitHeight);
                    max = Math.max(max, item.height);
                }
            }
            if (implict != -1) {
                root.maxImplicitCellHeight = implict + root.topMargin + root.bottomMargin;
                root.maxCellHeight = max + root.topMargin + root.bottomMargin;
            }
        }
    }
    Connections {
        target: root
        enabled: root.enabledCalMaxCellHeight
        property int count: 0
        function onCalMaxCellHeight() {
            if (count++ >= 10) {
                count = 0;
                m_cal_timer.stop();
                m_cal_timer.triggered();
            } else {
                m_cal_timer.restart();
            }
        }
        function onWidthChanged() {
            root.calMaxCellHeight();
        }
    }
}
