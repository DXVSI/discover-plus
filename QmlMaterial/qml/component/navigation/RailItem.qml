import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property MD.StateRailItem mdState: MD.StateRailItem {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    spacing: 8

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pointSize: MD.Token.typescale.label_large.size

    property bool expand: width > 80
    property int lineHeight: MD.Token.typescale.label_large.line_height
    property Item trailing: null
    readonly property real range: (control.width - (56 + 24)) / (336 - 56)

    contentItem: Item {
        implicitHeight: m_content_main.implicitHeight + (control.expand ? 0 : m_text.implicitHeight)
        implicitWidth: m_content_main.implicitWidth

        MD.Control {
            id: m_content_main
            height: parent.height - (control.expand ? 0 : m_text.implicitHeight)
            width: parent.width

            hoverEnabled: false
            focusPolicy: Qt.NoFocus
            leftInset: 12
            rightInset: 12
            contentItem: Row {
                Item {
                    height: 2
                    width: (56 + 24 - control.icon.width) / 2
                }
                MD.Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: control.icon.name
                    size: control.icon.width
                    color: control.mdState.supportTextColor
                    fill: control.checked
                }
                MD.ItemProxy {
                    item: control.trailing
                }
            }

            background: Item {
                id: m_background
                implicitWidth: control.expand ? 336 : 56
                implicitHeight: control.expand ? 56 : 32

                MD.ElevationRectangle {
                    x: 0
                    y: 0
                    width: m_background.width
                    height: m_background.height

                    radius: 28
                    color: control.mdState.backgroundColor

                    elevationVisible: control.enabled && color.a > 0
                    elevation: control.mdState.elevation

                    MD.Ripple2 {
                        anchors.fill: parent
                        readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
                        radius: height / 2
                        pressX: p.x
                        pressY: p.y
                        pressed: control.pressed
                        stateOpacity: control.mdState.stateLayerOpacity
                        color: control.mdState.stateLayerColor
                    }
                }
            }
        }
        MD.Text {
            id: m_text
            x: {
                const min = ((56 + 24) - width) / 2;
                const max = ((56 + 24 + control.icon.width) / 2 + 12);
                return min + (max - min) * control.range;
            }
            y: {
                const min = m_content_main.height + 2;
                const max = (m_content_main.height - height) / 2;
                return min + (max - min) * control.range;
            }
            opacity: {
                const left = 0.2;
                const right = 0.8;
                const w = control.range;
                return MD.Util.teleportCurve(w, left, right);
            }
            typescale: {
                control.range > 0.5 ? MD.Token.typescale.label_large : MD.Token.typescale.label_medium;
            }
            font.capitalization: Font.Capitalize
            text: control.text
            prominent: control.checked
        }
    }

    /*
    contentItem: ColumnLayout {
        spacing: 4
        MD.Control {
            Layout.alignment: Qt.AlignHCenter
            hoverEnabled: false
            focusPolicy: Qt.NoFocus
            leftInset: 12
            rightInset: 12

            contentItem: MD.Icon {
                name: control.icon.name
                size: control.icon.width
                color: control.mdState.supportTextColor
                fill: control.checked
            }

            background: Item {
                implicitWidth: 56
                implicitHeight: 32
                MD.ElevationRectangle {
                    anchors.centerIn: parent
                    height: parent.height
                    width: 56

                    NumberAnimation on width {
                        alwaysRunToEnd: true
                        from: 48
                        to: 56
                        duration: 200
                        running: control.checked
                    }

                    radius: height / 2
                    color: control.mdState.backgroundColor
                    elevation: control.mdState.elevation
                }
                MD.Ripple2 {
                    readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
                    anchors.fill: parent
                    radius: height / 2
                    pressX: p.x
                    pressY: p.y
                    pressed: control.pressed
                    stateOpacity: control.mdState.stateLayerOpacity
                    color: control.mdState.stateLayerColor
                }
            }
        }

        MD.Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            font.capitalization: Font.Capitalize
            typescale: MD.Token.typescale.label_medium
            text: control.text
            prominent: control.checked
        }
    }
    */

    background: Item {}
}
