import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Qcm.Material as MD

T.Control {
    id: root

    readonly property alias actions: m_layout.actions
    property int display: T.Button.IconOnly
    property alias alignment: m_layout.alignment
    readonly property alias maximumContentWidth: m_layout.implicitWidth

    property string overflowIconName: "overflow-menu"

    /**
     * @brief This property holds the combined width of all visible delegates.
     * @property int visibleWidth
     */
    readonly property alias visibleWidth: m_layout.visibleWidth

    /**
     * @brief This property sets the handling method for items that do not match the toolbar's height.
     *
     * When toolbar items do not match the height of the toolbar, there are
     * several ways we can deal with this. This property sets the preferred way.
     *
     * Permitted values are:
     * * ``HeightMode.AlwaysCenter``
     * * ``HeightMode.AlwaysFill``
     * * ``AlwaysFill.ConstrainIfLarger``
     *
     * default: ``HeightMode::ConstrainIfLarger``
     *
     * @see ToolBarLayout::heightMode
     * @see ToolBarLayout::HeightMode
     * @property ToolBarLayout::HeightMode heightMode
     */
    property alias heightMode: m_layout.heightMode

    implicitHeight: m_layout.implicitHeight
    implicitWidth: m_layout.implicitWidth

    Binding {
        when: root.parent instanceof RowLayout
        root.Layout.minimumWidth: m_layout.minimumWidth
    }

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    contentItem: MD.ToolBarLayout {
        id: m_layout
        maxShowActionNum: 2

        separatorDelegate: MD.ToolSeparator {}

        fullDelegate: MD.IconButton {
            action: MD.ToolBarLayout.action
        }

        iconDelegate: MD.IconButton {
            action: MD.ToolBarLayout.action
        }

        moreButton: MD.IconButton {
            icon.name: MD.Token.icon.more_vert
            property var oldPopup: null
            onClicked: {
                if (!oldPopup || !oldPopup.visible) {
                    oldPopup = MD.Util.showPopup(comp_menu, {}, this);
                    oldPopup.y = this.height;
                }
            }

            Component {
                id: comp_menu
                MD.ActionMenu {
                    actions: m_layout.hiddenActions
                }
            }
        }
    }
}
