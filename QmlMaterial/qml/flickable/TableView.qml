import QtQuick
import Qcm.Material as MD

TableView {
    readonly property int hoveredRow: m_hover.hovered ? lastHoveredRow : -1
    property int lastHoveredRow: -1
    signal cellHovered(int row, int column)
    synchronousDrag: true
    pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    rebound: Transition {}

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0

    onCellHovered: function (row, column) {
        lastHoveredRow = row;
    }
    selectionBehavior: TableView.SelectRows
    HoverHandler {
        id: m_hover
    }
}
