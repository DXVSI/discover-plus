import QtQuick
import Qcm.Material as MD

Item {
    id: root
    property int radius: 0
    property alias inner: m_image
    // tl,tr,bl,br
    property MD.corners corners: MD.Util.corners(radius)
    property alias asynchronous: m_image.asynchronous
    property alias retainWhileLoading: m_image.retainWhileLoading
    property alias autoTransform: m_image.autoTransform
    property alias cache: m_image.cache
    property alias currentFrame: m_image.currentFrame
    property alias fillMode: m_image.fillMode
    property alias frameCount: m_image.frameCount
    property alias horizontalAlignment: m_image.horizontalAlignment
    property alias mipmap: m_image.mipmap
    property alias mirror: m_image.mirror
    property alias mirrorVertically: m_image.mirrorVertically
    property alias progress: m_image.progress
    property alias source: m_image.source
    property alias sourceClipRect: m_image.sourceClipRect
    property alias sourceSize: m_image.sourceSize
    property alias status: m_image.status
    property alias verticalAlignment: m_image.verticalAlignment

    property alias paintedHeight: m_image.paintedHeight
    property alias paintedWidth: m_image.paintedWidth

    implicitHeight: {
        return sourceSize.height;
    }
    implicitWidth: {
        return sourceSize.width;
    }

    property int elevation: MD.Token.elevation.level0

    MD.Elevation {
        anchors.fill: parent
        elevation: root.elevation
        corners: root.corners
        opacity: root.status === Image.Ready && root.paintedHeight > 0 ? 1 : 0

    }

    Image {
        id: m_image
        anchors.fill: parent
        retainWhileLoading: true
        cache: true
        smooth: true
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        layer.effect: MD.RoundClip {
            corners: root.corners
            size: Qt.vector2d(root.width, root.height)
        }
    }
}
