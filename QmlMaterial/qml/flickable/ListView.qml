import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    clip: true
    property bool busy: false
    property bool expand: false

    reuseItems: true
    synchronousDrag: true
    pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds

    cacheBuffer: 96

    rebound: Transition {}
    populate: Transition {}

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0
}
