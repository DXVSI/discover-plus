import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.FABNormal
    property int color: MD.Enum.FABColorPrimary
    property QtObject flickable: null
    property MD.StateFAB mdState: MD.StateFAB {
        item: control
    }

    Binding {
        control.mdState.color: control.color
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: _size(type, 8, 16, 30)
    spacing: 0

    icon.width: _size(type, 24, 24, 36)
    icon.height: _size(type, 24, 24, 36)

    font.weight: MD.Token.typescale.label_large.weight
    font.pixelSize: Math.min(icon.width, icon.height)
    font.family: MD.Token.font.icon_family

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        Text {
            anchors.centerIn: parent
            font: control.font
            text: control.icon.name
            color: control.mdState.textColor
            lineHeight: font.pixelSize
            lineHeightMode: Text.FixedHeight
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: control._size(control.type, 40, 56, 96)
        implicitHeight: control._size(control.type, 40, 56, 96)

        radius: control._size(control.type, 12, 16, 28)
        color: control.mdState.backgroundColor

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: control.mdState.ctx.color.outline

        elevationVisible: elevation && color.a > 0 && !control.flat
        elevation: control.mdState.elevation

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

    function _size(t, small, normal, large) {
        return t == MD.Enum.FABSmall ? small : (t == MD.Enum.FABLarge ? large : normal);
    }

    MD.InputBlock {
        id: m_input_block
        when: m_show_state.state == 'hide'
        target: control
    }

    Item {
        id: m_show_state
        visible: false
        state: "show"
        states: [
            State {
                name: "hide"
                when: {
                    const fk = control.flickable;
                    const fk_end = fk && !MD.Util.epsilonEqual(fk.visibleArea.heightRatio, 1.0) && (1.0 - fk.visibleArea.heightRatio - fk.visibleArea.yPosition) * fk.height < 4.0;
                    return !control.visible || fk_end;
                }
                PropertyChanges {
                    control.scale: 0.5
                    control.opacity: 0.0
                }
            },
            State {
                name: "show"
                when: true
                PropertyChanges {
                    control.scale: 1.0
                    control.opacity: 1.0
                }
            }
        ]

        transitions: [
            Transition {
                to: 'show'
                ParallelAnimation {
                    XAnimator {
                        target: control
                        from: control.x + 16
                        to: control.x
                        duration: 200
                    }
                    YAnimator {
                        target: control
                        from: control.y + 16
                        to: control.y
                        duration: 200
                    }
                    ScaleAnimator {
                        duration: 200
                    }
                    OpacityAnimator {
                        duration: 200
                    }
                }
            },
            Transition {
                to: 'hide'
                ParallelAnimation {
                    XAnimator {
                        target: control
                        from: control.x
                        to: control.x + 16
                        duration: 200
                    }
                    YAnimator {
                        target: control
                        from: control.y
                        to: control.y + 16
                        duration: 200
                    }
                    ScaleAnimator {
                        duration: 200
                    }
                    OpacityAnimator {
                        duration: 200
                    }
                }
            }
        ]
    }
}
