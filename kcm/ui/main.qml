/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils

SimpleKCM {
    id: root

    ConfigModule.buttons: ConfigModule.Default | ConfigModule.Apply

    QQC2.ButtonGroup {
        id: autoUpdatesGroup
        onCheckedButtonChanged: {
            kcm.updatesSettings.useUnattendedUpdates = automaticallyRadio.checked
        }
    }

    QQC2.ButtonGroup {
        id: offlineUpdatesGroup
        onCheckedButtonChanged: {
            kcm.discoverSettings.useOfflineUpdates = offlineUpdatesOption.checked
        }
    }

    implicitWidth: Kirigami.Units.gridUnit * 38
    implicitHeight: Kirigami.Units.gridUnit * 35


    Kirigami.FormLayout {
        width: parent.width

        QQC2.RadioButton {
            Kirigami.FormData.label: i18n("Update software:")
            text: i18n("Manually")

            QQC2.ButtonGroup.group: autoUpdatesGroup
            checked: !kcm.updatesSettings.useUnattendedUpdates

            SettingStateBinding {
                configObject: kcm.updatesSettings
                settingName: "useUnattendedUpdates"
            }
        }
        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.RadioButton {
                id: automaticallyRadio
                text: i18n("Automatically")

                QQC2.ButtonGroup.group: autoUpdatesGroup
                checked: kcm.updatesSettings.useUnattendedUpdates

                SettingStateBinding {
                    configObject: kcm.updatesSettings
                    settingName: "useUnattendedUpdates"
                }
            }

            Kirigami.ContextualHelpButton {
                toolTipText: xi18nc("@info", "Software updates will be downloaded automatically when they become available. Updates for applications will be installed immediately, while updates for the system will be installed the next time it’s restarted.")
            }
        }

        QQC2.ComboBox {
            id: frequencyComboBox
            Kirigami.FormData.label: kcm.updatesSettings.useUnattendedUpdates ? i18nc("@title:group", "Update frequency:") : i18nc("@title:group", "Notification frequency:")
            textRole: "text"
            valueRole: "value"

            readonly property var updatesFrequencyModel: [
                { text: i18nc("@item:inlistbox", "Daily"),   value: 60 * 60 * 24 },
                { text: i18nc("@item:inlistbox", "Weekly"),  value: 60 * 60 * 24 * 7 },
                { text: i18nc("@item:inlistbox", "Monthly"), value: 60 * 60 * 24 * 30 },
                { text: i18nc("@item:inlistbox", "Never"),   value: -1 },
            ]

            // Same as updatesFrequencyModel but without "Never"
            readonly property var unattendedUpdatesFrequencyModel: [
                updatesFrequencyModel[0],
                updatesFrequencyModel[1],
                updatesFrequencyModel[2],
            ]

            model: kcm.updatesSettings.useUnattendedUpdates ? unattendedUpdatesFrequencyModel : updatesFrequencyModel

            currentValue: kcm.updatesSettings.requiredNotificationInterval
            onActivated:  kcm.updatesSettings.requiredNotificationInterval = currentValue

            Connections {
                target: kcm.updatesSettings

                function onUseUnattendedUpdatesChanged() {
                    if (kcm.updatesSettings.useUnattendedUpdates &&
                        kcm.updatesSettings.requiredNotificationInterval === frequencyComboBox.updatesFrequencyModel[3].value) {
                        kcm.updatesSettings.requiredNotificationInterval = frequencyComboBox.updatesFrequencyModel[0].value
                    }
                }
            }

            SettingStateBinding {
                configObject: kcm.updatesSettings
                settingName: "requiredNotificationInterval"
            }
        }

        Item {
            implicitHeight: Kirigami.Units.largeSpacing
        }

        ColumnLayout {
            spacing: 0
            Kirigami.FormData.label: i18n("Apply system updates:")
            Kirigami.FormData.buddyFor: offlineUpdatesOption
            visible: !kcm.mandatoryRebootAfterUpdate
            enabled: !kcm.discoverSettings.isUseOfflineUpdatesImmutable

            QQC2.RadioButton {
                id: offlineUpdatesOption
                text: i18nc("@option:radio part of the logical sentence 'Apply system updates after rebooting'", "After rebooting")

                QQC2.ButtonGroup.group: offlineUpdatesGroup
                checked: kcm.discoverSettings.useOfflineUpdates

                SettingStateBinding {
                    configObject: kcm.discoverSettings
                    settingName: "useOfflineUpdates"
                }
            }

            QQC2.Label {
                text: i18nc("@label The thing being recommended is to use the 'apply updates when rebooting' setting", "Recommended to maximize system stability")
                leftPadding: offlineUpdatesOption.indicator.width
                font: Kirigami.Theme.smallFont
            }

            QQC2.RadioButton {
                text: i18nc("@option:radio part of the logical sentence 'Apply system updates immediately'", "Immediately")

                QQC2.ButtonGroup.group: offlineUpdatesGroup
                enabled: !kcm.discoverSettings.isUseOfflineUpdatesImmutable
                checked: !kcm.discoverSettings.useOfflineUpdates

                SettingStateBinding {
                    configObject: kcm.discoverSettings
                    settingName: "useOfflineUpdates"
                }
            }
        }
    }
}
