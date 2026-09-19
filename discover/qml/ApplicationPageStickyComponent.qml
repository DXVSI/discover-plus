/*
 *   SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.discover as Discover

Kirigami.Padding {
    id: stickyComponent

    required property Discover.AbstractResource application

    required property bool availableFromOnlySingleSource

    required property bool isOfflineUpgrade

    padding: Kirigami.Units.largeSpacing

    contentItem: RowLayout {
        id: row
        spacing: Kirigami.Units.largeSpacing

        Kirigami.IconTitleSubtitle {
            Layout.fillWidth: true

            font.weight: Font.DemiBold
            subtitleFont.weight: Font.Normal
            elide: Text.ElideRight

            icon.source: stickyComponent.application.icon
            title: stickyComponent.application.name
            subtitle: {
                if (stickyComponent.isOfflineUpgrade) {
                    return stickyComponent.application.upgradeText.length > 0 ? stickyComponent.application.upgradeText : "";
                } else if (stickyComponent.application.author.length > 0) {
                    return stickyComponent.application.author;
                } else {
                    return i18nc("As in 'this app is made by an unknown author'", "Unknown author");
                }
            }
        }

        InstallApplicationButton {
            application: stickyComponent.application
            buttonActiveFocusOnTab: true
            availableFromOnlySingleSource: stickyComponent.availableFromOnlySingleSource
            hideInvokeButton: false
            // A long name of a COPR package must not take the header for itself: in a narrow
            // window the title keeps its icon and some text first, the name gets what is left
            maximumPackageNameWidth: {
                const titleWidth = Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit * 5;
                const left = stickyComponent.availableWidth - titleWidth - packageNameSurroundingsWidth - scrollToTopButton.implicitWidth - row.spacing * 2;
                return Math.max(Kirigami.Units.gridUnit, Math.min(Kirigami.Units.gridUnit * 10, left));
            }
        }

        QQC2.Button {
            id: scrollToTopButton
            icon.name: "go-top-symbolic"
            text: i18nc("@action:button", "Scroll to top")
            display: QQC2.AbstractButton.IconOnly

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered || activeFocus
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            onClicked: stickyHeader.scrollToTop()
        }
    }
}
