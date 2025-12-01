import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 1.0
        to: 0.0
        duration: MD.Token.duration.medium2
        easing.type: Easing.InOutCubic
    }

    ScaleAnimator {
        from: 1.0
        to: 0.92
        duration: MD.Token.duration.medium2
        easing.type: Easing.InOutCubic
    }
}

