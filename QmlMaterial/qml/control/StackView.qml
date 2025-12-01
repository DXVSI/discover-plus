import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.StackView {
    id: control

    popEnter: MD.FadeInThroughMotion {}
    popExit: MD.FadeOutThroughMotion {}

    pushEnter: MD.FadeInThroughMotion {}
    pushExit: MD.FadeOutThroughMotion {}

    replaceEnter: MD.FadeInThroughMotion {}
    replaceExit: MD.FadeOutThroughMotion {}
}
