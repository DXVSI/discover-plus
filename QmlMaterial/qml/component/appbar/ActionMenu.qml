import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

MD.Menu {
    id: root

    property alias actions: actionsInstantiator.model

    property Component submenuComponent
    property Component itemDelegate: MD.MenuItem {}
    property Component separatorDelegate: MD.MenuSeparator {
        property MD.Action action
    }
    property Component loaderDelegate: Loader {
        property MD.Action action
    }
    property MD.Action parentAction
    property T.MenuItem parentItem

    Instantiator {
        id: actionsInstantiator

        active: root.visible
        delegate: QtObject {
            readonly property MD.Action action: modelData

            property QtObject item: null
            property bool isSubMenu: false

            Component.onCompleted: {
                if (action?.separator) {
                    item = root.separatorDelegate.createObject(null, {
                        action
                    });
                } else if (action.displayComponent) {
                    item = root.loaderDelegate.createObject(null, {
                        action,
                        sourceComponent: action.displayComponent
                    });
                } else {
                    item = root.itemDelegate.createObject(null, {
                        action
                    });
                }
                root.addItem(item);
            }

            Component.onDestruction: {
                if (isSubMenu) {
                    root.removeMenu(item);
                } else {
                    root.removeItem(item);
                }
                item.destroy();
            }
        }
    }
}
