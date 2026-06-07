import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

pragma ComponentBehavior: Bound

ConditionalLoader {
    id: root

    property alias application: listener.resource
    property bool availableFromOnlySingleSource: false
    property bool buttonActiveFocusOnTab: false
    property var installOrRemoveButtonDisplayStyle: QQC2.AbstractButton.TextBesideIcon
    property bool hideInvokeButton: true

    readonly property alias isActive: listener.isActive
    readonly property bool isStateAvailable: application.state !== Discover.AbstractResource.Broken
    readonly property alias listener: listener

    Discover.TransactionListener {
        id: listener
    }

    readonly property Kirigami.Action action: Kirigami.Action {
        text: {
            if (!root.isStateAvailable) {
                return i18nc("State being fetched", "Loading…")
            }
            if (!root.application.isInstalled) {
                if (root.availableFromOnlySingleSource) {
                    return i18nc("@action:button %1 is the name of a software repository", "Install from %1", root.application.displayOrigin);
                }
                return i18nc("@action:button", "Install");
            }
            return i18n("Remove");
        }
        icon {
            name: root.application.isInstalled ? "edit-delete" : "download"
            color: !root.isActive && enabled
                ? (root.application.isInstalled ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor)
                : Kirigami.Theme.backgroundColor
        }
        visible: !root.isActive && (!root.application.isInstalled || root.application.isRemovable)
        enabled: !root.isActive && root.isStateAvailable
        onTriggered: root.click()
    }

    readonly property Kirigami.Action cancelAction: Kirigami.Action {
        text: i18n("Cancel")
        icon.name: "dialog-cancel"
        enabled: listener.isCancellable
        tooltip: listener.statusText
        onTriggered: {
            listener.cancel()
            enabled = false
        }
        visible: root.isActive
        onVisibleChanged: enabled = true
    }

    function click() {
        if (!isActive) {
            if (root.application.isInstalled) {
                Discover.ResourcesModel.removeApplication(root.application);
            } else {
                Discover.ResourcesModel.installApplication(root.application);
            }
        } else {
            console.warn("trying to un/install but resource still active", root.application.name);
        }
    }

    condition: root.isActive
    componentTrue: RowLayout {
        spacing: Kirigami.Units.smallSpacing
        TransactionProgressIndicator {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress / 100
        }

        // Cancel button
        QQC2.Button {
            Layout.fillHeight: true
            action: root.cancelAction

            display: QQC2.AbstractButton.IconOnly

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered || activeFocus
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }

    componentFalse: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Button {
            id: invokeButton
            visible: !root.hideInvokeButton && root.application.isInstalled && root.application.canExecute && !listener.isActive
            text: root.application.executeLabel
            icon.name: "media-playback-start-symbolic"
            onClicked: root.application.invokeApplication()
        }

        // Install/Remove button
        QQC2.Button {
            id: installOrRemoveButton

            visible: !root.application.isInstalled || root.application.isRemovable
            enabled: root.application.state !== Discover.AbstractResource.Broken
            activeFocusOnTab: root.buttonActiveFocusOnTab

            display: invokeButton.visible ? QQC2.AbstractButton.IconOnly : root.installOrRemoveButtonDisplayStyle
            text: root.action.text
            icon.name: root.action.icon.name
            icon.color: root.action.icon.color

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: (hovered || activeFocus) && display === QQC2.AbstractButton.IconOnly
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            onClicked: root.click()
        }
    }
}
