pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import QtQml.Models
import Qcm.Material as MD

MD.Shape {
    id: root
    property int strokeWidth: 4

    property real x1: 0
    property real x2: 0

    property color trackColor: root.MD.MProp.color.secondary_container

    property int count
    property list<MD.LinearActiveIndicatorData> indicators

    function drawLine(startFraction: real, endFraction: real, gapSize: real): point {
        startFraction = MD.Util.clamp(startFraction, 0, 1);
        endFraction = MD.Util.clamp(endFraction, 0, 1);
        // gap ramp down
        const GAP_RAMP_DOWN_THRESHOLD = 0.01;
        gapSize += strokeWidth / 2;
        const startGapSize = (gapSize * MD.Util.clamp(startFraction, 0, GAP_RAMP_DOWN_THRESHOLD) / GAP_RAMP_DOWN_THRESHOLD);
        const endGapSize = (gapSize * (1 - MD.Util.clamp(endFraction, 1 - GAP_RAMP_DOWN_THRESHOLD, 1)) / GAP_RAMP_DOWN_THRESHOLD);
        const start = width * startFraction + startGapSize;
        const end = width * endFraction - endGapSize;
        if (start >= end) {
            return Qt.point(0, 0);
        }
        return Qt.point(start, end);
    }

    component LinePath: ShapePath {
        id: m_p
        property point line
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        strokeWidth: root.strokeWidth
        startX: line.x
        startY: root.height / 2
        PathLine {
            x: m_p.line.y
            y: root.height / 2
        }
    }

    LinePath {
        line: {
            if (root.indicators.length < 1)
                return Qt.point(0, 0);
            const a = root.indicators[0];
            const start = 0;
            const end = a.startFraction;
            const gap = a.gapSize / 2;
            return root.drawLine(start, end, gap);
        }
        strokeColor: root.trackColor
    }

    LinePath {
        line: {
            if (root.indicators.length < 1)
                return Qt.point(0, 0);
            const a = root.indicators[root.indicators.length - 1];
            const start = a.endFraction;
            const end = 1;
            const gap = a.gapSize / 2;
            return root.drawLine(start, end, gap);
        }
        strokeColor: root.trackColor
    }

    Instantiator {
        model: Math.max(root.indicators.length, 1) - 1
        delegate: LinePath {
            required property int index
            property MD.LinearActiveIndicatorData cur: root.indicators[index]
            property MD.LinearActiveIndicatorData next: root.indicators[index + 1 % root.indicators.length]
            line: root.drawLine(cur.endFraction, next.startFraction, cur.gapSize / 2)
            strokeColor: root.trackColor
        }
        onObjectAdded: (idx, obj) => root.data.push(obj)
        onObjectRemoved: (idx, obj) => MD.Util.removeObj(root.data, obj)
    }

    Instantiator {
        model: root.indicators
        delegate: LinePath {
            required property MD.LinearActiveIndicatorData modelData
            line: root.drawLine(modelData.startFraction, modelData.endFraction, modelData.gapSize / 2)
            strokeColor: modelData.color
        }
        onObjectAdded: (idx, obj) => root.data.push(obj)
        onObjectRemoved: (idx, obj) => MD.Util.removeObj(root.data, obj)
    }
}
