import QtQuick
import Qcm.Material as MD

GridView {
    id: root

    clip: true

    synchronousDrag: true
    reuseItems: true
    pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    rebound: Transition {}
}
