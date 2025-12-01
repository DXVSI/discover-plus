import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

T.BusyIndicator {
    id: control
    enum AnimStateType {
        Running,
        Completing,
        Stopped
    }
    enum AnimType {
        Advance = 0,
        Reteat
    }

    property int animationState: CircularIndicator.Stopped
    property real strokeWidth: 4
    property color color: MD.MProp.color.primary
    property alias type: m_updator.indeterminateAnimationType
    readonly property alias progress: m_updator.progress

    running: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: strokeWidth
    clip: false

    onRunningChanged: {
        if (running) {
            m_updator.completeEndProgress = 0;
            animationState = CircularIndicator.Running;
        } else {
            if (animationState == CircularIndicator.Running)
                animationState = CircularIndicator.Completing;
        }
    }

    MD.CircularIndicatorUpdator {
        id: m_updator
    }

    NumberAnimation {
        running: control.animationState != CircularIndicator.Stopped
        loops: Animation.Infinite
        target: m_updator
        property: 'progress'
        from: 0
        to: 1
        duration: m_updator.duration
    }

    NumberAnimation {
        id: m_complete_end_anim
        running: control.animationState == CircularIndicator.Completing
        target: m_updator
        property: 'completeEndProgress'
        from: 0
        to: 1
        duration: m_updator.completeEndDuration
        onFinished: {
            if (control.animationState == CircularIndicator.Completing)
                control.animationState = CircularIndicator.Stopped;
        }
    }

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32
        MD.CircleIndicatorShape {
            id: m_shape
            anchors.fill: parent
            strokeColor: control.color
            rotation: m_updator.rotation
            startAngle: m_updator.startFraction * 360
            endAngle: m_updator.endFraction * 360

            strokeWidth: control.strokeWidth
        }
    }
}
