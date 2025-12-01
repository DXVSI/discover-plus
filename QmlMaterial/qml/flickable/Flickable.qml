import QtQuick

import Qcm.Material as MD

Flickable {
    id: root

    // it's hard to avoid item around Flickable is not transparent
    clip: true

    synchronousDrag: true
    pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    rebound: Transition {}

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0
}
