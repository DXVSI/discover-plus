import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Page {
    id: control

    property bool canBack: false
    property MD.Action leadingAction: MD.MProp.page.leadingAction
    property list<MD.Action> actions

    property alias showHeader: control.header.visible
    property alias showBackground: control.background.visible

    property int elevation: MD.Token.elevation.level0
    property color backgroundColor: MD.MProp.color.background
    property int radius: MD.MProp.page.radius
    property int backgroundRadius: MD.MProp.page.backgroundRadius
    property bool scrolling: false

    header: MD.AppBar {
        title: control.title
        leadingAction: control.leadingAction
        type: control.MD.MProp.page.headerType
        visible: control.MD.MProp.page.showHeader
        radius: control.backgroundRadius
        showBackground: control.scrolling
        actions: control.actions
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    // bottomPadding: header.visible ? radius : 0
    font.capitalization: Font.Capitalize

    background: MD.Rectangle {
        color: control.backgroundColor
        radius: control.backgroundRadius
        visible: control.MD.MProp.page.showBackground
    }
}
