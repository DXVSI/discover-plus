pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

Kirigami.Dialog {
    id: root

    title: i18n("Welcome to Discover")
    standardButtons: Kirigami.Dialog.NoButton
    closePolicy: Kirigami.Dialog.NoAutoClose

    property bool rpmFusionFree: true
    property bool rpmFusionNonfree: true
    property bool flathub: true

    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            source: "plasmadiscover"
            implicitWidth: Kirigami.Units.iconSizes.huge
            implicitHeight: Kirigami.Units.iconSizes.huge
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: i18n("Enable Additional Software Sources")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 25
            text: i18n("To access more applications including multimedia codecs, drivers, and popular software, we recommend enabling the following repositories:")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.CheckBox {
                id: rpmFusionFreeCheck
                Kirigami.FormData.label: i18n("RPM Fusion Free:")
                text: i18n("Open source software (multimedia codecs, tools)")
                checked: root.rpmFusionFree && !DiscoverApp.FedoraRepoManager.rpmFusionFreeInstalled
                enabled: !DiscoverApp.FedoraRepoManager.rpmFusionFreeInstalled && !DiscoverApp.FedoraRepoManager.installing
                visible: !DiscoverApp.FedoraRepoManager.rpmFusionFreeInstalled

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Includes ffmpeg, VLC plugins, and other multimedia software")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.rpmFusionFreeInstalled
                Kirigami.FormData.label: i18n("RPM Fusion Free:")
                text: i18n("Already installed")
                opacity: 0.7
            }

            QQC2.CheckBox {
                id: rpmFusionNonfreeCheck
                Kirigami.FormData.label: i18n("RPM Fusion Nonfree:")
                text: i18n("Proprietary software (NVIDIA drivers, Steam, Discord)")
                checked: root.rpmFusionNonfree && !DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled
                enabled: !DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled && !DiscoverApp.FedoraRepoManager.installing
                visible: !DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Includes NVIDIA drivers, Steam, Discord, and other proprietary software")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled
                Kirigami.FormData.label: i18n("RPM Fusion Nonfree:")
                text: i18n("Already installed")
                opacity: 0.7
            }

            QQC2.CheckBox {
                id: flathubCheck
                Kirigami.FormData.label: i18n("Flathub:")
                text: i18n("Flatpak applications repository")
                checked: root.flathub && !DiscoverApp.FedoraRepoManager.flathubInstalled
                enabled: !DiscoverApp.FedoraRepoManager.flathubInstalled && !DiscoverApp.FedoraRepoManager.installing
                visible: !DiscoverApp.FedoraRepoManager.flathubInstalled

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Access thousands of applications in sandboxed Flatpak format")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.flathubInstalled
                Kirigami.FormData.label: i18n("Flathub:")
                text: i18n("Already installed")
                opacity: 0.7
            }
        }

        // Progress indicator
        QQC2.BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: DiscoverApp.FedoraRepoManager.installing
            visible: running
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            visible: DiscoverApp.FedoraRepoManager.installing
            text: i18n("Installing repositories... Please authenticate if prompted.")
        }

        // Error message
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            visible: DiscoverApp.FedoraRepoManager.installError.length > 0
            text: DiscoverApp.FedoraRepoManager.installError
        }
    }

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Enable Selected")
            icon.name: "dialog-ok-apply"
            enabled: !DiscoverApp.FedoraRepoManager.installing &&
                     (rpmFusionFreeCheck.checked || rpmFusionNonfreeCheck.checked || flathubCheck.checked)
            onTriggered: {
                if (rpmFusionFreeCheck.checked || rpmFusionNonfreeCheck.checked) {
                    DiscoverApp.FedoraRepoManager.installRpmFusion(
                        rpmFusionFreeCheck.checked,
                        rpmFusionNonfreeCheck.checked,
                        true // Also install appstream data
                    )
                }
                if (flathubCheck.checked) {
                    DiscoverApp.FedoraRepoManager.installFlathub()
                }
            }
        },
        Kirigami.Action {
            text: i18n("Skip")
            icon.name: "dialog-cancel"
            enabled: !DiscoverApp.FedoraRepoManager.installing
            onTriggered: {
                DiscoverApp.FedoraRepoManager.firstRunCompleted = true
                root.close()
            }
        }
    ]

    Connections {
        target: DiscoverApp.FedoraRepoManager

        function onInstallationFinished(success) {
            if (success) {
                // Check if all selected items are now installed
                let allDone = true
                if (rpmFusionFreeCheck.checked && !DiscoverApp.FedoraRepoManager.rpmFusionFreeInstalled) {
                    allDone = false
                }
                if (rpmFusionNonfreeCheck.checked && !DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled) {
                    allDone = false
                }
                if (flathubCheck.checked && !DiscoverApp.FedoraRepoManager.flathubInstalled) {
                    allDone = false
                }

                if (allDone || (!rpmFusionFreeCheck.checked && !rpmFusionNonfreeCheck.checked && !flathubCheck.checked)) {
                    DiscoverApp.FedoraRepoManager.firstRunCompleted = true
                    // Trigger refresh to load new AppStream data
                    Discover.ResourcesModel.updateAction.trigger()
                    root.close()
                }
            }
        }
    }
}
