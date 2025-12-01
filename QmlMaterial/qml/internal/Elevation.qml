import QtQuick
import Qcm.Material as MD

MD.ElevationImpl {
    id: root
    color: MD.MProp.color.shadow
    visible: !MD.Util.epsilonEqual(elevation, MD.Token.elevation.level0)

    Behavior on elevation {
        NumberAnimation {
            duration: MD.Token.duration.medium1
            easing: MD.Token.easing.emphasized_accelerate
        }
    }
}
