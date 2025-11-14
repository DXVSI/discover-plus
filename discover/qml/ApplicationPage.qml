/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.purpose as Purpose

DiscoverPage {
    id: appInfo

    title: "" // It would duplicate the text in the header right below it
    clip: true

    required property Discover.AbstractResource application

    readonly property int visibleReviews: 3
    readonly property int internalSpacings: padding * 2
    readonly property bool availableFromOnlySingleSource: !originsMenuAction.visible

    // Usually this page is not the top level page, but when we are, isHome being
    // true will ensure that the search field suggests we are searching in the list
    // of available apps, not inside the app page itself. This will happen when
    // Discover is launched e.g. from krunner or otherwise requested to show a
    // specific application on launch.
    readonly property bool isHome: true

    readonly property bool isOfflineUpgrade: application.packageName === "discover-offline-upgrade"

    readonly property bool isTechnicalPackage: application.type == Discover.AbstractResource.ApplicationSupport
                                            || application.type == Discover.AbstractResource.System

    readonly property int smallButtonSize: Kirigami.Units.iconSizes.small + (Kirigami.Units.smallSpacing * 2)

    function colorForLicenseType(licenseType: string): string {
        switch(licenseType) {
            case "free":
                return Kirigami.Theme.positiveTextColor;
            case "non-free":
                return Kirigami.Theme.neutralTextColor;
            case "proprietary":
                return Kirigami.Theme.negativeTextColor
            case "unknown":
            default:
                return Kirigami.Theme.neutralTextColor;
        }
    }

    function explanationForLicenseType(licenseType: string): string {
        let freeSoftwareUrl = "https://www.gnu.org/philosophy/free-sw.html"
        let fsfUrl = "https://www.fsf.org/"
        let osiUrl = "https://opensource.org/"
        let proprietarySoftwareUrl = "https://www.gnu.org/proprietary"
        let hasHomepageUrl = application.homepage.toString().length > 0

        switch(licenseType) {
            case "proprietary":
                if (hasHomepageUrl) {
                    return xi18nc("@info", "Only install %1 if you fully trust its authors because it is <emphasis strong='true'>proprietary</emphasis>: Your freedom to use, modify, and redistribute this application is restricted, and its source code is partially or entirely closed to public inspection and improvement. This means third parties and users like you cannot verify its operation, security, and trustworthiness.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways — such as harvesting your personal information, tracking your location, or transmitting the contents of your data to someone else. Only use it if you fully trust its authors. More information may be available on <link url='%2'>the application's website</link>.<nl/><nl/>Learn more at <link url='%3'>%3</link>.",
                                appInfo.application.name,
                                appInfo.application.homepage.toString(),
                                proprietarySoftwareUrl)
                } else {
                    return xi18nc("@info", "Only install %1 if you fully trust its authors because it is <emphasis strong='true'>proprietary</emphasis>: Your freedom to use, modify, and redistribute this application is restricted, and its source code is partially or entirely closed to public inspection and improvement. This means third parties and users like you cannot verify its operation, security, and trustworthiness.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways — such as harvesting your personal information, tracking your location, or transmitting the contents of your data to someone else. Only use it if you fully trust its authors. Learn more at <link url='%2'>%2</link>.",
                                  appInfo.application.name,
                                  proprietarySoftwareUrl)
                }

            case "non-free":
                if (hasHomepageUrl) {
                    return xi18nc("@info", "%1 uses one or more licenses not certified as “Free Software” by either the <link url='%2'>Free Software Foundation</link> or the <link url='%3'>Open Source Initiative</link>. This means your freedom to use, study, modify, and share it may be restricted in some ways.<nl/><nl/>Make sure to read the license text and understand any restrictions before using the software.<nl/><nl/>If the license does not even grant access to read the source code, make sure you fully trust the authors, as no one else can verify the trustworthiness and security of its code to ensure that it is not acting against you in hidden ways. More information may be available on <link url='%4'>the application's website</link>.<nl/><nl/>Learn more at <link url='%5'>%5</link>.",
                                appInfo.application.name,
                                fsfUrl,
                                osiUrl,
                                appInfo.application.homepage.toString(),
                                freeSoftwareUrl);
                } else {
                    return xi18nc("@info", "%1 uses one or more licenses not certified as “Free Software” by either the <link url='%2'>Free Software Foundation</link> or the <link url='%3'>Open Source Initiative</link>. This means your freedom to use, study, modify, and share it may be restricted in some ways.<nl/><nl/>Make sure to read the license text and understand any restrictions before using the software.<nl/><nl/>If the license does not even grant access to read the source code, make sure you fully trust the authors, as no one else can verify the trustworthiness and security of its code to ensure that it is not acting against you in hidden ways.<nl/><nl/>Learn more at <link url='%4'>%4</link>.",
                                  appInfo.application.name,
                                  fsfUrl,
                                  osiUrl,
                                  freeSoftwareUrl);
                }

            case "unknown":
                if (hasHomepageUrl) {
                    return xi18nc("@info", "%1 does not indicate under which license it is distributed. You may be able to determine this on <link url='%2'>the application's website</link>. Find it there or contact the author if you want to use this application for anything other than private personal use.",
                                 appInfo.application.name,
                                 appInfo.application.homepage.toString());
                } else {
                    return i18nc("@info", "%1 does not indicate under which license it is distributed. Contact the application's author if you want to use it for anything other than private personal use.",
                                 appInfo.application.name);
                }

            case "free":
            default:
                return "";
        }
    }

    Discover.TransactionListener {
        id: transactionListener
        resource: appInfo.application
    }

    ReviewsPage {
        id: reviewsSheet
        parent: appInfo.QQC2.Overlay.overlay
        model: Discover.ReviewsModel {
            id: reviewsModel
            resource: appInfo.application
            preferredSortRole: reviewsSheet.sortRole
        }
        Component.onCompleted: reviewsSheet.sortRole = reviewsModel.preferredSortRole
    }

    actions: []  // Disable default actions, we'll add custom buttons

    QQC2.ActionGroup {
        id: sourcesGroup
        exclusive: true
    }

    Kirigami.Action {
        id: shareAction
        text: i18nc("@action:button share a link to this app", "Share")
        icon.name: "document-share"
        visible: application.url.toString().length > 0 && !appInfo.isTechnicalPackage
        onTriggered: shareSheet.open()
    }

    Kirigami.Action {
        id: addonsAction
        text: i18nc("@action:button", "Add-ons")
        icon.name: "extension-symbolic"
        visible: addonsView.containsAddons
        onTriggered: {
            if (addonsView.addonsCount === 0) {
                Navigation.openExtends(application.appstreamId, appInfo.application.name)
            } else {
                addonsView.visible = true
            }
        }
    }

    // Multi-source origin display and switcher
    Kirigami.Action {
        id: originsMenuAction

        text: i18nc("@item:inlistbox %1 is the name of an app source e.g. \"Flathub\" or \"Ubuntu\"", "From %1", appInfo.application.displayOrigin)
        visible: children.length > 1 && !transactionListener.isActive
        children: sourcesGroup.actions
    }

    Instantiator {
        // alternativeResourcesModel
        model: Discover.ResourcesProxyModel {
            allBackends: true
            resourcesUrl: appInfo.application.url
        }
        delegate: QQC2.Action {
            required property var model

            QQC2.ActionGroup.group: sourcesGroup
            text: model.availableVersion
                ? i18n("%1 - %2", model.displayOrigin, model.availableVersion)
                : model.displayOrigin
            icon.name: model.sourceIcon
            checkable: true
            checked: appInfo.application === model.application
            onTriggered: {
                appInfo.application = model.application
            }
        }
    }

    Kirigami.Action {
        id: invokeAction
        visible: application.isInstalled && application.canExecute && !transactionListener.isActive
        text: application.executeLabel
        icon.name: "media-playback-start-symbolic"
        onTriggered: application.invokeApplication()
    }

    Kirigami.Action {
        id: installRemoveAndProgressAction
        displayComponent: InstallApplicationButton {
            id: appbutton
            application: appInfo.application
            buttonActiveFocusOnTab: true
            availableFromOnlySingleSource: appInfo.availableFromOnlySingleSource
            flat: true
        }
    }

    Kirigami.ImageColors {
        id: appImageColorExtractor
        source: appInfo.application.icon
    }

    Kirigami.PromptDialog {
        id: shareSheet
        parent: applicationWindow().overlay
        implicitWidth: Kirigami.Units.gridUnit * 20
        title: i18nc("@title:window", "Share Link to Application")
        standardButtons: QQC2.Dialog.NoButton

        Purpose.AlternativesView {
            id: alts
            Layout.fillWidth: true
            pluginType: "ShareUrl"
            inputData: {
                "urls": [ application.url.toString() ],
                "title": i18nc("The subject line for an email. %1 is the name of an application", "Check out the %1 app!", application.name)
            }
            onFinished: {
                shareSheet.close()
                if (error !== 0) {
                    console.error("job finished with error", error, message)
                }
                alts.reset()
            }
        }
    }

    // Scrollable page content
    ColumnLayout {
        id: pageLayout

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        spacing: appInfo.internalSpacings

        // Modern colored header with app icon, name, and metadata
        Rectangle {
            Layout.fillWidth: true

            // Undo page paddings so that the header touches the edges. We don't
            // want it to actually be in the header: area since then it wouldn't
            // scroll away, which we do want.
            Layout.topMargin: -appInfo.topPadding
            Layout.leftMargin: -appInfo.leftPadding
            Layout.rightMargin: -appInfo.rightPadding

            implicitHeight: headerLayout.implicitHeight + (headerLayout.anchors.topMargin * 2)

            // Modern gradient background
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(appImageColorExtractor.dominant.r,
                                  appImageColorExtractor.dominant.g,
                                  appImageColorExtractor.dominant.b, 0.15)
                }
                GradientStop {
                    position: 0.5
                    color: Qt.rgba(appImageColorExtractor.dominant.r,
                                  appImageColorExtractor.dominant.g,
                                  appImageColorExtractor.dominant.b, 0.08)
                }
                GradientStop {
                    position: 1.0
                    color: Kirigami.Theme.backgroundColor
                }
            }

            // Subtle border at bottom
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                              Kirigami.Theme.textColor.b, 0.1)
            }

            GridLayout {
                id: headerLayout

                readonly property bool hasMetadata: appMetadataLayout.visible
                readonly property int effectiveBasicInfoWidth: appBasicInfoLayout.implicitWidth
                readonly property int effectiveMetadataWidth: hasMetadata ? appMetadataLayout.implicitWidth + columnSpacing : 0
                readonly property int effectiveSpaceAvailable: pageLayout.width - anchors.leftMargin - anchors.rightMargin

                readonly property bool stackedMode: hasMetadata && (effectiveBasicInfoWidth + effectiveMetadataWidth > effectiveSpaceAvailable)

                columns: stackedMode || !hasMetadata ? 1 : 2
                rows: stackedMode ? 2 : 1
                columnSpacing: 0
                rowSpacing: appInfo.padding

                anchors {
                    top: parent.top
                    topMargin: appInfo.padding
                    left: parent.left
                    leftMargin: appInfo.padding
                    right: parent.right
                    rightMargin: appInfo.padding
                }


                // App icon, name, author, and rating
                RowLayout {
                    id: appBasicInfoLayout
                    Layout.fillWidth: true
                    Layout.maximumWidth: headerLayout.stackedMode ? headerLayout.implicitWidth : -1
                    Layout.alignment: headerLayout.stackedMode ? Qt.AlignHCenter : Qt.AlignLeft
                    spacing: appInfo.padding

                    // App icon with modern styling
                    Item {
                        implicitWidth: Kirigami.Units.iconSizes.huge + Kirigami.Units.largeSpacing
                        implicitHeight: implicitWidth

                        // Icon background with gradient
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width
                            height: width
                            radius: Kirigami.Units.largeSpacing

                            gradient: Gradient {
                                GradientStop {
                                    position: 0.0
                                    color: Qt.rgba(1, 1, 1, 0.1)
                                }
                                GradientStop {
                                    position: 1.0
                                    color: Qt.rgba(1, 1, 1, 0.05)
                                }
                            }

                            border.width: 1
                            border.color: Qt.rgba(1, 1, 1, 0.2)

                            // Shadow effect
                            layer.enabled: true
                            layer.effect: MultiEffect {
                                shadowEnabled: true
                                shadowHorizontalOffset: 0
                                shadowVerticalOffset: 2
                                shadowBlur: 0.5
                                shadowOpacity: 0.2
                                shadowColor: Qt.rgba(0, 0, 0, 0.8)
                            }

                            Kirigami.Icon {
                                anchors.centerIn: parent
                                implicitWidth: Kirigami.Units.iconSizes.huge
                                implicitHeight: Kirigami.Units.iconSizes.huge
                                source: appInfo.application.icon
                            }
                        }
                    }

                    // App name, author, and rating
                    ColumnLayout {

                        spacing: 0

                        // App name
                        Kirigami.Heading {
                            Layout.fillWidth: true
                            text: appInfo.application.name
                            type: Kirigami.Heading.Type.Primary
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }

                        // Author (for apps) or upgrade info (for offline upgrades). Verification check
                        RowLayout {
                            Layout.fillWidth: true

                            QQC2.Label {
                                id: author

                                visible: text.length > 0

                                text: {
                                    if (appInfo.isOfflineUpgrade) {
                                        return appInfo.application.upgradeText.length > 0 ? appInfo.application.upgradeText : "";
                                    } else if (appInfo.application.author.length > 0) {
                                        return appInfo.application.author;
                                    } else {
                                        return i18n("Unknown author");
                                    }
                                }
                                wrapMode: Text.Wrap
                                maximumLineCount: 5
                                elide: Text.ElideRight
                            }

                            Kirigami.Icon {
                                visible: verifiedTooltip.QQC2.ToolTip.text.length > 0
                                source: appInfo.application.verifiedIconName
                                Layout.maximumHeight: author.contentHeight
                                Layout.fillHeight: true

                                QQC2.Control {
                                    id: verifiedTooltip
                                    anchors.fill: parent

                                    QQC2.ToolTip.text: appInfo.application.verifiedMessage
                                    QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? pressed : hovered) && QQC2.ToolTip.text !== ""
                                    QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
                                }
                            }
                        }

                        // Rating
                        RowLayout {
                            visible: !appInfo.isTechnicalPackage

                            Rating {
                                value: appInfo.application.rating.sortableRating
                                starSize: author.font.pointSize
                                precision: Rating.Precision.HalfStar
                            }

                            QQC2.Label {
                                text: appInfo.application.rating ? i18np("%1 rating", "%1 ratings", appInfo.application.rating.ratingCount) : i18n("No ratings yet")
                            }
                        }
                    }
                }

                // Metadata with modern card style
                Rectangle {
                    id: appMetadataCard

                    Layout.alignment: headerLayout.stackedMode ? Qt.AlignHCenter : Qt.AlignRight
                    Layout.preferredWidth: appMetadataLayout.implicitWidth + Kirigami.Units.largeSpacing * 2
                    Layout.preferredHeight: appMetadataLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
                    Layout.topMargin: 70 // Push metadata below buttons

                    radius: Kirigami.Units.smallSpacing
                    color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.05)
                    border.width: 1
                    border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

                    // Not relevant to offline updates
                    visible: !appInfo.isOfflineUpgrade

                    GridLayout {
                        id: appMetadataLayout

                        anchors.centerIn: parent

                        columns: 2
                        rows: Math.ceil(appMetadataLayout.visibleChildren.count / 2)
                        columnSpacing: Kirigami.Units.largeSpacing
                        rowSpacing: Kirigami.Units.smallSpacing

                    // Version
                    QQC2.Label {
                        text: i18n("Version:")
                        Layout.alignment: Qt.AlignRight
                    }
                    QQC2.Label {
                        text: appInfo.application.versionString
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }

                    // Size
                    QQC2.Label {
                        text: i18n("Size:")
                        Layout.alignment: Qt.AlignRight
                    }
                    QQC2.Label {
                        text: appInfo.application.sizeDescription
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }

                    // Licenses
                    QQC2.Label {
                        text: i18np("License:", "Licenses:", appInfo.application.licenses.length)
                        Layout.alignment: Qt.AlignRight
                    }
                    RowLayout {
                        id: licenseRowLayout
                        spacing: Kirigami.Units.smallSpacing

                        readonly property string infoButtonToolTipText: i18nc("@info:tooltip for button opening license type description", "What does this mean?")

                        QQC2.Label {
                            visible : appInfo.application.licenses.length === 0
                            text: i18nc("The app does not provide any licenses", "Unknown")
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                            color: appInfo.colorForLicenseType("unknown")
                        }


                        // Button to open the license details dialog if license is empty
                        QQC2.Button {
                            Layout.preferredWidth: appInfo.smallButtonSize
                            Layout.preferredHeight: appInfo.smallButtonSize
                            visible : appInfo.application.licenses.length === 0
                            icon.name: "help-contextual"
                            onClicked: licenseDetailsDialog.openWithLicenseType("unknown");

                            QQC2.ToolTip {
                                text: licenseRowLayout.infoButtonToolTipText
                            }
                        }

                        Repeater {
                            visible: appInfo.application.licenses.length > 0
                            model: appInfo.application.licenses.slice(0, 2)
                            delegate: RowLayout {
                                id: delegate

                                required property var modelData
                                required property int index

                                spacing: Kirigami.Units.smallSpacing

                                RowLayout {
                                    spacing: 0
                                    Kirigami.UrlButton {
                                        // Override some things to keep the right appearance for non-free licenses with no URL.
                                        readonly property bool hasUrl: url !== ""
                                        enabled: true
                                        font.underline: hasUrl
                                        acceptedButtons: hasUrl ? Qt.LeftButton : Qt.NoButton
                                        mouseArea.cursorShape: hasUrl ? Qt.PointingHandCursor : undefined

                                        text: delegate.modelData.name
                                        url: delegate.modelData.url
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignTop
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 3
                                        elide: Text.ElideRight
                                        normalColor: appInfo.colorForLicenseType(delegate.modelData.licenseType)
                                    }
                                    QQC2.Label {
                                        readonly property int licensesCount: appInfo.application.licenses.length
                                        text: i18nc("Separator between license labels e.g. 'GPL-3.0, Proprietary'", ",")
                                        visible: delegate.index <= (licensesCount - 2)
                                    }
                                }

                                // Button to open the license details dialog
                                QQC2.Button {
                                    Layout.preferredWidth: appInfo.smallButtonSize
                                    Layout.preferredHeight: appInfo.smallButtonSize
                                    visible: delegate.modelData.licenseType === "unknown"
                                          || delegate.modelData.licenseType === "non-free"
                                          || delegate.modelData.licenseType === "proprietary"
                                    icon.name: "help-contextual"
                                    onClicked: licenseDetailsDialog.openWithLicenseType(delegate.modelData.licenseType);

                                    QQC2.ToolTip {
                                        text: i18n(licenseRowLayout.infoButtonToolTipText)
                                    }
                                }
                            }
                        }

                        // "See More licenses" link, in case there are a lot of them
                        Kirigami.LinkButton {
                            visible: application.licenses.length > 3
                            text: i18nc("Show all licenses of the package", "See more…")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            elide: Text.ElideRight
                            onClicked: allLicensesSheet.open();
                        }
                    }

                    // Content Rating
                    QQC2.Label {
                        Layout.alignment: Qt.AlignRight
                        visible: !appInfo.isTechnicalPackage
                        text: i18nc("@label The app is suitable for people of the following ages or older", "Ages:")
                    }
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        visible: !appInfo.isTechnicalPackage

                        QQC2.Label {
                            text: application.contentRatingMinimumAge === 0
                                ? i18nc("@item As in, the app is suitable for everyone", "Everyone")
                                : i18nc("@item %1 is a person's age in number of years",
                                        "%1+", appInfo.application.contentRatingMinimumAge)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }

                        // Button to open the content rating details dialog
                        QQC2.Button {
                            Layout.preferredWidth: appInfo.smallButtonSize
                            Layout.preferredHeight: appInfo.smallButtonSize
                            visible: appInfo.application.contentRatingDescription.length > 0
                            icon.name: "help-contextual"
                            text: i18n("See details")
                            display: QQC2.AbstractButton.IconOnly
                            onClicked: contentRatingDialog.open()

                            QQC2.ToolTip {
                                text: parent.text
                            }
                        }
                    }
                    } // GridLayout end
                } // Rectangle end

                // Modern action buttons row
                RowLayout {
                    anchors {
                        right: parent.right
                        top: parent.top
                        margins: appInfo.padding
                    }
                    spacing: Kirigami.Units.smallSpacing

                    // Add-ons button
                    Rectangle {
                        visible: addonsView.containsAddons
                        width: addonsBtn.width + Kirigami.Units.largeSpacing * 2
                        height: addonsBtn.height + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(0.5, 0.3, 0.7, 0.15) }
                            GradientStop { position: 1.0; color: Qt.rgba(0.5, 0.3, 0.7, 0.25) }
                        }
                        border.width: 1
                        border.color: Qt.rgba(0.5, 0.3, 0.7, 0.3)

                        QQC2.Button {
                            id: addonsBtn
                            anchors.centerIn: parent
                            flat: true
                            text: i18nc("@action:button", "Add-ons")
                            icon.name: "extension-symbolic"
                            icon.color: "#8e44ad"
                            onClicked: {
                                if (addonsView.addonsCount === 0) {
                                    Navigation.openExtends(application.appstreamId, appInfo.application.name)
                                } else {
                                    addonsView.visible = true
                                }
                            }
                        }
                    }

                    // Source/Origin button (if multiple sources available)
                    Rectangle {
                        visible: sourcesGroup.actions.length > 1 && !transactionListener.isActive
                        width: sourceBtn.width + Kirigami.Units.largeSpacing * 2
                        height: sourceBtn.height + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(0.9, 0.5, 0.1, 0.15) }
                            GradientStop { position: 1.0; color: Qt.rgba(0.9, 0.5, 0.1, 0.25) }
                        }
                        border.width: 1
                        border.color: Qt.rgba(0.9, 0.5, 0.1, 0.3)

                        QQC2.Button {
                            id: sourceBtn
                            anchors.centerIn: parent
                            flat: true
                            text: i18nc("@item:inlistbox %1 is the name of an app source e.g. \"Flathub\" or \"Ubuntu\"", "From %1", appInfo.application.displayOrigin)
                            icon.name: appInfo.application.sourceIcon
                            icon.color: "#e67e22"
                            onClicked: sourceMenu.popup()

                            QQC2.Menu {
                                id: sourceMenu
                                Instantiator {
                                    model: Discover.ResourcesProxyModel {
                                        allBackends: true
                                        resourcesUrl: appInfo.application.url
                                    }
                                    delegate: QQC2.MenuItem {
                                        required property var model
                                        text: model.availableVersion
                                            ? i18n("%1 - %2", model.displayOrigin, model.availableVersion)
                                            : model.displayOrigin
                                        icon.name: model.sourceIcon
                                        checkable: true
                                        checked: appInfo.application === model.application
                                        onTriggered: {
                                            appInfo.application = model.application
                                        }
                                    }
                                    onObjectAdded: (index, object) => sourceMenu.addItem(object)
                                    onObjectRemoved: (index, object) => sourceMenu.removeItem(object)
                                }
                            }
                        }
                    }

                    // Share button
                    Rectangle {
                        visible: application.url.toString().length > 0 && !appInfo.isTechnicalPackage
                        width: shareBtn.width + Kirigami.Units.largeSpacing * 2
                        height: shareBtn.height + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.6, 1, 0.15) }
                            GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.6, 1, 0.25) }
                        }
                        border.width: 1
                        border.color: Qt.rgba(0.2, 0.6, 1, 0.3)

                        QQC2.Button {
                            id: shareBtn
                            anchors.centerIn: parent
                            flat: true
                            text: i18nc("@action:button share a link to this app", "Share")
                            icon.name: "document-share"
                            icon.color: "#3498db"
                            onClicked: shareSheet.open()
                        }
                    }

                    // Launch button
                    Rectangle {
                        visible: application.isInstalled && application.canExecute && !transactionListener.isActive
                        width: launchBtn.width + Kirigami.Units.largeSpacing * 2
                        height: launchBtn.height + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.7, 0.3, 0.15) }
                            GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.7, 0.3, 0.25) }
                        }
                        border.width: 1
                        border.color: Qt.rgba(0.2, 0.7, 0.3, 0.3)

                        QQC2.Button {
                            id: launchBtn
                            anchors.centerIn: parent
                            flat: true
                            text: application.executeLabel
                            icon.name: "media-playback-start-symbolic"
                            icon.color: "#27ae60"
                            onClicked: application.invokeApplication()
                        }
                    }

                    // Install/Remove button
                    Rectangle {
                        // Hide for system updates (offline upgrades)
                        visible: !appInfo.isOfflineUpgrade
                        width: installRemoveBtn.implicitWidth + Kirigami.Units.largeSpacing * 2
                        height: installRemoveBtn.implicitHeight + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: {
                                    if (transactionListener.isActive) {
                                        return Qt.rgba(0.9, 0.6, 0.2, 0.15)
                                    } else if (application.isInstalled) {
                                        return Qt.rgba(0.8, 0.3, 0.3, 0.15)
                                    } else {
                                        return Qt.rgba(0.2, 0.7, 0.3, 0.15)
                                    }
                                }
                            }
                            GradientStop {
                                position: 1.0
                                color: {
                                    if (transactionListener.isActive) {
                                        return Qt.rgba(0.9, 0.6, 0.2, 0.25)
                                    } else if (application.isInstalled) {
                                        return Qt.rgba(0.8, 0.3, 0.3, 0.25)
                                    } else {
                                        return Qt.rgba(0.2, 0.7, 0.3, 0.25)
                                    }
                                }
                            }
                        }
                        border.width: 1
                        border.color: {
                            if (transactionListener.isActive) {
                                return Qt.rgba(0.9, 0.6, 0.2, 0.3)
                            } else if (application.isInstalled) {
                                return Qt.rgba(0.8, 0.3, 0.3, 0.3)
                            } else {
                                return Qt.rgba(0.2, 0.7, 0.3, 0.3)
                            }
                        }

                        InstallApplicationButton {
                            id: installRemoveBtn
                            anchors.centerIn: parent
                            application: appInfo.application
                            flat: true
                            availableFromOnlySingleSource: appInfo.availableFromOnlySingleSource
                        }
                    }

                    // System update info button (for offline upgrades)
                    Rectangle {
                        visible: appInfo.isOfflineUpgrade
                        width: updateInfoBtn.width + Kirigami.Units.largeSpacing * 2
                        height: updateInfoBtn.height + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.6, 1, 0.15) }
                            GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.6, 1, 0.25) }
                        }
                        border.width: 1
                        border.color: Qt.rgba(0.2, 0.6, 1, 0.3)

                        QQC2.Button {
                            id: updateInfoBtn
                            anchors.centerIn: parent
                            flat: true
                            text: i18n("Update Ready")
                            icon.name: "system-software-update"
                            icon.color: "#3498db"
                            enabled: false // Just informational

                            QQC2.ToolTip {
                                text: i18n("System update is ready to be installed")
                            }
                        }
                    }
                }
            }

            Kirigami.Separator {
                width: parent.width
                anchors.top: parent.bottom
            }
        }

        // Screenshots
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true

            visible: carousel.hasFailed
            icon.name: "image-missing"
            text: i18nc("@info placeholder message", "Screenshots not available for %1", appInfo.application.name)
        }

        CarouselInlineView {
            id: carousel

            Layout.fillWidth: true
            // Undo page paddings so that the header touches the edges. We don't
            // want it to actually be in the header: area since then it wouldn't
            // scroll away, which we do want.
            Layout.leftMargin: -appInfo.leftPadding
            Layout.rightMargin: -appInfo.rightPadding
            // This roughly replicates scaling formula for the screenshots
            // gallery on FlatHub website, adjusted to scale with gridUnit
            Layout.minimumHeight: Math.round((16 + 1/9) * Kirigami.Units.gridUnit)
            Layout.maximumHeight: 30 * Kirigami.Units.gridUnit
            Layout.preferredHeight: Math.round(width / 2) + Math.round((2 + 7/9) * Kirigami.Units.gridUnit)

            edgeMargin: appInfo.padding
            visible: carouselModel.count > 0 && !hasFailed && !appInfo.isTechnicalPackage

            carouselModel: Discover.ScreenshotsModel {
                application: appInfo.application
            }
        }

        ColumnLayout {
            id: topObjectsLayout

            // InlineMessage components are supposed to manage their spacing
            // internally. However, at least for now they require some
            // assistance from outside to stack them one after another.
            spacing: 0

            Layout.fillWidth: true

            // Cancel out parent layout's spacing, making this component effectively zero-sized when empty.
            // When non-empty, the very first top margin is provided by this layout, but bottom margins
            // are implemented by Loaders that have visible loaded items.
            Layout.topMargin: hasActiveObjects ? 0 : -pageLayout.spacing
            Layout.bottomMargin: -pageLayout.spacing

            property bool hasActiveObjects: false
            visible: hasActiveObjects

            function bindVisibility() {
                hasActiveObjects = Qt.binding(() => {
                    for (let i = 0; i < topObjectsRepeater.count; i++) {
                        const loader = topObjectsRepeater.itemAt(i);
                        const item = loader.item;
                        if (item?.Discover.Activatable.active) {
                            return true;
                        }
                    }
                    return false;
                });
            }

            Timer {
                id: bindActiveTimer

                running: false
                repeat: false
                interval: 0

                onTriggered: topObjectsLayout.bindVisibility()
            }

            Repeater {
                id: topObjectsRepeater

                model: appInfo.application.topObjects

                delegate: Loader {
                    id: topObject
                    required property string modelData

                    Layout.fillWidth: item?.Layout.fillWidth ?? false
                    Layout.topMargin: 0
                    Layout.bottomMargin: item?.Discover.Activatable.active ? appInfo.padding : 0
                    Layout.preferredHeight: item?.Discover.Activatable.active ? item.implicitHeight : 0

                    onModelDataChanged: {
                        setSource(modelData, { resource: Qt.binding(() => appInfo.application) });
                    }
                    Connections {
                        target: topObject.item?.Discover.Activatable
                        function onActiveChanged() {
                            bindActiveTimer.start();
                        }
                    }
                }
                onItemAdded: (index, item) => {
                    bindActiveTimer.start();
                }
                onItemRemoved: (index, item) => {
                    bindActiveTimer.start();
                }
            }
        }

        // App description section with modern card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: descriptionLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.3, 0.5, 0.9, 0.05) }
                GradientStop { position: 0.5; color: Qt.rgba(0.2, 0.4, 0.8, 0.08) }
                GradientStop { position: 1.0; color: Qt.rgba(0.3, 0.5, 0.9, 0.05) }
            }
            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            ColumnLayout {
                id: descriptionLayout
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.smallSpacing

                // Short description
                // Not using Kirigami.Heading here because that component doesn't
                // support selectable text, and we want this to be selectable because
                // it's also used to show the path for local packages, and that makes
                // sense to be selectable
                Kirigami.SelectableLabel {
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentWidth
                    // Not relevant to the offline upgrade use case because we
                    // display the info in the header instead
                    visible: !appInfo.isOfflineUpgrade
                    text: appInfo.application.comment
                    wrapMode: Text.Wrap

                    // Match `level: 1` in Kirigami.Heading
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                    font.weight: Font.DemiBold

                    Accessible.role: Accessible.Heading
                }

                // Long app description
                Kirigami.SelectableLabel {
                    objectName: "applicationDescription" // for appium tests
                    Layout.fillWidth: true
                    Layout.preferredWidth: contentWidth
                    wrapMode: Text.WordWrap
                    text: appInfo.application.longDescription
                    textFormat: TextEdit.RichText
                    onLinkActivated: link => Qt.openUrlExternally(link);
                }
            }
        }

        // Changelog section with modern card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: changelogLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
            visible: changelogLabel.visible
            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.7, 0.3, 0.05) }
                GradientStop { position: 0.5; color: Qt.rgba(0.2, 0.6, 0.3, 0.08) }
                GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.7, 0.3, 0.05) }
            }
            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            ColumnLayout {
                id: changelogLayout
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        width: Kirigami.Units.iconSizes.medium
                        height: width
                        radius: width / 2
                        color: Qt.rgba(0.2, 0.7, 0.3, 0.15)

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            source: "view-refresh"
                            width: parent.width * 0.6
                            height: width
                            color: "#27ae60"
                        }
                    }

                    Kirigami.Heading {
                        text: i18n("🆕 What's New")
                        level: 2
                        type: Kirigami.Heading.Type.Primary
                        wrapMode: Text.Wrap
                    }
                }

                // Changelog text
                QQC2.Label {
                    id: changelogLabel

                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing

                    // Some backends are known to produce empty line break as a text
                    visible: text !== "" && text !== "<br />"
                    wrapMode: Text.WordWrap

                    Component.onCompleted: appInfo.application.fetchChangelog()
                    Connections {
                        target: appInfo.application
                        function onChangelogFetched(changelog) {
                            changelogLabel.text = changelog
                        }
                    }
                }
            }
        }

        // Reviews section with modern card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: reviewsLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
            visible: !appInfo.isTechnicalPackage
            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.9, 0.6, 0.2, 0.05) }
                GradientStop { position: 0.5; color: Qt.rgba(0.8, 0.5, 0.2, 0.08) }
                GradientStop { position: 1.0; color: Qt.rgba(0.9, 0.6, 0.2, 0.05) }
            }
            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            ColumnLayout {
                id: reviewsLayout
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        width: Kirigami.Units.iconSizes.medium
                        height: width
                        radius: width / 2
                        color: Qt.rgba(0.9, 0.6, 0.2, 0.15)

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            source: "dialog-messages"
                            width: parent.width * 0.6
                            height: width
                            color: "#f39c12"
                        }
                    }

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        text: i18n("⭐ Reviews")
                        level: 2
                        type: Kirigami.Heading.Type.Primary
                        wrapMode: Text.Wrap
                    }
                }

            Kirigami.LoadingPlaceholder {
                id: reviewsLoadingPlaceholder
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: Kirigami.Units.gridUnit * 15
                visible: reviewsModel.fetching
                text: i18n("Loading reviews for %1", appInfo.application.name)
            }

            Kirigami.PlaceholderMessage {
                id: reviewsError
                Layout.fillWidth: true
                readonly property bool hasError: reviewsModel.backend && reviewsModel.backend.errorMessage.length > 0 && text.length > 0 && reviewsModel.count === 0 && !reviewsLoadingPlaceholder.visible
                visible: hasError
                icon.name: "text-unflow"
                text: i18nc("@info placeholder message", "Reviews for %1 are temporarily unavailable", appInfo.application.name)
                explanation: reviewsModel.backend ? reviewsModel.backend.errorMessage : ""
            }

            Kirigami.PlaceholderMessage {
                Layout.fillWidth: true
                visible: !reviewsStats.visible && !reviewsLoadingPlaceholder.visible
                text: i18nc("@Info placeholder message", "No reviews posted yet")
                explanation: appInfo.application.isInstalled ? "" : i18nc("@info placeholder explanation", "Install to write a review")
                helpfulAction: Kirigami.Action {
                    enabled: appInfo.application.isInstalled

                    text: appInfo.application.isInstalled ? i18n("Write a Review") : i18n("Install to Write a Review")
                    icon.name: "document-edit"

                    onTriggered: {
                        reviewsSheet.openReviewDialog()
                    }
                }
            }

            ReviewsStats {
                id: reviewsStats
                visible: reviewsModel.count > 0
                Layout.fillWidth: true
                application: appInfo.application
                reviewsModel: reviewsModel
                model: reviewsSheet.model
                visibleReviews: Math.min(reviewsModel.count, appInfo.visibleReviews)
                compact: appInfo.compact
            }

            // Modern review-related buttons
            Flow {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                // Show all reviews button
                Rectangle {
                    visible: reviewsModel.count > visibleReviews
                    width: showAllButton.width
                    height: showAllButton.height
                    radius: Kirigami.Units.smallSpacing
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.6, 1, 0.15) }
                        GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.6, 1, 0.25) }
                    }

                    QQC2.Button {
                        id: showAllButton
                        anchors.centerIn: parent
                        flat: true
                        text: i18nc("@action:button", "Show All Reviews")
                        icon.name: "view-visible"
                        onClicked: reviewsSheet.open()
                    }
                }

                // Write review button
                Rectangle {
                    visible: transactionListener.resource.state !== Discover.AbstractResource.Broken
                          && reviewsModel.backend
                          && !reviewsError.visible
                          && reviewsStats.visible
                          && reviewsModel.backend.isResourceSupported(appInfo.application)
                    width: writeReviewButton.width
                    height: writeReviewButton.height
                    radius: Kirigami.Units.smallSpacing
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: appInfo.application.isInstalled ? Qt.rgba(0.2, 0.8, 0.4, 0.15) : Qt.rgba(0.5, 0.5, 0.5, 0.1)
                        }
                        GradientStop {
                            position: 1.0
                            color: appInfo.application.isInstalled ? Qt.rgba(0.2, 0.8, 0.4, 0.25) : Qt.rgba(0.5, 0.5, 0.5, 0.2)
                        }
                    }

                    QQC2.Button {
                        id: writeReviewButton
                        anchors.centerIn: parent
                        flat: true
                        enabled: appInfo.application.isInstalled
                        text: appInfo.application.isInstalled ? i18n("Write a Review") : i18n("Install to Write a Review")
                        icon.name: "document-edit"
                        onClicked: reviewsSheet.openReviewDialog()
                    }
                }
            }
            } // reviewsLayout end
        } // Rectangle end

        // "External Links" section with modern card
        Rectangle {
            readonly property int visibleButtons: (helpButton.visible ? 1 : 0)
                                                + (homepageButton.visible ? 1: 0)
                                                + (donateButton.visible ? 1 : 0)
                                                + (bugButton.visible ? 1 : 0)
                                                + (contributeButton.visible ? 1 : 0)
            visible: visibleButtons > 0 && !appInfo.isTechnicalPackage

            Layout.fillWidth: true
            Layout.preferredHeight: linksLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.5, 0.3, 0.7, 0.05) }
                GradientStop { position: 0.5; color: Qt.rgba(0.5, 0.3, 0.7, 0.08) }
                GradientStop { position: 1.0; color: Qt.rgba(0.5, 0.3, 0.7, 0.05) }
            }
            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            ColumnLayout {
                id: linksLayout
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        width: Kirigami.Units.iconSizes.medium
                        height: width
                        radius: width / 2
                        color: Qt.rgba(0.5, 0.3, 0.7, 0.15)

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            source: "internet-services"
                            width: parent.width * 0.6
                            height: width
                            color: "#8e44ad"
                        }
                    }

                    Kirigami.Heading {
                        text: i18nc("@title", "🔗 External Links")
                        level: 2
                        type: Kirigami.Heading.Type.Primary
                        wrapMode: Text.Wrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.largeSpacing

                ApplicationResourceButton {
                    id: helpButton

                    visible: website.length > 0

                    icon: "documentation-symbolic"
                    website: application.helpURL.toString()
                    linkText: i18nc("@info text of a web URL", "Read the documentation")
                }

                ApplicationResourceButton {
                    id: homepageButton


                    visible: website.length > 0

                    icon: "internet-services-symbolic"
                    website: application.homepage.toString()
                    linkText: i18nc("@info text of a web URL", "Visit the project's website")
                }

                ApplicationResourceButton {
                    id: donateButton

                    visible: website.length > 0

                    icon: "help-donate-symbolic"
                    website: application.donationURL.toString()
                    linkText: i18nc("@info text of a web URL", "Donate to the project")
                }

                ApplicationResourceButton {
                    id: bugButton

                    visible: website.length > 0

                    icon: "tools-report-bug-symbolic"
                    website: application.bugURL.toString()
                    linkText: i18nc("@info text of a web URL", "Report a bug")
                }

                ApplicationResourceButton {
                    id: contributeButton

                    visible: website.length > 0

                    icon: "applications-development-symbolic"
                    website: application.contributeURL.toString()
                    linkText: i18nc("@info text of a web URL", "Start contributing")
                }
                } // ColumnLayout for buttons end
            } // linksLayout end
        } // Rectangle end

        Repeater {
            model: appInfo.application.bottomObjects

            delegate: Loader {
                required property string modelData

                Layout.fillWidth: true

                onModelDataChanged: {
                    setSource(modelData, { resource: Qt.binding(() => appInfo.application) });
                }
            }
        }
    }

    AddonsView {
        id: addonsView

        application: appInfo.application
        parent: appInfo.QQC2.Overlay.overlay
    }

    Kirigami.Dialog {
        id: allLicensesSheet
        title: i18n("All Licenses")
        standardButtons: Kirigami.Dialog.NoButton
        preferredWidth: Kirigami.Units.gridUnit * 16
        maximumHeight: Kirigami.Units.gridUnit * 20

        ColumnLayout {
            spacing: 0

            Repeater {
                model: appInfo.application.licenses

                delegate: QQC2.ItemDelegate {
                    background: null
                    id: delegate

                    required property var modelData

                    contentItem: Kirigami.UrlButton {
                        // Override some things to keep the right appearance for non-free licenses with no URL.
                        readonly property bool hasUrl: url !== ""
                        enabled: true
                        font.underline: hasUrl
                        acceptedButtons: hasUrl ? Qt.LeftButton : Qt.NoButton
                        mouseArea.cursorShape: hasUrl ? Qt.PointingHandCursor : undefined

                        text: delegate.modelData.name
                        url: delegate.modelData.url
                        horizontalAlignment: Text.AlignLeft
                        normalColor: appInfo.colorForLicenseType(delegate.modelData.licenseType)
                    }
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: contentRatingDialog
        parent: appInfo.QQC2.Overlay.overlay
        title: i18n("Content Rating")
        preferredWidth: Kirigami.Units.gridUnit * 25
        standardButtons: Kirigami.Dialog.NoButton

        QQC2.Label {
            text: appInfo.application.contentRatingDescription
            textFormat: Text.MarkdownText
            wrapMode: Text.Wrap
        }
    }

    Kirigami.Dialog {
        id: licenseDetailsDialog

        function openWithLicenseType(licenseType: string): void {
            licenseExplanation.text = appInfo.explanationForLicenseType(licenseType);
            open();
        }

        parent: appInfo.QQC2.Overlay.overlay
        width: Kirigami.Units.gridUnit * 25
        standardButtons: Kirigami.Dialog.NoButton

        title: i18nc("@title:window", "License Information")

        TextEdit {
            id: licenseExplanation

            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing

            wrapMode: Text.Wrap
            textFormat: TextEdit.RichText
            readOnly: true

            color: Kirigami.Theme.textColor
            selectedTextColor: Kirigami.Theme.highlightedTextColor
            selectionColor: Kirigami.Theme.highlightColor

            onLinkActivated: url => Qt.openUrlExternally(url)

            HoverHandler {
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }
    }
}
