import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TabBar {
    id: control

    property int type: MD.Enum.PrimaryTab
    property int radius: 0
    property MD.corners corners: MD.Util.corners(radius)
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    spacing: 1

    contentItem: ListView {
        id: m_view
        model: control.contentModel
        currentIndex: control.currentIndex

        contentHeight: height - topMargin - bottomMargin
        implicitWidth: contentWidth + leftMargin + rightMargin

        spacing: control.spacing
        orientation: ListView.Horizontal
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem

        highlightMoveDuration: 250
        highlightResizeDuration: 0
        highlightFollowsCurrentItem: true
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: 48
        preferredHighlightEnd: width - 48

        highlight: Item {
            z: 2
            Item {
                x: control.type == MD.Enum.PrimaryTab ? (parent.width - width) / 2 : 0
                y: control.position === T.TabBar.Footer ? 0 : parent.height - height
                height: control.type == MD.Enum.PrimaryTab ? 3 : 2
                width: control.type == MD.Enum.PrimaryTab ? (m_view.currentItem?.implicitContentWidth ?? 0) : parent.width
                clip: true

                Behavior on width {
                    NumberAnimation {
                        duration: 250
                    }
                }
                Rectangle {
                    height: parent.height * 2
                    width: parent.width
                    radius: control.type == MD.Enum.PrimaryTab ? 3 : 0
                    color: MD.Token.color.primary
                }
            }
        }

        ListView.onAdd: {
            const idx = count - 1;
            const item = itemAtIndex(idx);
            if (item instanceof MD.TabButton) {
                item.type = control.type;
            }
        }
    }

    background: MD.Rectangle {
        color: control.MD.MProp.backgroundColor
        corners: control.corners

        //layer.enabled: control.Material.elevation > 0
        //layer.effect: MD.ElevationEffect {
        //    elevation: control.Material.elevation
        //    fullWidth: true
        //}

        MD.Divider {
            anchors.bottom: parent.bottom
        }
    }

    layer.enabled: true
    //layer.sourceRect: Qt.rect(0, 0, control.width, control.height + 8)
    //layer.textureSize: Qt.size(control.width, control.height + 8)
    layer.effect: Item {
        property var source
        MD.RoundClip {
            source: parent.source
            width: parent.width
            height: parent.height
            corners: control.corners
            size: Qt.vector2d(control.width, control.height)
        }
    }
}
