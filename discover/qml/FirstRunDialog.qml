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
            text: i18n("Set Up Your System")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 25
            text: i18n("Configure your system for the best experience. Enable repositories, optimize settings, and install popular software.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            // ========== SYSTEM ==========
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("System")
            }

            QQC2.CheckBox {
                id: dnfConfigCheck
                Kirigami.FormData.label: i18n("DNF Configuration:")
                text: i18n("Optimize package manager (parallel downloads, fastest mirror)")
                checked: !DiscoverApp.FedoraRepoManager.dnfConfigured
                enabled: !DiscoverApp.FedoraRepoManager.dnfConfigured && !DiscoverApp.FedoraRepoManager.installing
                visible: !DiscoverApp.FedoraRepoManager.dnfConfigured

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Sets max_parallel_downloads=10, fastestmirror=True, defaultyes=True, keepcache=True")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.dnfConfigured
                Kirigami.FormData.label: i18n("DNF Configuration:")
                text: i18n("Already optimized")
                opacity: 0.7
            }

            QQC2.CheckBox {
                id: ciscoCheck
                Kirigami.FormData.label: i18n("Cisco OpenH264:")
                text: i18n("Disable repository (blocked in Russia, causes timeouts)")
                checked: DiscoverApp.FedoraRepoManager.ciscoRepoEnabled
                enabled: DiscoverApp.FedoraRepoManager.ciscoRepoEnabled && !DiscoverApp.FedoraRepoManager.installing
                visible: DiscoverApp.FedoraRepoManager.ciscoRepoEnabled
            }

            QQC2.Label {
                visible: !DiscoverApp.FedoraRepoManager.ciscoRepoEnabled
                Kirigami.FormData.label: i18n("Cisco OpenH264:")
                text: i18n("Already disabled")
                opacity: 0.7
            }

            // ========== REPOSITORIES ==========
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Repositories")
            }

            QQC2.CheckBox {
                id: rpmFusionFreeCheck
                Kirigami.FormData.label: i18n("RPM Fusion Free:")
                text: i18n("Open source multimedia codecs and tools")
                checked: !DiscoverApp.FedoraRepoManager.rpmFusionFreeInstalled
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
                text: i18n("NVIDIA drivers, Steam, Discord and more")
                checked: !DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled
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
                checked: !DiscoverApp.FedoraRepoManager.flathubInstalled
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

            // ========== ADDITIONAL REPOSITORIES ==========
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Additional Repositories")
            }

            QQC2.CheckBox {
                id: nvidiaRepoCheck
                Kirigami.FormData.label: i18n("NVIDIA Drivers:")
                text: i18n("Enable GPU driver repository")
                checked: false
                enabled: !DiscoverApp.FedoraRepoManager.nvidiaRepoEnabled
                         && !DiscoverApp.FedoraRepoManager.installing
                         && (DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled || rpmFusionNonfreeCheck.checked)
                visible: !DiscoverApp.FedoraRepoManager.nvidiaRepoEnabled

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: enabled
                    ? i18n("Enable RPM Fusion NVIDIA Driver repository for GPU drivers")
                    : i18n("Requires RPM Fusion Nonfree to be installed first")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.nvidiaRepoEnabled
                Kirigami.FormData.label: i18n("NVIDIA Drivers:")
                text: i18n("Already enabled")
                opacity: 0.7
            }

            QQC2.CheckBox {
                id: steamRepoCheck
                Kirigami.FormData.label: i18n("Steam:")
                text: i18n("Enable gaming platform repository")
                checked: false
                enabled: !DiscoverApp.FedoraRepoManager.steamRepoEnabled
                         && !DiscoverApp.FedoraRepoManager.installing
                         && (DiscoverApp.FedoraRepoManager.rpmFusionNonfreeInstalled || rpmFusionNonfreeCheck.checked)
                visible: !DiscoverApp.FedoraRepoManager.steamRepoEnabled

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: enabled
                    ? i18n("Enable RPM Fusion Steam repository")
                    : i18n("Requires RPM Fusion Nonfree to be installed first")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.steamRepoEnabled
                Kirigami.FormData.label: i18n("Steam:")
                text: i18n("Already enabled")
                opacity: 0.7
            }

            QQC2.CheckBox {
                id: googleChromeCheck
                Kirigami.FormData.label: i18n("Google Chrome:")
                text: i18n("Enable web browser repository")
                checked: false
                enabled: !DiscoverApp.FedoraRepoManager.googleChromeRepoEnabled && !DiscoverApp.FedoraRepoManager.installing
                visible: !DiscoverApp.FedoraRepoManager.googleChromeRepoEnabled

                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Add or enable Google Chrome repository")
            }

            QQC2.Label {
                visible: DiscoverApp.FedoraRepoManager.googleChromeRepoEnabled
                Kirigami.FormData.label: i18n("Google Chrome:")
                text: i18n("Already enabled")
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
            text: i18n("Applying settings... Please authenticate if prompted.")
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
            text: i18n("Apply")
            icon.name: "dialog-ok-apply"
            enabled: !DiscoverApp.FedoraRepoManager.installing &&
                     ((dnfConfigCheck.visible && dnfConfigCheck.checked) ||
                      (ciscoCheck.visible && ciscoCheck.checked) ||
                      (rpmFusionFreeCheck.visible && rpmFusionFreeCheck.checked) ||
                      (rpmFusionNonfreeCheck.visible && rpmFusionNonfreeCheck.checked) ||
                      (flathubCheck.visible && flathubCheck.checked) ||
                      (nvidiaRepoCheck.visible && nvidiaRepoCheck.checked) ||
                      (steamRepoCheck.visible && steamRepoCheck.checked) ||
                      (googleChromeCheck.visible && googleChromeCheck.checked))
            onTriggered: {
                DiscoverApp.FedoraRepoManager.applySetup(
                    dnfConfigCheck.visible && dnfConfigCheck.checked,
                    ciscoCheck.visible && ciscoCheck.checked,
                    rpmFusionFreeCheck.visible && rpmFusionFreeCheck.checked,
                    rpmFusionNonfreeCheck.visible && rpmFusionNonfreeCheck.checked,
                    flathubCheck.visible && flathubCheck.checked,
                    googleChromeCheck.visible && googleChromeCheck.checked,
                    nvidiaRepoCheck.visible && nvidiaRepoCheck.checked,
                    steamRepoCheck.visible && steamRepoCheck.checked
                )
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
                DiscoverApp.FedoraRepoManager.firstRunCompleted = true
                Discover.ResourcesModel.updateAction.trigger()
                root.close()
            }
        }
    }
}
