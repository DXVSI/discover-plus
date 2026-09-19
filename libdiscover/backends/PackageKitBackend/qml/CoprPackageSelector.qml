/*
 *   SPDX-FileCopyrightText: 2026 DXVSI <https://github.com/DXVSI>
 *
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

    // Read once per delivery: every read of the property builds the whole list anew
    readonly property var packages: resource.coprProjectPackages
    readonly property var shownPackages: {
        const filter = filterField.text.toLowerCase();
        return filter.length > 0 ? packages.filter(packageData => packageData.name.toLowerCase().includes(filter)) : packages;
    }

    // Not needed for the only package of a project, unless there is something to say about it
    Discover.Activatable.active: resource.isCoprProjectResource
        && (resource.coprInstallStatus !== "ready" || packages.length !== 1 || resource.coprInstallWarning.length > 0
            || resource.coprInstallError.length > 0)

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
                || root.resource.coprInstallError.length > 0
            ? Kirigami.MessageType.Warning
            : Kirigami.MessageType.Information
        text: {
            // A large project is cut short: what the user looks for may be in the rest
            const status = root.resource.coprPackageListLimited
                ? i18nd("libdiscover", "%1\nThis COPR project has more packages than Discover loads: the list below is limited, and a package that is missing from it cannot be installed from here.", statusText)
                : statusText;
            // One of the two requests may fail while the other one answers
            const error = root.resource.coprInstallError;
            return error.length > 0 && root.resource.coprInstallStatus !== "failed"
                ? i18nd("libdiscover", "%1 Not everything about this COPR project could be loaded: %2", status, error)
                : status;
        }
        readonly property string statusText: {
            const selected = root.resource.selectedCoprPackageName;
            switch (root.resource.coprInstallStatus) {
            case "idle":
            case "loading":
                return i18nd("libdiscover", "Loading packages for this COPR project...");
            case "failed":
                return root.resource.coprInstallError.length > 0
                    ? i18nd("libdiscover", "The packages of this COPR project could not be loaded: %1", root.resource.coprInstallError)
                    : i18nd("libdiscover", "The packages of this COPR project could not be loaded.");
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
            visible: root.resource.coprInstallStatus === "failed" || root.resource.coprInstallError.length > 0
            text: i18nd("libdiscover", "Retry")
            icon.name: "view-refresh"
            onTriggered: root.resource.fetchProjectPackages()
        }
    }

    Kirigami.SearchField {
        id: filterField
        Layout.fillWidth: true
        // For a list that does not fit into its frame
        visible: packagesScroll.visible && root.packages.length > 20
        placeholderText: i18nd("libdiscover", "Filter packages…")
    }

    // A project may have hundreds of packages: only the rows in the frame exist
    QQC2.ScrollView {
        id: packagesScroll
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(packagesView.contentHeight, Kirigami.Units.gridUnit * 18)
        visible: root.resource.coprProjectPackagesLoaded && root.packages.length > 1

        ListView {
            id: packagesView

            function showSelectedPackage() {
                const selected = root.resource.selectedCoprPackageName;
                const index = root.shownPackages.findIndex(packageData => packageData.name === selected);
                if (index >= 0) {
                    positionViewAtIndex(index, ListView.Contain);
                }
            }

            clip: true
            reuseItems: true
            spacing: Kirigami.Units.smallSpacing
            model: root.shownPackages
            onModelChanged: Qt.callLater(showSelectedPackage)

            delegate: QQC2.ItemDelegate {
                id: delegate

                required property var modelData

                width: ListView.view.width
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
