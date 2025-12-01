import QtQuick
import Qcm.Material as MD

MD.MStateImpl {
    id: root

    Binding {
        target: root
        property: "target"
        when: root?.item ?? false
        value: root.item
    }

    Binding {
        target: root
        property: "enabled"
        when: root.target
        value: root.target?.mdState === root
    }
}