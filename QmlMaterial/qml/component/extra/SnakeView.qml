pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    property int m_snake_id: 0
    property bool bottomToTop: false

    model: m_snake

    MD.SnakeModel {
        id: m_snake
    }

    function show(text, duration = 4000, flag = MD.Enum.TFCloseable, action = null) {
        const snake = m_snake.createSnake();
        snake.text = text;
        snake.duration = duration;
        snake.flag = flag;
        if(action) snake.action = action;
        m_snake.showSnake(snake);
    }

    z: Infinity
    spacing: 5
    verticalLayoutDirection: bottomToTop ? ListView.BottomToTop : ListView.TopToBottom
    interactive: false

    remove: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                to: 0
                duration: 500
            }
        }
    }

    add: Transition {
        id: addTrans
        ParallelAnimation {
            NumberAnimation {
                properties: "height"
                easing.type: Easing.InOutCubic
                from: 0
                to: addTrans.ViewTransition.item.height
                duration: 200
            }
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
    }

    header: Item {
        implicitHeight: 32
    }

    delegate: Item {
        id: dg_bar
        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
        width: Math.min(Math.max(implicitWidth, 200), ListView.view.width)
        implicitWidth: children[0].implicitWidth
        implicitHeight: children[0].implicitHeight
        clip: true
        required property var model

        function close() {
            m_snake.removeById(model.sid);
        }

        MD.SnakeBar {
            x: 0
            y: 0
            width: parent.width
            height: implicitHeight
            text: dg_bar.model.text
            showClose: dg_bar.model.flag
            onClosed: dg_bar.close()
            action: dg_bar.model.action
            Component.onCompleted: {
                const sid = dg_bar.model.sid;
                if (dg_bar.model.action) {
                    dg_bar.model.action.triggered.connect(() => {
                        m_snake.removeById(sid);
                    });
                }
            }
        }
    }
}
