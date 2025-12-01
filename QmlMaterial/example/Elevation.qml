import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

MD.Page {
    id: root
    title: 'Components'
    padding: 0

    MD.VerticalFlickable {
        anchors.fill: parent
        topMargin: 16
        bottomMargin: 16

        ColumnLayout {
            width: parent.width

            MD.Pane {
                Layout.fillWidth: parent.width <= implicitWidth
                Layout.alignment: Qt.AlignHCenter

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Elevation'
                        typescale: MD.Token.typescale.title_large
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Shadow'
                            typescale: MD.Token.typescale.title_medium
                        }
                        MD.Pane {
                            Layout.fillWidth: true
                            padding: 36
                            radius: MD.Token.shape.corner.medium

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 24

                                MD.Slider {
                                    id: m_slider_radius
                                    value: 8
                                    from: 0
                                    to: 80
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 36
                                    Elevation {
                                        elevation: MD.Token.elevation.level0
                                    }
                                    Elevation {
                                        elevation: MD.Token.elevation.level1
                                    }
                                    Elevation {
                                        elevation: MD.Token.elevation.level2
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 36
                                    Elevation {
                                        elevation: MD.Token.elevation.level3
                                    }
                                    Elevation {
                                        elevation: MD.Token.elevation.level4
                                    }
                                    Elevation {
                                        elevation: MD.Token.elevation.level5
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    component Elevation: MD.Elevation {
        radius: m_slider_radius.value
        Layout.fillWidth: true
        Layout.preferredWidth: 160
        Layout.preferredHeight: width
        Layout.maximumWidth: 160
        Layout.maximumHeight: 160
        implicitWidth: 40
        implicitHeight: width
        visible: true

        MD.Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: MD.MProp.color.surface_container
        }
    }
}
