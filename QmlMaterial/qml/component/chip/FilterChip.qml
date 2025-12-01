import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateFilterChip mdState: MD.StateFilterChip {
        item: control
    }

    property alias elevated: control.mdState.elevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    checkable: !action
    checked: action
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: 8
    rightPadding: MD.Util.hasIcon(icon) ? 8 : 16
    spacing: 8

    icon.width: 18
    icon.height: 18
    font.capitalization: Font.Capitalize

    contentItem: Row {
        opacity: control.mdState.contentOpacity
        Item {
            id: m_leading
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: children[0].implicitWidth
            implicitHeight: children[0].implicitHeight

            states: State {
                when: !control.checked
                name: "hidden"
                PropertyChanges {
                    control.leftPadding: 16
                    m_leading.implicitWidth: 0
                    m_leading.opacity: 0
                    m_leading.visible: false
                }
            }

            transitions: [
                Transition {
                    to: ""
                    SequentialAnimation {
                        PropertyAction {
                            property: "visible"
                        }
                        ParallelAnimation {
                            OpacityAnimator {
                                duration: 100
                            }
                            NumberAnimation {
                                property: "implicitWidth"
                                duration: 100
                            }
                            NumberAnimation {
                                property: "leftPadding"
                                duration: 50
                            }
                        }
                    }
                },
                Transition {
                    to: "hidden"
                    SequentialAnimation {
                        ParallelAnimation {
                            OpacityAnimator {
                                duration: 100
                            }
                            NumberAnimation {
                                property: "implicitWidth"
                                duration: 100
                            }
                            NumberAnimation {
                                property: "leftPadding"
                                duration: 50
                            }
                        }
                        PropertyAction {
                            property: "visible"
                        }
                    }
                }
            ]

            Row {
                spacing: 0
                MD.Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: MD.Token.icon.check
                    size: control.icon.width
                    color: control.mdState.leadingIconColor
                }
                MD.Space {
                    spacing: control.spacing
                }
            }
        }
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            MD.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.label_large
            }
            MD.Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: name
                name: control.icon.name
                size: control.icon.width
                color: control.mdState.trailingIconColor
            }
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.checked ? 0 : 1
        border.color: control.mdState.outlineColor
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

}
