pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Templates as T
import Qcm.Material as MD

T.ComboBox {
    id: control

    property int type: MD.Enum.TextFieldOutlined
    property MD.StateComboBox mdState: MD.StateComboBox {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 12
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    topPadding: 0
    bottomPadding: 0

    delegate: MD.MenuItem {
        required property var model
        required property int index

        width: ListView.view.width
        text: model[control.textRole]
        selected: control.currentIndex == index
        highlighted: control.highlightedIndex == index
    }

    indicator: MD.Icon {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        name: MD.Token.icon.arrow_drop_down
        size: 24
    }

    contentItem: MD.TextInput {
        typescale: MD.Token.typescale.body_large

        padding: 0
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: !control.editable
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        color: control.mdState.textColor
        selectionColor: control.mdState.ctx.color.primary
        selectedTextColor: control.mdState.ctx.color.getOn(selectionColor)
        verticalAlignment: TextInput.AlignVCenter
    }

    background: Item {
        implicitWidth: 64
        implicitHeight: 48
        MD.OutlineTextFieldShape {
            anchors.fill: parent
            borderColor: control.mdState.outlineColor
            radius: MD.Token.shape.corner.extra_small
        }
    }

    popup: MD.Menu {
        y: control.editable ? control.height - 5 : 0
        height: implicitHeight
        width: control.width
        transformOrigin: Item.Top
        modal: false
        focus: false
        model: control.delegateModel
        topMargin: 12
        bottomMargin: 12
        verticalPadding: 8
        currentIndex: control.currentIndex
    }
}
