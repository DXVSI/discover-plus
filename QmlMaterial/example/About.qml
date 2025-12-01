import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

MD.Page {
    id: root
    title: 'About'
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        MD.Pane {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.preferredWidth: 400
            backgroundColor: MD.MProp.color.surface_container
            radius: MD.Token.shape.corner.medium
            padding: 12

            ColumnLayout {
                anchors.fill: parent

                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    MD.Label {
                        text: 'QmlMaterial'
                        typescale: MD.Token.typescale.title_large
                    }
                    MD.Label {
                        text: MD.Token.version
                        typescale: MD.Token.typescale.label_large
                        opacity: 0.8
                    }

                    RowLayout {
                        MD.SuggestionChip {
                            text: 'repo'
                            onClicked: {
                                Qt.openUrlExternally('https://github.com/hypengw/QmlMaterial');
                            }
                        }
                        MD.SuggestionChip {
                            text: 'issue'
                            onClicked: {
                                Qt.openUrlExternally('https://github.com/hypengw/QmlMaterial/issues');
                            }
                        }
                        MD.SuggestionChip {
                            text: 'donate'
                            onClicked: {
                                Qt.openUrlExternally('https://ko-fi.com/hypengw');
                            }
                        }
                    }
                }
            }
        }
    }
}
