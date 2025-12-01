import QtQuick
import QtQuick.Window
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: size
    implicitHeight: size

    property string name
    property int size: 24
    property int weight: Font.Normal
    property bool fill: false
    property alias horizontalAlignment: m_text_icon.horizontalAlignment
    property alias verticalAlignment: m_text_icon.verticalAlignment

    property int lineHeight: MD.Token.typescale.label_large.line_height
    property color color: MD.MProp.color.on_background

    property real _fill: fill ? 1 : 0
    Behavior on _fill {
        NumberAnimation {
            duration: 150
        }
    }

    readonly property var _fillVals: ([0, 0.25, 0.5, 0.75, 1])
    readonly property real _fillSeg: {
        let v = 0;
        if (fill) {
            v = _fillVals.find(el => _fill <= el);
        } else {
            v = MD.Util.findLast(_fillVals, el => _fill >= el);
        }
        return v;
    }

    Text {
        id: m_text_icon
        anchors.centerIn: parent
        renderType: Text.CurveRendering

        font.family: root.fill ? MD.Token.font.icon_fill_family : MD.Token.font.icon_family
        font.weight: root.weight
        font.variableAxes: {
            if (!MD.Token.font.is_vf_icon) {
                return {};
            }
            return {
                "FILL": root._fillSeg
            };
        }
        font.pixelSize: root.size / scale
        scale: 1.0 / MD.Token.cal_curve_scale(Screen.devicePixelRatio)

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: root.name
        color: root.color
        lineHeight: root.lineHeight
        lineHeightMode: Text.FixedHeight
    }
}
