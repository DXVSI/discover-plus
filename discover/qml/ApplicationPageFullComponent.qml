/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
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
    id: fullComponent

    required property Discover.AbstractResource application

    required property bool availableFromOnlySingleSource

    required property bool isOfflineUpgrade

    required property bool isTechnicalPackage

    required property var colorForLicenseType

    signal openContentRatingDialog()
    signal openLicenseDetailsDialog(licenseType: string)
    signal openAllLicensesSheet()

    padding: Kirigami.Units.gridUnit

    contentItem: GridLayout {
        id: fullComponentLayout

        readonly property bool hasMetadata: !fullComponent.isOfflineUpgrade
        readonly property real effectiveInfoWidth: infoLayout.implicitWidth
        readonly property real effectiveMetaWidth: hasMetadata ? metaLayout.implicitWidth : 0

        readonly property bool stackedMode: hasMetadata && (effectiveInfoWidth + effectiveMetaWidth + fullComponentLayout.rowSpacing > width)

        rowSpacing: Kirigami.Units.gridUnit
        columnSpacing: Kirigami.Units.gridUnit
        columns: stackedMode || !hasMetadata ? 1 : 2
        rows: stackedMode ? 2 : 1

        // App info (icon, name, author, rating, install button)
        ColumnLayout {
            id: infoLayout
            Layout.alignment: fullComponentLayout.stackedMode ? Qt.AlignHCenter : Qt.AlignLeft

            spacing: Kirigami.Units.largeSpacing

            // Icon, name, author, rating
            RowLayout {
                id: infoIconNameAuthorRatingLayout
                spacing: Kirigami.Units.gridUnit

                // Icon
                Kirigami.Icon {
                    id: appIcon
                    implicitWidth: Kirigami.Units.iconSizes.huge
                    implicitHeight: Kirigami.Units.iconSizes.huge
                    source: fullComponent.application.icon
                }

                // Name, author, rating
                ColumnLayout {
                    spacing: 0

                    // Name
                    Kirigami.Heading {
                        Layout.maximumWidth: Math.ceil(implicitWidth)
                        Layout.fillWidth: true

                        type: Kirigami.Heading.Type.Primary
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        elide: Text.ElideRight

                        text: fullComponent.application.name
                    }

                    // Author or upgrade info, verification
                    RowLayout {
                        Layout.fillWidth: true

                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            id: appAuthor
                            // NOTE: Math.ceil(implicitWidth) is more correct
                            // but causes the layout to break in some cases
                            Layout.maximumWidth: implicitWidth + 1
                            Layout.fillWidth: true

                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            visible: text.length > 0

                            text: {
                                if (fullComponent.isOfflineUpgrade) {
                                    return fullComponent.application.upgradeText.length > 0 ? fullComponent.application.upgradeText : "";
                                } else if (fullComponent.application.author.length > 0) {
                                    return fullComponent.application.author;
                                } else {
                                    return i18nc("As in 'this app is made by an unknown author'", "Unknown author");
                                }
                            }
                        }

                        Kirigami.ContextualHelpButton {
                            id: appVerification
                            Layout.fillHeight: true
                            Layout.maximumHeight: appAuthor.height

                            visible: toolTipText.length > 0
                            icon.name: fullComponent.application.verifiedIconName
                            toolTipText: fullComponent.application.verifiedMessage

                            transform: Translate {
                                // NOTE: Keeps us nice and tight with the actual end of
                                // the text (where a large word is potentially wrapped)
                                // without affecting layout
                                x: -Math.floor(appAuthor.width - appAuthor.paintedWidth)
                            }
                        }
                    }

                    // Rating
                    RowLayout {
                        visible: !fullComponent.isTechnicalPackage

                        Rating {
                            value: fullComponent.application.rating.rating
                            starSize: appAuthor.font.pointSize
                            precision: Rating.Precision.HalfStar
                        }

                        QQC2.Label {
                            text: fullComponent.application.rating ? i18ncp("@info as in the number of ratings an application has", "%1 rating", "%1 ratings", fullComponent.application.rating.ratingCount) : i18nc("@info", "No ratings yet")
                        }
                    }
                }
            }

            // Install button
            RowLayout {
                spacing: Kirigami.Units.gridUnit

                Item {
                    implicitWidth: appIcon.width
                }

                InstallApplicationButton {
                    application: fullComponent.application
                    buttonActiveFocusOnTab: true
                    availableFromOnlySingleSource: fullComponent.availableFromOnlySingleSource
                    hideInvokeButton: false
                }
            }
        }

        // Metadata (version, size, licenses, content rating)
        GridLayout {
            id: metaLayout
            Layout.alignment: fullComponentLayout.stackedMode ? Qt.AlignHCenter : Qt.AlignRight

            readonly property real labelsWidth: Math.max(appVersionLabel.width,
                                                         appSizeLabel.width,
                                                         appLicensesLabel.width,
                                                         appContentRatingLabel.width)

            // Not using Kirigami.FormLayout here because we never
            // want mobile mode and we want control over spacing

            columnSpacing: Kirigami.Units.smallSpacing
            rowSpacing: 0
            columns: 2
            rows: Math.ceil(metaLayout.visibleChildren.count / 2)

            // Not relevant to offline updates
            visible: !fullComponent.isOfflineUpgrade

            // Version
            QQC2.Label {
                id: appVersionLabel
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                text: i18nc("@info", "Version:")
                visible: fullComponent.application.versionString.length > 0
            }

            QQC2.Label {
                Layout.maximumWidth: Math.ceil(implicitWidth)
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: fullComponent.application.versionString
                visible: fullComponent.application.versionString.length > 0
            }

            // Size
            QQC2.Label {
                id: appSizeLabel
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                text: i18n("Size:")
                visible: fullComponent.application.sizeDescription.length > 0
            }

            QQC2.BusyIndicator {
                implicitWidth: Kirigami.Units.iconSizes.sizeForLabels
                implicitHeight: Kirigami.Units.iconSizes.sizeForLabels
                visible: !appSize.visible && fullComponent.application.sizeDescription.length > 0
                running: visible
            }

            QQC2.Label {
                id: appSize
                Layout.maximumWidth: Math.ceil(implicitWidth)
                Layout.fillWidth: true

                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                visible: fullComponent.application.size != 0 && fullComponent.application.sizeDescription.length > 0

                text: fullComponent.application.sizeDescription
            }

            // Licenses
            QQC2.Label {
                id: appLicensesLabel
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                text: i18ncp("@info", "License:", "Licenses:", fullComponent.application.licenses.length)
            }

            ColumnLayout {
                id: appLicenseLayout

                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    visible: fullComponent.application.licenses.length === 0

                    QQC2.Label {
                        id: appLicenseUnknownLabel
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight

                        color: fullComponent.colorForLicenseType("unknown")
                        text: i18nc("The app does not provide any licenses", "Unknown")
                    }

                    QQC2.Button {
                        implicitHeight: appLicenseUnknownLabel.height

                        icon.name: "help-contextual-symbolic"
                        text: i18nc("@info:tooltip for button opening license type description", "What does this mean?")
                        display: QQC2.AbstractButton.IconOnly
                        flat: true

                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: hovered || activeFocus
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                        onClicked: fullComponent.openLicenseDetailsDialog("unknown")
                    }
                }

                Repeater {
                    visible: fullComponent.application.licenses.length > 0
                    model: fullComponent.application.licenses.slice(0, 2)
                    delegate: RowLayout {
                        id: delegate

                        required property var modelData
                        required property int index

                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.UrlButton {
                            id: licenseDelegateUrlButton
                            Layout.fillWidth: true
                            Layout.maximumWidth: Math.min(implicitWidth + 1, fullComponentLayout.width
                                                                             - metaLayout.labelsWidth
                                                                             - (licenseDelegateButton.visible ? licenseDelegateButton.width : 0))

                            // Override some things to keep the right appearance for non-free licenses with no URL
                            readonly property bool hasUrl: url !== ""
                            enabled: true
                            font.underline: hasUrl
                            acceptedButtons: hasUrl ? Qt.LeftButton : Qt.NoButton
                            mouseArea.cursorShape: hasUrl ? Qt.PointingHandCursor : undefined

                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            normalColor: fullComponent.colorForLicenseType(delegate.modelData.licenseType)

                            text: delegate.modelData.name
                            url: delegate.modelData.url
                        }

                        // Button to open the license details dialog
                        QQC2.Button {
                            id: licenseDelegateButton
                            implicitHeight: licenseDelegateUrlButton.height

                            visible: delegate.modelData.licenseType === "unknown"
                                     || delegate.modelData.licenseType === "non-free"
                                     || delegate.modelData.licenseType === "proprietary"

                            icon.name: "help-contextual-symbolic"
                            text: i18nc("@info:tooltip for button opening license type description", "What does this mean?")
                            display: QQC2.AbstractButton.IconOnly
                            flat: true

                            QQC2.ToolTip.text: text
                            QQC2.ToolTip.visible: hovered || activeFocus
                            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                            transform: Translate {
                                // NOTE: Keeps us nice and tight with the actual end of
                                // the text (where a large word is potentially wrapped)
                                // without affecting layout
                                x: -Math.floor(licenseDelegateUrlButton.width - licenseDelegateUrlButton.paintedWidth)
                            }

                            onClicked: fullComponent.openLicenseDetailsDialog(delegate.modelData.licenseType)
                        }
                    }
                }

                // "See All licenses" link, in case there are a lot of them
                Kirigami.LinkButton {
                    visible: application.licenses.length > 3
                    text: i18nc("Show all licenses of the package", "All licenses…")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignTop
                    elide: Text.ElideRight
                    onClicked: fullComponent.openAllLicensesSheet()
                }
            }

            // Content rating
            QQC2.Label {
                id: appContentRatingLabel
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                visible: !fullComponent.isTechnicalPackage
                text: i18nc("@info The app is suitable for people of the following ages or older", "Ages:")
            }

            RowLayout {
                Layout.fillWidth: true

                visible: !fullComponent.isTechnicalPackage

                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    id: appContentRating
                    Layout.maximumWidth: Math.ceil(implicitWidth)
                    Layout.fillWidth: true

                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight

                    text: application.contentRatingMinimumAge === 0 ? i18nc("@item As in, the app is suitable for everyone", "Everyone")
                                                                    : i18nc("@item %1 is a person's age in number of years",
                                                                            "%1+", fullComponent.application.contentRatingMinimumAge)
                }

                QQC2.Button {
                    implicitHeight: appContentRating.height

                    visible: fullComponent.application.contentRatingDescription.length > 0

                    icon.name: "help-contextual-symbolic"
                    text: i18nc("@action:button See content rating details", "See details")
                    display: QQC2.AbstractButton.IconOnly
                    flat: true

                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered || activeFocus
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                    onClicked: fullComponent.openContentRatingDialog()
                }
            }
        }
    }
}
