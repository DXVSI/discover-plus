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

    Discover.Activatable.active: resource.isCoprProjectResource
        && (!resource.coprProjectPackagesLoaded || resource.coprProjectPackages.length !== 1)

    spacing: Kirigami.Units.smallSpacing

    Component.onCompleted: resource.fetchProjectPackages()
    onResourceChanged: resource.fetchProjectPackages()

    function packageSubtitle(packageData): string {
        const parts = [];
        if (packageData.latestBuildState) {
            parts.push(i18nd("libdiscover", "latest build: %1", packageData.latestBuildState));
        }
        if (packageData.availableChroots.length > 0) {
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
        type: root.resource.coprProjectPackagesLoaded && root.resource.coprProjectPackages.length === 0
            ? Kirigami.MessageType.Warning
            : Kirigami.MessageType.Information
        text: {
            if (!root.resource.coprProjectPackagesLoaded) {
                return i18nd("libdiscover", "Loading packages for this COPR project...");
            }
            if (root.resource.coprProjectPackages.length === 0) {
                return i18nd("libdiscover", "No installable packages were returned for this COPR project.");
            }
            if (root.resource.selectedCoprPackageName.length === 0) {
                return i18nd("libdiscover", "Choose which package to install from this COPR project.");
            }
            return i18nd("libdiscover", "Selected package: %1", root.resource.selectedCoprPackageName);
        }
        visible: true
        showCloseButton: false
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
