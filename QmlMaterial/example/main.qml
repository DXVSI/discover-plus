import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.ApplicationWindow {
    height: 600
    visible: true
    width: 900

    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.MProp.backgroundColor: MD.MProp.color.surface_container
    property int windowClass: MD.Token.window_class.select_type(width)
    MD.MProp.size.windowClass: windowClass

    onWidthChanged: {
        windowClass = MD.Token.window_class.select_type(width);
    }

    Loader {
        anchors.fill: parent
        source: 'qrc:/Qcm/Material/Example/Example.qml'
    }
}
