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

    // COPR resources say why nothing can be installed yet. Everything else has no such
    // property, the status is empty then and nothing below applies.
    readonly property string coprStatus: application.isInstalled ? "" : (application.coprInstallStatus ?? "")
    readonly property string coprPackageName: application.selectedCoprPackageName ?? ""
    // In a list the button opens the application page to choose a package and is only
    // worth showing when it does something
    property bool listItem: false
    readonly property bool hasAction: isActive || !["idle", "loading", "empty", "unavailable"].includes(coprStatus)
    readonly property bool buttonEnabled: {
        switch (coprStatus) {
        case "failed":
            return true;
        case "needs-selection":
            return listItem;
        case "idle":
        case "loading":
        case "empty":
        case "unavailable":
            return false;
        }
        return isStateAvailable;
    }
    readonly property string coprReason: {
        switch (coprStatus) {
        case "idle":
        case "loading":
            return i18nc("@info:tooltip", "Loading the packages of this COPR project…");
        case "failed":
            return i18nc("@info:tooltip", "The packages of this COPR project could not be loaded. Click to try again.");
        case "needs-selection":
            return listItem
                ? i18nc("@info:tooltip", "This COPR project has several packages. Click to choose which one to install.")
                : i18nc("@info:tooltip", "This COPR project has several packages. Choose which one to install in the package list on this page.");
        case "empty":
            return i18nc("@info:tooltip", "This COPR project has no packages.");
        case "unavailable":
            return coprPackageName.length > 0
                ? i18nc("@info:tooltip %1 is the name of a package", "%1 is not built for this Fedora version and architecture.", coprPackageName)
                : i18nc("@info:tooltip", "Nothing in this COPR project is built for this Fedora version and architecture.");
        }
        return "";
    }
    // What the button does, for the tooltip and for screen readers: from a COPR project
    // the package is installed that was selected automatically or by the user
    readonly property string actionName: coprStatus === "ready" && coprPackageName.length > 0
        ? i18nc("@info:tooltip %1 is the name of a package", "Install %1", coprPackageName)
        : action.text

    signal packageSelectionRequested()

    Discover.TransactionListener {
        id: listener
    }

    readonly property Kirigami.Action action: Kirigami.Action {
        text: {
            switch (root.coprStatus) {
            case "failed":
                return i18nc("@action:button", "Retry");
            case "needs-selection":
                return i18nc("@action:button", "Choose Package…");
            case "empty":
            case "unavailable":
                return i18nc("@action:button", "Not Available");
            }
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
            name: {
                switch (root.coprStatus) {
                case "failed":
                    return "view-refresh";
                case "needs-selection":
                    return "view-list-details";
                }
                return root.application.isInstalled ? "edit-delete" : "download";
            }
            color: {
                if (root.isActive || !enabled) {
                    return Kirigami.Theme.backgroundColor;
                }
                if (root.coprStatus === "failed" || root.coprStatus === "needs-selection") {
                    return Kirigami.Theme.textColor;
                }
                return root.application.isInstalled ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor;
            }
        }
        visible: !root.isActive && (!root.application.isInstalled || root.application.isRemovable)
        enabled: !root.isActive && root.buttonEnabled
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
            // Never an install in these states
            if (root.coprStatus === "failed") {
                // A list item only needs the monitor, the application page the details as well
                if (root.listItem) {
                    root.application.fetchProjectMonitor();
                } else {
                    root.application.fetchProjectPackages();
                }
                return;
            }
            if (root.coprStatus === "needs-selection") {
                root.packageSelectionRequested();
                return;
            }
            if (!root.buttonEnabled) {
                return;
            }
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

        // A disabled button is not hovered: the reason of a COPR state is shown for the row,
        // unless a list has disabled it as a whole to hide it
        HoverHandler {
            id: rowHover
            enabled: root.coprReason.length > 0 && !installOrRemoveButton.enabled && root.enabled
        }

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
            enabled: root.buttonEnabled
            activeFocusOnTab: root.buttonActiveFocusOnTab

            display: invokeButton.visible ? QQC2.AbstractButton.IconOnly : root.installOrRemoveButtonDisplayStyle
            text: root.action.text
            icon.name: root.action.icon.name
            icon.color: root.action.icon.color

            Accessible.name: root.actionName
            Accessible.description: root.coprReason

            QQC2.ToolTip.text: root.coprReason.length > 0 ? root.coprReason : root.actionName
            QQC2.ToolTip.visible: ((hovered || activeFocus) && (display === QQC2.AbstractButton.IconOnly || root.actionName !== text || root.coprReason.length > 0))
                || rowHover.hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            onClicked: root.click()
        }
    }
}
