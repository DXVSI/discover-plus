import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 0.0
        to: 1.0
        duration: MD.Token.duration.medium2
        easing.type: Easing.InOutCubic
    }

    ScaleAnimator {
        from: 0.92
        to: 1.0
        duration: MD.Token.duration.medium2
        easing.type: Easing.InOutCubic
    }
}