pragma ComponentBehavior: Bound

import QtQml.Models
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.config as KConfig

Kirigami.ApplicationWindow {
    id: window

    property string currentTopLevel

    readonly property string topBrowsingComp: "BrowsingPage.qml"
    readonly property string topInstalledComp: "InstalledPage.qml"
    readonly property string topCoprComp: "CoprPage.qml"
    readonly property string topSearchComp: "SearchPage.qml"
    readonly property string topUpdateComp: "UpdatesPage.qml"
    readonly property string topSourcesComp: "SourcesPage.qml"
    readonly property string topAboutComp: "AboutPage.qml"

    objectName: "DiscoverMainWindow"
    title: leftPage?.title ?? ""

    width: Kirigami.Units.gridUnit * 52
    height: Math.max(Kirigami.Units.gridUnit * 39, window.globalDrawer.contentHeight)

    visible: true

    minimumWidth: Kirigami.Units.gridUnit * 17
    minimumHeight: Kirigami.Units.gridUnit * 17

    pageStack.globalToolBar.style: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.Titles : Kirigami.ApplicationHeaderStyle.Auto
    pageStack.globalToolBar.showNavigationButtons: pageStack.currentIndex === 0 ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.ShowBackButton
    pageStack.globalToolBar.canContainHandles: true // mobile handles in header
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    
    readonly property Item leftPage: window.pageStack.depth > 0 ? window.pageStack.get(0) : null

    KConfig.WindowStateSaver {
        configGroupName: "MainWindow"
    }

    Component.onCompleted: {
        if (app.isRoot) {
            messagesSheet.addMessage(i18n("Running as <em>root</em> is discouraged and unnecessary."));
        }
    }

    // This property is queried from C++, do not remove it
    readonly property string describeSources: feedbackLoader.item?.describeDataSources ?? ""
    Loader {
        id: feedbackLoader
        active: typeof DiscoverApp.UserFeedbackSettings !== "undefined"
        source: "Feedback.qml"
    }

    Kirigami.PagePool {
        id: globalPool
    }

    TopLevelPageData {
        id: featuredAction
        icon.name: "go-home"
        text: i18n("&Home")
        component: topBrowsingComp
        objectName: "discover"
    }

    TopLevelPageData {
        id: searchAction
        visible: enabled
        enabled: !window.wideScreen
        icon.name: "search"
        text: i18n("&Search")
        component: topSearchComp
        objectName: "search"
        shortcut: StandardKey.Find
    }
    TopLevelPageData {
        id: installedAction
        icon.name: "view-list-details"
        text: i18n("&Installed")
        component: topInstalledComp
        objectName: "installed"
    }
    TopLevelPageData {
        id: coprAction
        icon.name: "package"
        text: i18n("&COPR")
        component: topCoprComp
        objectName: "copr"
    }
    TopLevelPageData {
        id: updateAction

        icon.name: Discover.ResourcesModel.updatesCount <= 0
            ? "update-none"
            : (Discover.ResourcesModel.hasSecurityUpdates ? "update-high" : "update-low")

        text: Discover.ResourcesModel.fetchingUpdatesProgress !== 100 ? i18n("&Updates (Fetching…)") : i18n("&Updates (%1)", Discover.ResourcesModel.updatesCount)

        component: topUpdateComp
        objectName: "update"
    }
    TopLevelPageData {
        id: aboutAction
        icon.name: "help-feedback"
        text: i18n("&About")
        component: topAboutComp
        objectName: "about"
        shortcut: StandardKey.HelpContents
    }
    TopLevelPageData {
        id: sourcesAction
        icon.name: "configure"
        text: i18n("S&ettings")
        component: topSourcesComp
        objectName: "sources"
        shortcut: StandardKey.Preferences
    }

    Kirigami.Action {
        id: refreshAction
        readonly property Discover.DiscoverAction action: Discover.ResourcesModel.updateAction
        text: action.text
        icon.name: "view-refresh"
        icon.color: "#3498db"
        onTriggered: action.trigger()
        enabled: action.enabled
        // Don't need to show this action on mobile since you can pull down
        // on the view to refresh, which is the common and expected behavior
        // on that platform - but is not possible on desktop
        visible: !Kirigami.Settings.isMobile
        tooltip: shortcut.nativeText

        // Need to define an explicit Shortcut object so we can get its text
        // using shortcut.nativeText
        shortcut: Shortcut {
            sequences: [ StandardKey.Refresh ]
            onActivated: refreshAction.trigger()
        }
    }

    Connections {
        target: app

        function onOpenApplicationInternal(app) {
            Navigation.clearStack()
            Navigation.openApplication(app)
        }

        function onListMimeInternal(mime)  {
            currentTopLevel = topBrowsingComp;
            Navigation.openApplicationMime(mime)
        }

        function onListCategoryInternal(cat)  {
            currentTopLevel = topBrowsingComp;
            Navigation.openCategory(cat)
        }

        function onOpenSearch(search) {
            Navigation.clearStack()
            Navigation.openApplicationList({ search })
        }

        function onOpenErrorPage(errorMessage, errorExplanation, buttonText, buttonIcon, buttonUrl) {
            Navigation.clearStack()
            console.warn(`Error: ${errorMessage}\n${errorExplanation}\nPlease visit ${buttonUrl}`)
            window.pageStack.push(errorPageComponent, { title: i18n("Error"), errorMessage, errorExplanation, buttonText, buttonIcon, buttonUrl })
        }

        function onUnableToFind(resid) {
            messagesSheet.addMessage(i18n("Unable to find resource: %1", resid));
            Navigation.openHome()
        }
    }

    Connections {
        target: Discover.ResourcesModel

        function onSwitchToUpdates() {
            window.currentTopLevel = topUpdateComp
        }
        function onPassiveMessage(message) {
            messagesSheet.addMessage(message);
        }
    }


    footer: footerLoader.item

    Loader {
        id: footerLoader
        active: !window.wideScreen
        sourceComponent: Kirigami.NavigationTabBar {
            actions: [
                featuredAction,
                searchAction,
                installedAction,
                coprAction,
                updateAction,
            ]
            Component.onCompleted: {
                // Exclusivity is already handled by the actions. This prevents BUG:448460
                tabGroup.exclusive = false
            }
        }
    }

    Component {
        id: errorPageComponent
        Kirigami.Page {
            id: page
            property string errorMessage: ""
            property string errorExplanation: ""
            property string buttonText: ""
            property string buttonIcon: ""
            property string buttonUrl: ""
            readonly property bool isHome: true

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.gridUnit * 2)
                visible: page.errorMessage !== ""
                type: Kirigami.PlaceholderMessage.Type.Actionable // All error messages must be actionable
                icon.name: "emblem-warning"
                text: page.errorMessage
                explanation: page.errorExplanation
                helpfulAction: Kirigami.Action {
                    icon.name: page.buttonIcon
                    text: page.buttonText
                    enabled: page.buttonText.length > 0 && page.buttonUrl.length > 0
                    onTriggered: {
                        Qt.openUrlExternally(page.buttonUrl)
                    }
                }
                onLinkActivated: link => Qt.openUrlExternally(link)
            }
        }
    }

    Component {
        id: proceedDialog
        Kirigami.OverlaySheet {
            id: sheet
            showCloseButton: false
            property QtObject transaction
            property alias description: descriptionLabel.text
            property bool acted: false

            // No need to add our own ScrollView since OverlaySheet includes
            // one automatically.
            // But we do need to put the label into a Layout of some sort so we
            // can limit the width of the sheet.
            ColumnLayout {
                QQC2.Label {
                    id: descriptionLabel

                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 20

                    textFormat: Text.StyledText
                    wrapMode: Text.WordWrap
                }
            }

            footer: RowLayout {

                Item { Layout.fillWidth : true }

                QQC2.Button {
                    text: i18n("Proceed")
                    icon.name: "dialog-ok"
                    onClicked: {
                        transaction.proceed()
                        sheet.acted = true
                        sheet.close()
                    }
                    Keys.onEnterPressed: clicked()
                    Keys.onReturnPressed: clicked()
                }

                QQC2.Button {
                    Layout.alignment: Qt.AlignRight
                    text: i18n("Cancel")
                    icon.name: "dialog-cancel"
                    onClicked: {
                        transaction.cancel()
                        sheet.acted = true
                        sheet.close()
                    }
                    Keys.onEscapePressed: clicked()
                }
            }

            onVisibleChanged: if(!visible) {
                sheet.destroy(1000)
                if (!sheet.acted) {
                    transaction.cancel()
                }
            }
        }
    }

    Component {
        id: distroErrorMessageDialog
        Kirigami.OverlaySheet {
            id: sheet
            property alias message: messageLabel.text

            // No need to add our own ScrollView since OverlaySheet includes
            // one automatically.
            // But we do need to put the label into a Layout of some sort so we
            // can limit the width of the sheet.
            ColumnLayout {
                spacing: Kirigami.Units.largeSpacing

                QQC2.Label {
                    id: messageLabel

                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 20

                    textFormat: Text.StyledText
                    wrapMode: Text.WordWrap
                }

                Kirigami.UrlButton {
                    Layout.fillWidth: true
                    text: i18nc("@info %1 is the name of the operating system", "Report this issue to %1", Discover.ResourcesModel.distroName())
                    url: Discover.ResourcesModel.distroBugReportUrl()
                }
            }

            onVisibleChanged: if (!visible) {
                sheet.destroy(1000)
            }
        }
    }

    Kirigami.Dialog {
        id: messagesSheet

        padding: Kirigami.Units.largeSpacing
        property bool showTechnicalDetails: false

        function addMessage(message: string) {
            messages.append({message});
            app.restore();
        }

        title: messagesSheet.showTechnicalDetails ? (messages.count > 1 ? i18n("Error %1 of %2", messagesSheetView.currentIndex + 1, messages.count) : i18n("Error")) : i18n("Update Issue")

        // No need to add our own ScrollView since OverlaySheet includes
        // one automatically.
        // But we do need to put the label into a Layout of some sort so we
        // can limit the width of the sheet.
        ColumnLayout {
            QQC2.Label {
                id: friendlyMessage
                visible: !messagesSheet.showTechnicalDetails
                Layout.fillWidth: true
                text: i18n("There was an issue during the update or installation process. Please try again later.")
                wrapMode: Text.WordWrap
            }

            StackLayout {
                id: messagesSheetView
                visible: messagesSheet.showTechnicalDetails
                Layout.fillWidth: true
                Layout.bottomMargin: Kirigami.Units.gridUnit

                Repeater {
                    // roles: {
                    //   message: string
                    // }
                    model: ListModel {
                        id: messages

                        onCountChanged: {
                            messagesSheet.visible = (count > 0);
                            if (count > 0 && messagesSheetView.currentIndex === -1) {
                                messagesSheetView.currentIndex = 0;
                            }
                        }
                    }

                    delegate: QQC2.Label {
                        required property string message
                        text: message
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Show Previous")
                icon.name: "go-previous"
                visible: messagesSheet.showTechnicalDetails && messages.count > 1
                enabled: visible && messagesSheetView.currentIndex > 0
                onTriggered: {
                    if (messagesSheetView.currentIndex > 0) {
                        messagesSheetView.currentIndex--;
                    }
                }
            },
            Kirigami.Action {
                text: i18nc("@action:button", "Show Next")
                icon.name: "go-next"
                visible: messagesSheet.showTechnicalDetails && messages.count > 1
                enabled: visible && messagesSheetView.currentIndex < messages.count - 1
                onTriggered: {
                    if (messagesSheetView.currentIndex < messages.count) {
                        messagesSheetView.currentIndex++;
                    }
                }
            },
            Kirigami.Action {
                visible: !messagesSheet.showTechnicalDetails
                text: i18n("See Technical Details")
                icon.name: "view-process-system"
                onTriggered: {
                    messagesSheet.showTechnicalDetails = true;
                }
            },
            Kirigami.Action {
                visible: messagesSheet.showTechnicalDetails
                text: i18n("Copy to Clipboard")
                icon.name: "edit-copy"
                onTriggered: {
                    app.copyTextToClipboard(messages.get(messagesSheetView.currentIndex).message);
                }
            }
        ]

        onVisibleChanged: if (!visible) {
            messagesSheetView.currentIndex = -1;
            messages.clear();
            messagesSheet.showTechnicalDetails = false;
        }
    }

    Instantiator {
        model: Discover.TransactionModel

        delegate: Connections {
            required property Discover.Transaction transaction

            target: transaction

            function onProceedRequest(title, description) {
                const dialog = proceedDialog.createObject(window, { transaction, title, description })
                dialog.open()
                app.restore()
            }

            function onPassiveMessage(message) {
                messagesSheet.addMessage(message);
            }

            function onDistroErrorMessage(message, actions) {
                const dialog = distroErrorMessageDialog.createObject(window, { title: i18n("Error"), transaction, message })
                dialog.open()
                app.restore()
            }

            function onWebflowStarted(url) {
                const component = Qt.createComponent("WebflowDialog.qml");
                if (component.status === Component.Error) {
                    Qt.openUrlExternally(url);
                    console.error("Webflow Error", component.errorString())
                } else if (component.status === Component.Ready) {
                    const sheet = component.createObject(window, { transaction, url });
                    sheet.open()
                }
                component.destroy();
            }
        }
    }

    DiscoverApp.PowerManagementInterface {
        reason: Discover.TransactionModel.mainTransactionText
        preventSleep: Discover.TransactionModel.count > 0
    }

    contextDrawer: Kirigami.ContextDrawer {}

    globalDrawer: DiscoverDrawer {
        wideScreen: window.wideScreen
    }

    onCurrentTopLevelChanged: {
        pageStack.clear();
        if (currentTopLevel) {
            const pageUrl = globalPool.loadPage(currentTopLevel);
            pageStack.push(pageUrl);
        }
        globalDrawer.forceSearchFieldFocus();
    }

    DiscoverApp.UnityLauncher {
        launcherId: "org.kde.discover.desktop"
        progressVisible: Discover.TransactionModel.count > 0
        progress: Discover.TransactionModel.progress
    }
}
