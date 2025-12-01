import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.Page {
    component ColorGroup: Item {
        id: tt
        Layout.fillWidth: true
        property alias model: rep.model
        implicitHeight: children[0].implicitHeight
        clip: false

        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            Repeater {
                id: rep
                Rectangle {
                    required property string modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    color: {
                        return MD.Token.color[modelData]
                    }

                    MD.Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.modelData
                        color: MD.Token.color.getOn(MD.Token.color[parent.modelData])
                    }
                }
            }
        }

        layer.enabled: true
        layer.effect: MD.RoundClip {
            radius: 12
            size: Qt.vector2d(tt.width, tt.height)
        }
    }

    contentItem: MD.VerticalFlickable {
        id: flick
        topMargin: 8
        bottomMargin: 8

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 16

            ColorGroup {
                model: ['primary', 'on_primary', 'primary_container', 'on_primary_container']
            }
            ColorGroup {
                model: ['secondary', 'on_secondary', 'secondary_container', 'on_secondary_container']
            }
            ColorGroup {
                model: ['tertiary', 'on_tertiary', 'tertiary_container', 'on_tertiary_container']
            }
            ColorGroup {
                model: ['error', 'on_error', 'error_container', 'on_error_container']
            }
            ColorGroup {
                model: ['surface_dim', 'surface', 'surface_bright', 'surface_container_lowest', 'surface_container_low', 'surface_container', 'surface_container_high', 'surface_container_highest', 'on_surface', 'on_surface_variant' ]
            }
            ColorGroup {
                model: ['outline', 'shadow', 'inverse_surface', 'inverse_on_surface', 'inverse_primary']
            }
            ColorGroup {
                model: ['neutral_10', 'neutral_20', 'neutral_30', 'neutral_40', 'neutral_50',
                        'neutral_60', 'neutral_70', 'neutral_80', 'neutral_90', ]
            }
        }
    }
}
