import QtQuick
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: m_row.implicitWidth
    implicitHeight: m_row.implicitHeight

    property alias spacing: m_row.spacing

    property alias text: m_label.text
    property alias color: m_label.color
    property alias label: m_label

    property alias icon: m_icon

    property alias icon_component_active: m_loader.active
    property alias icon_component: m_loader.sourceComponent

    property int style: MD.Enum.IconAndText

    property int horizontalAlignment: Qt.AlignHCenter

    Row {
        id: m_row
        Binding {
            when: root.horizontalAlignment == Qt.AlignLeft
            m_row.anchors.left: root.left
        }
        Binding {
            when: root.horizontalAlignment == Qt.AlignRight
            m_row.anchors.right: root.right
        }
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: root.horizontalAlignment == Qt.AlignHCenter ? parent.horizontalCenter : undefined
        spacing: 8

        Item {
            anchors.verticalCenter: parent.verticalCenter
            implicitHeight: Math.max(m_icon.implicitHeight, m_loader.implicitHeight)
            implicitWidth: Math.max(m_icon.implicitWidth, m_loader.implicitWidth)
            visible: root.style != MD.Enum.TextOnly && (m_icon.name.length > 0 || m_loader.active)
            MD.Icon {
                id: m_icon
                anchors.verticalCenter: parent.verticalCenter
                visible: name.length > 0 && !m_loader.active
                color: root.color
            }

            MD.Loader {
                id: m_loader
                anchors.verticalCenter: parent.verticalCenter
                active: false
                visible: active
            }
        }

        MD.Label {
            id: m_label
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: Text.AlignVCenter
            visible: root.style != MD.Enum.IconOnly
            color: root.color
            useTypescale: false
        }
    }
}
