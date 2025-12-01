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
        Disjoint = 0,
        Contiguous
    }

    property int animationState: LinearIndicator.Stopped
    property int strokeWidth: 4
    property alias type: m_updator.indeterminateAnimationType
    running: false
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    onRunningChanged: {
        if (running) {
            m_updator.completeEndProgress = 0;
            animationState = LinearIndicator.Running;
        } else {
            if (animationState == LinearIndicator.Running)
                animationState = LinearIndicator.Completing;
        }
    }

    MD.LinearIndicatorUpdator {
        id: m_updator
        colors: {
            const m = control.MD.MProp.color;
            if (indeterminateAnimationType == MD.LinearIndicatorUpdator.DisJoint) {
                return [m.primary];
            } else {
                return [m.primary, m.primary, m.primary];
            }
        }
    }

    NumberAnimation {
        running: control.animationState != LinearIndicator.Stopped
        loops: Animation.Infinite
        target: m_updator
        property: 'progress'
        from: 0
        to: 1
        duration: m_updator.duration
    }

    NumberAnimation {
        id: m_complete_end_anim
        running: control.animationState == LinearIndicator.Completing
        target: m_updator
        property: 'progress'
        to: 1
        duration: m_updator.completeEndDuration
        onFinished: {
            if (control.animationState == LinearIndicator.Completing)
                control.animationState = LinearIndicator.Stopped;
        }
    }

    contentItem: Item {
        implicitHeight: m_shape.strokeWidth
        implicitWidth: 100
        MD.LinearIndicatorShape {
            id: m_shape
            anchors.fill: parent
            indicators: m_updator.activeIndicators
            strokeWidth: control.animationState != LinearIndicator.Stopped ? control.strokeWidth : 0

            Behavior on strokeWidth {
                NumberAnimation {
                    duration: MD.Token.duration.short2
                }
            }
        }
    }

    background: Item {}
}
