pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Button {
    id: control
    property bool busy: {
        if (action instanceof MD.Action) {
            return (action as MD.Action).busy;
        }
        return false;
    }
    contentItem.visible: !busy

    MD.Loader {
        id: m_loader
        anchors.centerIn: parent
        active: control.busy
        sourceComponent: comp_busy
        Component {
            id: comp_busy
            MD.CircularIndicator {
                anchors.centerIn: parent
                running: true
                strokeWidth: 2
                padding: 0
                implicitWidth: {
                    const w = Math.min(control.icon.width, control.icon.height);
                    return w - 6;
                }
                implicitHeight: implicitWidth
                color: {
                    const ctx = control.mdState.ctx;
                    switch (control.type) {
                    case MD.Enum.BtFilled:
                        return ctx.color.on_primary;
                    case MD.Enum.BtFilledTonal:
                        return ctx.color.on_secondary_container;
                    default:
                        return ctx.color.primary;
                    }
                }
            }
        }
    }
}
