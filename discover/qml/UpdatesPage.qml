pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

DiscoverPage {
    id: page

    title: i18n("Updates")

    property string footerLabel: ""
    property int footerProgress: 0
    property bool busy: false
    readonly property string name: title

    Discover.ResourcesUpdatesModel {
        id: resourcesUpdatesModel
        onPassiveMessage: message => {
            sheet.errorMessage = message;
            sheet.visible = true;
        }
        onIsProgressingChanged: {
            if (!isProgressing) {
                resourcesUpdatesModel.prepare()
            }
        }

        Component.onCompleted: {
            if (!isProgressing) {
                resourcesUpdatesModel.prepare()
            }
        }
    }

    Kirigami.OverlaySheet {
        id: sheet

        property string errorMessage: ""

        parent: page.QQC2.Overlay.overlay

        title: contentLoader.sourceComponent === friendlyMessageComponent ? i18n("Update Issue") :  i18n("Technical details")

        Loader {
            id: contentLoader
            active: true
            sourceComponent: friendlyMessageComponent

            Component {
                id: friendlyMessageComponent

                ColumnLayout {
                    QQC2.Label {
                        id: friendlyMessage
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.round(page.width * 0.75)
                        Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
                        text: i18n("There was an issue installing this update. Please try again later.")
                        wrapMode: Text.WordWrap
                    }
                    QQC2.Button {
                        id: seeDetailsAndreportIssueButton
                        Layout.alignment: Qt.AlignRight
                        text: i18n("See Technical Details")
                        icon.name: "view-process-system"
                        onClicked: {
                            contentLoader.sourceComponent = nerdyDetailsComponent;
                        }
                    }
                }
            }

            Component {
                id: nerdyDetailsComponent

                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.TextArea {
                        Layout.fillWidth: true
                        text: sheet.errorMessage
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.round(page.width*0.75)
                        Layout.topMargin: Kirigami.Units.largeSpacing
                        Layout.bottomMargin: Kirigami.Units.largeSpacing
                        text: i18nc("@info %1 is the name of the user's distro/OS", "If the error indicated above looks like a real issue and not a temporary network error, please report it to %1, not KDE.", Discover.ResourcesModel.distroName)
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        QQC2.Button {
                            text: i18n("Copy Text")
                            icon.name: "edit-copy"
                            onClicked: {
                                app.copyTextToClipboard(sheet.errorMessage);
                                window.showPassiveNotification(i18nc("@info %1 is the name of the user's distro/OS", "Error message copied. Remember to report it to %1, not KDE!", Discover.ResourcesModel.distroName));
                            }
                        }

                        Item { Layout.fillWidth: true}

                        QQC2.Button {
                            text: i18nc("@action:button %1 is the name of the user's distro/OS", "Report Issue to %1", Discover.ResourcesModel.distroName)
                            icon.name: "tools-report-bug"
                            onClicked: {
                                Qt.openUrlExternally(Discover.ResourcesModel.distroBugReportUrl())
                                sheet.visible = false
                            }
                        }
                    }
                }
            }
        }

        // Ensure that friendly message is shown if the user closes the sheet and
        // then opens it again
        onVisibleChanged: if (visible) {
            contentLoader.sourceComponent = friendlyMessageComponent;
        }
    }

    Discover.UpdateModel {
        id: updateModel
        backend: resourcesUpdatesModel
    }

    property bool startHeadlessUpdate: false
    function considerStartingHeadlessUpdate() {
        if (!startHeadlessUpdate || !readyToUpdate) {
            return;
        }
        if (updateAction.enabled) {
            updateAction.trigger()
            app.quitWhenIdle();
            startHeadlessUpdate = false;
        } else if (updateAction.hasErrors) {
            console.warn("Unable to start update")
            app.restore();
            startHeadlessUpdate = false;
        } else {
            console.warn("Waiting for updates")
        }
    }
    onStartHeadlessUpdateChanged: considerStartingHeadlessUpdate()
    readonly property bool readyToUpdate: !resourcesUpdatesModel.isProgressing && !resourcesUpdatesModel.isFetching
    onReadyToUpdateChanged: considerStartingHeadlessUpdate()
    readonly property alias hasErrors: updateAction.hasErrors
    Kirigami.Action {
        id: updateAction
        text: page.unselected > 0 ? i18nc("@action:button as in, 'update the selected items' ", "Update Selected") : i18nc("@action:button as in, 'update all items'", "Update All")
        visible: updateModel.toUpdateCount
        icon.name: "update-none"

        readonly property bool hasErrors: page.header.children.some(item => item?.visible && item instanceof Kirigami.InlineMessage)

        enabled: page.readyToUpdate && !hasErrors
        onEnabledChanged: enabled => {
            page.considerStartingHeadlessUpdate()
        }
        onTriggered: resourcesUpdatesModel.updateAll()
    }

    header: ColumnLayout {
        id: errorsColumn

        spacing: Kirigami.Units.smallSpacing

        DiscoverInlineMessage {
            Layout.fillWidth: true
            inlineMessage: Discover.ResourcesModel.inlineMessage
        }

        Repeater {
            model: resourcesUpdatesModel.errorMessages
            delegate: Kirigami.InlineMessage {
                id: inline

                required property string modelData

                Layout.fillWidth: true
                position: Kirigami.InlineMessage.Position.Header
                text: modelData
                visible: true
                type: Kirigami.MessageType.Error
                onVisibleChanged: errorsColumn.childrenChanged()

                actions: [
                    Kirigami.Action {
                        icon.name: "dialog-cancel"
                        text: i18n("Ignore")
                        onTriggered: {
                            inline.visible = false
                        }
                    }
                ]
            }
        }
    }

    footer: ColumnLayout {
        // NOTE: we need to very aggressively anchor the layout or it will not have suitable dimensions
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        QQC2.ScrollView {
            id: scv
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? Kirigami.Units.gridUnit * 10 : 0
            visible: log.contents.length > 0
            QQC2.TextArea {
                readOnly: true
                text: log.contents
                wrapMode: TextEdit.Wrap

                cursorPosition: text.length - 1
                font: Kirigami.Theme.fixedWidthFont

                Discover.ReadFile {
                    id: log
                    filter: ".*ALPM-SCRIPTLET\\] .*"
                    path: "/var/log/pacman.log"
                }
            }
        }

        QQC2.ToolBar {
            id: footerToolbar
            Layout.fillWidth: true
            visible: (updateModel.totalUpdatesCount > 0 && resourcesUpdatesModel.isProgressing) || (!resourcesUpdatesModel.isFetching && updateModel.hasUpdates)

            position: QQC2.ToolBar.Footer

            contentItem: RowLayout {
                QQC2.ToolButton {
                    enabled: page.unselected > 0 && updateAction.enabled && !resourcesUpdatesModel.isFetching
                    visible: updateModel.totalUpdatesCount > 1 && !resourcesUpdatesModel.isProgressing
                    icon.name: "edit-select-all"
                    text: i18n("Select All")
                    onClicked: { updateModel.checkAll(); }
                }

                QQC2.ToolButton {
                    enabled: page.unselected !== updateModel.totalUpdatesCount && updateAction.enabled && !resourcesUpdatesModel.isFetching
                    visible: updateModel.totalUpdatesCount > 1 && !resourcesUpdatesModel.isProgressing
                    icon.name: "edit-select-none"
                    text: i18n("Select None")
                    onClicked: { updateModel.uncheckAll(); }
                }

                RowLayout {
                    visible: resourcesUpdatesModel.needsReboot && resourcesUpdatesModel.isProgressing
                    spacing: Kirigami.Units.smallSpacing

                    Layout.fillWidth: true

                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing

                    RowLayout {
                        visible: resourcesUpdatesModel.needsReboot && resourcesUpdatesModel.isProgressing
                        spacing: Kirigami.Units.smallSpacing
                        Layout.fillWidth: true

                        QQC2.Label {
                            text: i18nc("@info on the completion of updates, the action that automatically happens after (e.g shut down)", "On completion, automatically:")
                        }

                        QQC2.ComboBox {
                            id: actionAfterUpdateCombo
                            model: [i18nc("@item:inlistbox placeholder for when no action is selected", "Select an action"), i18nc("@item:inlistbox", "Restart"), i18nc("@item:inlistbox", "Shut down")]
                        }
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    horizontalAlignment: Text.AlignRight
                    text: i18n("Total size: %1", updateModel.updateSize)
                    elide: Text.ElideLeft
                }
            }
        }
    }

    Kirigami.Action {
        id: cancelUpdateAction
        icon.name: "dialog-cancel"
        text: i18n("Cancel")
        enabled: resourcesUpdatesModel.transaction && resourcesUpdatesModel.transaction.isCancellable
        onTriggered: resourcesUpdatesModel.transaction.cancel()
    }

    readonly property int unselected: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)

    supportsRefreshing: true
    onRefreshingChanged: {
        Discover.ResourcesModel.updateAction.triggered()
        refreshing = false
    }

    readonly property Item report: Item {
        parent: page
        anchors.fill: parent

        Kirigami.Action {
            id: promptRestartAction
            icon.name: "system-reboot-update"
            text: i18nc("@action:button", "Restart and Install Updates")
            visible: false
            onTriggered: app.promptReboot()
        }

        Kirigami.LoadingPlaceholder {
            id: statusLabel

            width: parent.width - Kirigami.Units.gridUnit * 2
            // Fixed Y location so it doesn't jump around as backends load
            y: (parent.height / 2) - (Kirigami.Units.gridUnit * 3)

            icon.name: {
                if (page.footerProgress === 0 && page.footerLabel !== "" && !page.busy) {
                    return "update-none"
                } else {
                    return ""
                }
            }
            text: page.footerLabel
            determinate: true
            progressBar.value: page.footerProgress

            QQC2.Label {
                Layout.fillWidth: true

                visible: text.length > 0
                opacity: 0.75

                text: Discover.ResourcesModel.remainingDescription

                horizontalAlignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap

            }
        }
    }
    ListView {
        id: updatesView
        currentIndex: -1
        reuseItems: true
        clip: true

        activeFocusOnTab: true
        onActiveFocusChanged: {
            if (activeFocus && currentIndex === -1) {
                currentIndex = 0
            }
        }

        Accessible.role: Accessible.List

        model: KItemModels.KSortFilterProxyModel {
            sourceModel: updateModel
            sortRole: Discover.UpdateModel.SectionResourceProgressRole
            filterRoleName: "resourceStateIsDone"
            filterString: "false"
        }

        section {
            property: "section"
            delegate: Kirigami.ListSectionHeader {
                required property string section

                width: updatesView.width
                label: section
            }
        }

        delegate: QQC2.ItemDelegate {
            id: listItem

            // type: roles of Discover.UpdateModel
            required property var model
            required property int index
            required property bool extended

            width: updatesView.width

            highlighted: false
            focus: ListView.isCurrentItem
            activeFocusOnTab: ListView.isCurrentItem
            checked: itemChecked.checked

            Accessible.name: model.display
            Accessible.description: model.resource?.upgradeText ?? ""
            Accessible.role: Accessible.ListItem

            onEnabledChanged: if (!enabled) {
                model.extended = false;
            }

            Keys.onSpacePressed: event => {
                itemChecked.clicked();
            }
            Keys.onReturnPressed: event => {
                model.extended = !model.extended;
            }
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Alt) {
                    model.extended = true;
                }
            }
            Keys.onReleased: event => {
                if (event.key === Qt.Key_Alt) {
                    model.extended = false;
                }
            }

            Component.onCompleted: {
                if (extended) {
                    updateModel.fetchUpdateDetails(index)
                    if (ListView.isCurrentItem) {
                        forceActiveFocus(Qt.OtherFocusReason)
                    }
                }
            }
            onExtendedChanged: if (extended) {
                updateModel.fetchUpdateDetails(index)
            } else {
                moreInformationButton.focus = false
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    QQC2.CheckBox {
                        id: itemChecked
                        Layout.alignment: Qt.AlignVCenter
                        checked: listItem.model.checked === Qt.Checked
                        activeFocusOnTab: false
                        onClicked: listItem.model.checked = (listItem.model.checked === Qt.Checked ? Qt.Unchecked : Qt.Checked)
                        enabled: !resourcesUpdatesModel.isProgressing
                    }

                    Kirigami.Icon {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                        Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                        source: listItem.model.decoration
                        selected: listItem.down
                        smooth: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Kirigami.Units.smallSpacing
                        Layout.alignment: Qt.AlignVCenter

                        spacing: 0

                        // App name
                        Kirigami.Heading {
                            Layout.fillWidth: true
                            text: listItem.model.display
                            level: 3
                            elide: Text.ElideRight
                            color: listItem.down ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                        }

                        // Version numbers
                        QQC2.Label {
                            Layout.fillWidth: true
                            elide: truncated ? Text.ElideLeft : Text.ElideRight
                            text: listItem.model.resource?.upgradeText ?? ""
                            color: listItem.down ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            opacity: 0.75
                        }
                    }

                    TransactionProgressIndicator {
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 6

                        Kirigami.Theme.colorSet: Kirigami.Theme.View
                        Kirigami.Theme.inherit: false

                        text: listItem.model.resourceState === 2 ? i18n("Installing") : listItem.model.size

                        progress: listItem.model.resourceProgress / 100
                        selected: listItem.down
                    }
                }

                QQC2.Frame {
                    Layout.fillWidth: true
                    implicitHeight: view.implicitHeight
                    visible: listItem.model.extended && listItem.model.changelog.length > 0
                    QQC2.Label {
                        id: view
                        anchors {
                            right: parent.right
                            left: parent.left
                        }
                        text: listItem.model.changelog
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                        color: listItem.down ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                        onLinkActivated: link => Qt.openUrlExternally(link)

                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    visible: listItem.model.extended

                    QQC2.Label {
                        Layout.leftMargin: Kirigami.Units.gridUnit
                        text: i18n("Update from:")
                        color: listItem.down ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }
                    // Backend icon
                    Kirigami.Icon {
                        source: listItem.model.resource.sourceIcon
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        selected: listItem.down
                    }
                    // Backend label and origin/remote
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: listItem.model.resource.displayOrigin.length === 0 ? listItem.model.resource.backend.displayName
                                : i18nc("%1 is the backend that provides this app, %2 is the specific repository or address within that backend","%1 (%2)",
                                        listItem.model.resource.backend.displayName, listItem.model.resource.displayOrigin)
                        elide: Text.ElideRight
                        color: listItem.down ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }

                    QQC2.Button {
                        id: moreInformationButton
                        Layout.alignment: Qt.AlignRight
                        text: i18n("More Information…")
                        enabled: !resourcesUpdatesModel.isProgressing
                        onClicked: Navigation.openApplication(listItem.model.resource)
                    }
                }
            }

            onClicked: {
                model.extended = !model.extended
            }
        }
    }

    readonly property alias secSinceUpdate: resourcesUpdatesModel.secsToLastUpdate
    state:  ( resourcesUpdatesModel.isProgressing        ? "progressing"
            : resourcesUpdatesModel.isFetching           ? "fetching"
            : updateModel.hasUpdates                     ? "has-updates"
            : resourcesUpdatesModel.needsReboot          ? "reboot"
            : secSinceUpdate < 0                         ? "unknown"
            : secSinceUpdate === 0                       ? "now-uptodate"
            : secSinceUpdate < 1000 * 60 * 60 * 24       ? "uptodate"
            : secSinceUpdate < 1000 * 60 * 60 * 24 * 7   ? "medium"
            :                                              "low"
            )

    states: [
        State {
            name: "fetching"
            PropertyChanges { page.footerLabel: i18nc("@info", "Fetching updates…") }
            PropertyChanges { page.footerProgress: Discover.ResourcesModel.fetchingUpdatesProgress }
            PropertyChanges { page.actions: [ updateAction, refreshAction ] }
            PropertyChanges { page.busy: true }
            PropertyChanges { statusLabel.progressBar.visible: true }
            PropertyChanges { updatesView.opacity: 0 }
        },
        State {
            name: "progressing"
            PropertyChanges { page.supportsRefreshing: false }
            PropertyChanges { page.actions: [cancelUpdateAction] }
            PropertyChanges { statusLabel.visible: false }
        },
        State {
            name: "has-updates"
            PropertyChanges { page.title: i18nc("@info", "Updates") }
            // On mobile, we want "Update" to be the primary action so it's in
            // the center, but on desktop this feels a bit awkward and it would
            // be better to have "Update" be the right-most action
            PropertyChanges { page.actions: [ updateAction, refreshAction ] }
            PropertyChanges { statusLabel.visible: false }
        },
        State {
            name: "reboot"
            PropertyChanges { page.actions: [refreshAction] }
            PropertyChanges { page.footerLabel: i18nc("@info", "Updates will be installed after the system is restarted") }
            PropertyChanges { statusLabel.helpfulAction: promptRestartAction }
            PropertyChanges { statusLabel.explanation: i18nc("@info", "You can keep using the system if you're not ready to restart yet.") }
            PropertyChanges { statusLabel.progressBar.visible: false }
            StateChangeScript {
                script: if (resourcesUpdatesModel.readyToReboot) {
                    if (actionAfterUpdateCombo.currentIndex === 1) {
                        app.rebootNow()
                    } else if (actionAfterUpdateCombo.currentIndex === 2) {
                        app.shutdownNow()
                    }
                }
            }
        },
        State {
            name: "now-uptodate"
            PropertyChanges { page.footerLabel: i18nc("@info", "Up to date") }
            PropertyChanges { page.actions: [refreshAction] }
            PropertyChanges { statusLabel.explanation: "" }
            PropertyChanges { statusLabel.progressBar.visible: false }
        },
        State {
            name: "uptodate"
            PropertyChanges { page.footerLabel: i18nc("@info", "Up to date") }
            PropertyChanges { page.actions: [refreshAction] }
            PropertyChanges { statusLabel.explanation: "" }
            PropertyChanges { statusLabel.progressBar.visible: false }
        },
        State {
            name: "medium"
            PropertyChanges { page.title: i18nc("@info", "Up to date") }
            PropertyChanges { page.actions: [refreshAction] }
            PropertyChanges { statusLabel.explanation: "" }
            PropertyChanges { statusLabel.progressBar.visible: false }
        },
        State {
            name: "low"
            PropertyChanges { page.title: i18nc("@info", "Should check for updates") }
            PropertyChanges { page.actions: [refreshAction] }
            PropertyChanges { statusLabel.explanation: "" }
            PropertyChanges { statusLabel.progressBar.visible: false }
        },
        State {
            name: "unknown"
            PropertyChanges { page.title: i18nc("@info", "Time of last update unknown") }
            PropertyChanges { page.actions: [refreshAction] }
            PropertyChanges { statusLabel.explanation: "" }
            PropertyChanges { statusLabel.progressBar.visible: false }
        }
    ]
}
