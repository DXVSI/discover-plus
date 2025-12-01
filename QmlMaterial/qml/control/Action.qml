import QtQuick
import QtQuick.Templates as T

T.Action {
    default property list<QtObject> data
    property bool busy: false
    property bool closeMenu: true
    icon.width: 24
    icon.height: 24
}
