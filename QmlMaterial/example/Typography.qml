import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.Page {
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

            Repeater {
                model: [
                    'display_large',
                    'display_medium',
                    'display_small',
                    'headline_large',
                    'headline_medium',
                    'headline_small',
                    'title_large',
                    'title_medium',
                    'title_small',
                    'label_large',
                    'label_medium',
                    'label_small',
                    'body_large',
                    'body_medium',
                    'body_small',
                ]
                MD.Text {
                    required property string modelData
                    typescale: MD.Token.typescale[modelData]
                    text: modelData.replace('_', ' ')
                    font.capitalization: Font.Capitalize
                }
            }
        }
    }
}
