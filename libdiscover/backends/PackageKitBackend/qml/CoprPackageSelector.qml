/*
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    required property Discover.AbstractResource resource

    // Not needed for the only package of a project, unless there is something to say about it
    Discover.Activatable.active: resource.isCoprProjectResource
        && (resource.coprInstallStatus !== "ready" || resource.coprProjectPackages.length !== 1 || resource.coprInstallWarning.length > 0)

    spacing: Kirigami.Units.smallSpacing

    // The install takes the package that was selected when it started
    Discover.TransactionListener {
        id: transactionListener
        resource: root.resource
    }

    Component.onCompleted: resource.fetchProjectPackages()
    onResourceChanged: resource.fetchProjectPackages()

    function packageSubtitle(packageData): string {
        const parts = [];
        if (packageData.latestBuildState) {
            parts.push(i18nd("libdiscover", "latest build: %1", packageData.latestBuildState));
        }
        if (packageData.isAvailabilityKnown) {
            if (packageData.isAvailableForCurrentFedora) {
                parts.push(i18nd("libdiscover", "available for this Fedora version"));
            } else {
                parts.push(i18nd("libdiscover", "not built for this Fedora version"));
            }
        }
        return parts.join(" - ");
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        type: ["failed", "empty", "unavailable"].includes(root.resource.coprInstallStatus) || root.resource.coprInstallWarning.length > 0
            ? Kirigami.MessageType.Warning
            : Kirigami.MessageType.Information
        text: {
            const selected = root.resource.selectedCoprPackageName;
            switch (root.resource.coprInstallStatus) {
            case "idle":
            case "loading":
                return i18nd("libdiscover", "Loading packages for this COPR project...");
            case "failed":
                return i18nd("libdiscover", "The packages of this COPR project could not be loaded.");
            case "empty":
                return i18nd("libdiscover", "No installable packages were returned for this COPR project.");
            case "needs-selection":
                return i18nd("libdiscover", "Choose which package to install from this COPR project.");
            case "unavailable":
                return selected.length > 0
                    ? i18nd("libdiscover", "Selected package: %1. It is not built for this Fedora version and architecture.", selected)
                    : i18nd("libdiscover", "Nothing in this COPR project is built for this Fedora version and architecture.");
            }
            if (root.resource.coprInstallWarning.length > 0) {
                return i18nd("libdiscover", "Selected package: %1. %2", selected, root.resource.coprInstallWarning);
            }
            return i18nd("libdiscover", "Selected package: %1", selected);
        }
        visible: true
        showCloseButton: false
        actions: Kirigami.Action {
            visible: root.resource.coprInstallStatus === "failed"
            text: i18nd("libdiscover", "Retry")
            icon.name: "view-refresh"
            onTriggered: root.resource.fetchProjectPackages()
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        visible: root.resource.coprProjectPackagesLoaded && root.resource.coprProjectPackages.length > 1

        Repeater {
            model: root.resource.coprProjectPackages

            delegate: QQC2.ItemDelegate {
                id: delegate

                required property var modelData

                Layout.fillWidth: true
                // The radio button included
                enabled: !transactionListener.isActive
                onClicked: root.resource.selectCoprProjectPackage(modelData.name)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.RadioButton {
                        checked: root.resource.selectedCoprPackageName === delegate.modelData.name
                        onClicked: root.resource.selectCoprProjectPackage(delegate.modelData.name)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: delegate.modelData.version
                                ? i18nd("libdiscover", "%1 (%2)", delegate.modelData.name, delegate.modelData.version)
                                : delegate.modelData.name
                            font.weight: Font.DemiBold
                            wrapMode: Text.Wrap
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: root.packageSubtitle(delegate.modelData)
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
