import QtQuick

Text {
    id: root
    property bool controlFocus: false
    property bool controlHasText: false
    property int verticalPadding: 8
    property int controlHeight: height
    property int largestHeight: 0

    property bool filled: false
    property real targetScale: 0.8
    y: (controlHeight - height) / 2.0
    scale: 1.0
    verticalAlignment: Text.AlignVCenter

    transformOrigin: {
        switch (effectiveHorizontalAlignment) {
        case Text.AlignLeft:
        case Text.AlignJustify:
            return Item.Left;
        case Text.AlignRight:
            return Item.Right;
        case Text.AlignHCenter:
            return Item.Center;
        }
    }

    states: [
        State {
            name: 'float'
            when: root.controlFocus || root.controlHasText
            PropertyChanges {
                root.y: root.filled ? root.verticalPadding : -root.largestHeight / 2.0
                root.scale: root.targetScale
            }
        }
    ]

    transitions: [
        Transition {
            to: ''
            ParallelAnimation {
                YAnimator {
                    duration: 300
                    easing.type: Easing.OutSine
                }
                ScaleAnimator {
                    duration: 300
                    easing.type: Easing.OutSine
                }
            }
        },
        Transition {
            to: 'float'
            ParallelAnimation {
                YAnimator {
                    duration: 300
                    easing.type: Easing.OutSine
                }
                ScaleAnimator {
                    duration: 300
                    easing.type: Easing.OutSine
                }
            }
        }
    ]

    Component.onCompleted: {
        largestHeight = implicitHeight;
        effectiveHorizontalAlignmentChanged();
    }
}
