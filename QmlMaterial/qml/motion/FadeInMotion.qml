import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 0.0
        to: 1.0
        duration: MD.Token.duration.medium4 * 0.3
        easing: MD.Token.easing.emphasized_decelerate
    }

    ScaleAnimator {
        from: 0.8
        to: 1.0
        duration: MD.Token.duration.medium4
        easing: MD.Token.easing.emphasized_decelerate
    }
}
