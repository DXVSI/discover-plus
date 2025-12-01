pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.TextFieldEmbed {
    id: control

    property int type: MD.Enum.TextFieldOutlined
    property string leadingIcon
    property string trailingIcon
    property MD.StateTextField mdState: MD.StateTextField {
        item: control
    }
    Binding {
        control.mdState.type: control.type
    }

    font.capitalization: Font.MixedCase
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, Math.max(contentWidth, m_placeholder.implicitWidth) + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + bottomPadding + (control.type === MD.Enum.TextFieldFilled ? 0 : topPadding))

    // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
    topInset: clip ? m_placeholder.largestHeight / 2 : 0
    bottomInset: 0

    leftPadding: leading.visible ? 16 + 12 + leading.implicitWidth : 16
    rightPadding: trailing.visible ? 16 + 12 + trailing.implicitWidth : 16
    bottomPadding: 8
    topPadding: {
        if (control.type === MD.Enum.TextFieldFilled) {
            const ch = cursorRectangle.height;
            const ph = m_placeholder.implicitHeight;
            return ph + 8;
        } else {
            return 8;
        }
    }

    MD.FloatingPlaceholderText {
        id: m_placeholder
        x: control.leftPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        text: control.placeholderText
        font: control.font
        color: control.mdState.placeholderColor
        elide: Text.ElideRight
        renderType: control.renderType

        controlFocus: control.activeFocus
        controlHeight: control.height
        verticalPadding: 8

        filled: control.type === MD.Enum.TextFieldFilled
        controlHasText: control.length > 0
        //controlImplicitBackgroundHeight: control.implicitBackgroundHeight
    }

    property Item leading: MD.Icon {
        anchors.left: parent?.left
        anchors.verticalCenter: parent?.verticalCenter
        anchors.leftMargin: 12
        name: control.leadingIcon
        visible: name
        size: 24
    }

    property Item trailing: MD.Icon {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 12
        visible: name
        name: control.trailingIcon
        size: 24
    }

    Item {
        anchors.fill: parent
        data: [m_placeholder, control.leading, control.trailing]
    }

    background: Item {
        implicitWidth: 64
        implicitHeight: 56

        MD.Loader {
            anchors.fill: parent
            sourceComponent: control.type == MD.Enum.TextFieldFilled ? m_filled_comp : m_outline_comp
        }
        Component {
            id: m_filled_comp
            MD.FilledTextFieldShape {
                color: control.mdState.backgroundColor
                radius: MD.Token.shape.corner.extra_small
                bottomLineColor: control.mdState.indicatorColor
                bottomLineWidth: control.mdState.indicatorHeight
            }
        }
        Component {
            id: m_outline_comp
            MD.OutlineTextFieldShape {
                borderColor: control.mdState.outlineColor
                radius: MD.Token.shape.corner.extra_small
                floatWidth: m_placeholder.implicitWidth * m_placeholder.targetScale + 2 + 2
                floatX: 16 - 2
                open: control.activeFocus || control.text.length > 0
            }
        }
    }
    color: control.mdState.textColor
    placeholderTextColor: control.mdState.placeholderColor

}
