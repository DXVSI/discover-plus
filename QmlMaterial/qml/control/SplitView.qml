pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.SplitView {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    property real handleOpacity: 1

    handle: MD.DragHandle {
        id: m_handle
        opacity: control.handleOpacity
        pressed: T.SplitHandle.pressed
        hovered: T.SplitHandle.hovered
        visualFocus: control.visualFocus
        orientation: control.orientation
        controlHeight: control.height
        controlWidth: control.width
    }
}
