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

    Discover.Activatable.active: true

    readonly property var details: resource.coprDetails

    spacing: Kirigami.Units.smallSpacing

    function hasText(value): bool {
        return value !== undefined && value !== null && String(value).length > 0;
    }

    function yesNo(value): string {
        return value ? i18nd("libdiscover", "yes") : i18nd("libdiscover", "no");
    }

    function addRow(rows, label, value): void {
        if (hasText(value)) {
            rows.push({
                "label": label,
                "value": value
            });
        }
    }

    function packageRows(): var {
        const rows = [];
        addRow(rows, i18nd("libdiscover", "Repository:"), details.repository);
        addRow(rows, i18nd("libdiscover", "Package:"), details.packageName);
        addRow(rows, i18nd("libdiscover", "Available for:"), details.availableFor);
        addRow(rows, i18nd("libdiscover", "Current Fedora:"), details.currentFedoraText);
        return rows;
    }

    function buildRows(): var {
        const rows = [];
        addRow(rows, i18nd("libdiscover", "Latest version:"), details.latestVersion);
        addRow(rows, i18nd("libdiscover", "Latest build:"), details.latestBuildState);
        addRow(rows, i18nd("libdiscover", "Submitted:"), details.buildSubmittedOn);
        addRow(rows, i18nd("libdiscover", "Started:"), details.buildStartedOn);
        addRow(rows, i18nd("libdiscover", "Finished:"), details.buildFinishedOn);
        addRow(rows, i18nd("libdiscover", "Submitter:"), details.buildSubmitter);
        return rows;
    }

    function sourceRows(): var {
        const rows = [];
        addRow(rows, i18nd("libdiscover", "Type:"), details.sourceType);
        addRow(rows, i18nd("libdiscover", "Spec:"), details.sourceSpec);
        addRow(rows, i18nd("libdiscover", "Subdirectory:"), details.sourceSubdirectory);
        return rows;
    }

    function repositoryFlagRows(): var {
        const rows = [];
        rows.push({
            "label": i18nd("libdiscover", "AppStream metadata:"),
            "value": yesNo(details.appstream)
        });
        rows.push({
            "label": i18nd("libdiscover", "Follows Fedora branching:"),
            "value": yesNo(details.followFedoraBranching)
        });
        rows.push({
            "label": i18nd("libdiscover", "Auto-prune:"),
            "value": yesNo(details.autoPrune)
        });
        rows.push({
            "label": i18nd("libdiscover", "Network during builds:"),
            "value": yesNo(details.enableNet)
        });
        rows.push({
            "label": i18nd("libdiscover", "Module hotfixes:"),
            "value": yesNo(details.moduleHotfixes)
        });
        addRow(rows, i18nd("libdiscover", "Repository priority:"), details.repoPriority);
        if (details.additionalRepos.length > 0) {
            addRow(rows, i18nd("libdiscover", "Additional repositories:"), details.additionalRepos.join(", "));
        }
        return rows;
    }

    component DetailSection: ColumnLayout {
        id: section

        required property string title
        required property var rows

        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        visible: rows.length > 0

        QQC2.Label {
            Layout.fillWidth: true
            text: section.title
            font.weight: Font.DemiBold
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            Repeater {
                model: section.rows

                delegate: RowLayout {
                    id: rowDelegate

                    required property var modelData

                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    spacing: Kirigami.Units.largeSpacing

                    QQC2.Label {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                        Layout.alignment: Qt.AlignTop
                        text: rowDelegate.modelData.label
                        color: Kirigami.Theme.disabledTextColor
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: rowDelegate.modelData.value
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        type: Kirigami.MessageType.Information
        text: i18nd("libdiscover", "COPR repositories are not officially supported by Fedora. Use at your own risk.")
        visible: true
        showCloseButton: false
    }

    Repeater {
        model: root.resource.coprWarnings

        delegate: Kirigami.InlineMessage {
            required property var modelData

            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            text: modelData.text
            visible: true
            showCloseButton: false
        }
    }

    DetailSection {
        title: i18nd("libdiscover", "COPR Details")
        rows: root.packageRows()
    }

    DetailSection {
        title: i18nd("libdiscover", "Build")
        rows: root.buildRows()
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        visible: sourceSection.visible || root.hasText(root.details.sourceUrl) || root.hasText(root.details.buildRepositoryUrl)

        DetailSection {
            id: sourceSection

            title: i18nd("libdiscover", "Source")
            rows: root.sourceRows()
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            visible: root.hasText(root.details.sourceUrl)
            text: i18nd("libdiscover", "Source URL")
            url: root.details.sourceUrl
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            visible: root.hasText(root.details.buildRepositoryUrl)
            text: i18nd("libdiscover", "Build repository")
            url: root.details.buildRepositoryUrl
        }
    }

    DetailSection {
        title: i18nd("libdiscover", "Repository Flags")
        rows: root.repositoryFlagRows()
    }

    DetailSection {
        title: i18nd("libdiscover", "Project")
        rows: {
            const rows = [];
            root.addRow(rows, i18nd("libdiscover", "Contact:"), root.details.contact);
            return rows;
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        visible: root.hasText(root.details.instructions)

        QQC2.Label {
            Layout.fillWidth: true
            text: i18nd("libdiscover", "Instructions")
            font.weight: Font.DemiBold
        }

        Kirigami.SelectableLabel {
            Layout.fillWidth: true
            text: root.details.instructions
            textFormat: Text.RichText
            wrapMode: Text.Wrap
        }
    }
}
