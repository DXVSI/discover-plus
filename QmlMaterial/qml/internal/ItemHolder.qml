import QtQuick

Item {
    id: root
    property Item item: null

    implicitHeight: item ? item.implicitHeight : 0
    implicitWidth: item ? item.implicitWidth : 0
    data: [item]

    Binding {
        when: root.item
        target: root.item
        property: 'width'
        restoreMode: Binding.RestoreNone
        value: root.width
    }

    Binding {
        when: root.item
        target: root.item
        property: 'height'
        restoreMode: Binding.RestoreNone
        value: root.height
    }
}
