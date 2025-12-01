import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

Rectangle {
    id: cursor

    color: MD.Token.color.primary
    width: 2
    visible: {
        let ok = parent.activeFocus;
        if (parent instanceof T.TextField) {
            ok &= !(parent as T.TextField).readOnly;
            ok &= (parent as T.TextField).selectionStart === (parent as T.TextField).selectionEnd;
        }
        return ok;
    }

    Connections {
        target: cursor.parent
        function onCursorPositionChanged() {
            // keep a moving cursor visible
            cursor.opacity = 1;
            timer.restart();
        }
    }

    Timer {
        id: timer
        running: {
            let ok = cursor.parent.activeFocus && interval != 0;
            if (parent instanceof T.TextField) {
                ok &= !(parent as T.TextField).readOnly;
            }
            return ok;
        }
        repeat: true
        interval: Qt.styleHints.cursorFlashTime / 2
        onTriggered: cursor.opacity = !cursor.opacity ? 1 : 0
        // force the cursor visible when gaining focus
        onRunningChanged: cursor.opacity = 1
    }
}
