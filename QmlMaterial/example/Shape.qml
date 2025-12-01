import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.Page {

    MD.VerticalFlickable {
        anchors.fill: parent
        topMargin: 16
        bottomMargin: 16

        ColumnLayout {
            width: parent.width

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter

                ColumnLayout {
                    spacing: 16

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Shape scale'
                            typescale: MD.Token.typescale.title_medium
                        }
                        MD.Pane {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 400
                            padding: 48
                            radius: MD.Token.shape.corner.medium
                            backgroundColor: MD.MProp.color.surface_container

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 24

                                Repeater {
                                    model: [MD.Token.shape.corner.none, MD.Token.shape.corner.extra_small, MD.Token.shape.corner.small, MD.Token.shape.corner.medium, MD.Token.shape.corner.large, MD.Token.shape.corner.extra_large, MD.Token.shape.corner.full]
                                    MD.Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        color: MD.MProp.color.primary
                                        implicitWidth: 200
                                        implicitHeight: 100
                                        radius: modelData === MD.Token.shape.corner.full ? height / 2 : modelData
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Asymmetric'
                            typescale: MD.Token.typescale.title_medium
                        }
                        MD.Pane {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 400
                            padding: 48
                            radius: MD.Token.shape.corner.medium
                            backgroundColor: MD.MProp.color.surface_container

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 24

                                Repeater {
                                    model: [
                                        MD.Util.corners(MD.Token.shape.corner.extra_small, 0),
                                        MD.Util.corners(MD.Token.shape.corner.large, 0),
                                        MD.Util.corners(MD.Token.shape.corner.extra_large, 0),
                                        MD.Util.corners(0 ,MD.Token.shape.corner.extra_large, 0, MD.Token.shape.corner.extra_large),
                                    ]
                                    MD.Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        color: MD.MProp.color.primary
                                        implicitWidth: 200
                                        implicitHeight: 100
                                        corners: modelData
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
