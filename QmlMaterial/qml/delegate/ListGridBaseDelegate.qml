import QtQuick
import Qcm.Material as MD

Item {
    id: root
    required property var model
    required property int index

    property MD.WidthProvider widthProvider
    readonly property int column: (index % widthProvider.column)
    property int cellHeight: 0

    implicitHeight: column == 0 ? cellHeight : 0
    width: ListView.view.contentWidth

    signal clicked

    property int cellX: {
        // bind width
        const w = widthProvider.width;
        return widthProvider.calculateX(column) + Math.min(0, w);
    }
    property int cellY: column != 0 ? -cellHeight : 0
}
