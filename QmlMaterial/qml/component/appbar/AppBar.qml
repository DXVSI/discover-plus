import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.ToolBar {
    id: control
    property int radius: 0
    property int type: MD.Enum.AppBarCenterAligned
    property bool showBackground: true
    property list<MD.Action> actions
    property alias leadingAction: m_leading.action
    property alias title: m_title.text
    property MD.StateAppBar mdState: MD.StateAppBar {
        item: control
    }
    Binding {
        control.mdState.type: control.type
        control.mdState.showBackground: control.showBackground
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    leftPadding: 16 - (m_leading.visible ? m_leading.leftInset + m_leading.leftPadding : 0)
    rightPadding: leftPadding

    contentItem: Item {
        implicitWidth: children[0].implicitWidth
        implicitHeight: children[0].implicitHeight

        MD.Text {
            id: m_title
            font.capitalization: Font.Capitalize
            typescale: control.mdState.typescale
            Binding {
                when: control.type === MD.Enum.AppBarCenterAligned
                restoreMode: Binding.RestoreNone
                m_text_row.implicitWidth: 48 * 2.0 + m_title.implicitWidth / 2.0
                m_title.x: m_bar_row.x + (m_bar_row.width - m_title.width) / 2.0
                m_title.y: m_bar_row.y + (m_bar_row.height - m_title.height) / 2.0
                m_title.horizontalAlignment: Text.AlignHCenter
                m_title.width: m_text_row.width
            }
            Binding {
                when: control.type === MD.Enum.AppBarSmall
                restoreMode: Binding.RestoreNone
                m_text_row.implicitWidth: m_title.implicitWidth
                m_title.x: m_bar_row.x + m_text_row.x
                m_title.y: m_bar_row.y + (m_bar_row.height - m_title.height) / 2.0
                m_title.horizontalAlignment: Text.AlignLeft
                m_title.width: m_text_row.width
            }
            Binding {
                when: control.type >= MD.Enum.AppBarMedium
                restoreMode: Binding.RestoreNone
                m_text_row.implicitWidth: 0
                m_title.x: 16 - control.leftPadding
                m_title.y: m_title.parent.height - m_title.height - (control.type === MD.Enum.AppBarMedium ? 24 : 28)
                m_title.horizontalAlignment: Text.AlignLeft
                m_title.width: m_title.parent.width - 16 * 2
            }
        }

        RowLayout {
            id: m_bar_row
            y: (64 - height) / 2.0
            width: parent.width

            MD.IconButton {
                id: m_leading
                visible: action
            }

            Item {
                id: m_text_row
                Layout.fillWidth: true
            }

            MD.ActionToolBar {
                Layout.preferredWidth: 48 * 2
                Layout.fillWidth: true
                Layout.maximumWidth: maximumContentWidth + 2
                actions: control.actions
            }
        }
    }

    background: MD.Rectangle {
        //elevation: control.mdState.elevation
        //elevationItem.height: height - control.radius
        //elevationItem.y: control.radius
        //elevationItem.corners: MD.Util.corners(0)

        implicitHeight: control.mdState.containerHeight
        corners: MD.Util.corners(control.radius, 0)
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity
    }

}
