pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.discover as Discover
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

DiscoverPage {
    id: page

    property string search
    readonly property string name: title

    clip: true
    title: i18n("Settings")

    Kirigami.Action {
        id: configureUpdatesAction
        text: i18n("Configure Updates…")
        displayHint: Kirigami.DisplayHint.AlwaysHide
        onTriggered: {
            KCMUtils.KCMLauncher.openSystemSettings("kcm_updates");
        }
    }

    actions: feedbackLoader.item?.actions ?? [configureUpdatesAction]

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        // Modern settings header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: settingsHeaderContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.smallSpacing

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.6, 0.4, 0.8, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.5, 0.3, 0.7, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.6, 0.4, 0.8, 0.08) }
            }

            radius: Kirigami.Units.smallSpacing
            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            Row {
                id: settingsHeaderContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: Kirigami.Units.largeSpacing
                    rightMargin: Kirigami.Units.largeSpacing
                }
                spacing: 8  // Минимальный отступ

                Rectangle {
                    width: Kirigami.Units.iconSizes.large
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.6, 0.4, 0.8, 0.15)
                    anchors.verticalCenter: parent.verticalCenter

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "configure"
                        width: parent.width * 0.6
                        height: width
                        color: "#9b59b6"
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Kirigami.Heading {
                        text: i18n("⚙️ Software Sources & Settings")
                        level: 2
                    }

                    QQC2.Label {
                        text: i18n("Manage repositories and configure update settings")
                        opacity: 0.7
                        font: Kirigami.Theme.smallFont
                    }
                }

                Item {
                    width: parent.width - x - updateSettingsBtn.width - parent.spacing * 2
                }

                QQC2.Button {
                    id: updateSettingsBtn
                    anchors.verticalCenter: parent.verticalCenter
                    icon.name: "configure"
                    text: i18n("Update Settings")
                    onClicked: configureUpdatesAction.trigger()
                }
            }
        }

        Repeater {
            model: Discover.SourcesModel.sources

            delegate: Kirigami.InlineMessage {
                id: delegate

                required property Discover.AbstractSourcesBackend modelData

                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                text: modelData.inlineAction?.toolTip ?? ""
                visible: modelData.inlineAction?.visible ?? false
                actions: Kirigami.Action {
                    icon.name: delegate.modelData.inlineAction?.iconName ?? ""
                    text: delegate.modelData.inlineAction?.text ?? ""
                    onTriggered: delegate.modelData.inlineAction?.trigger()
                }
            }
        }
    }

    ListView {
        id: sourcesView
        model: Discover.SourcesModel
        Component.onCompleted: Qt.callLater(Discover.SourcesModel.showingNow)
        currentIndex: -1
        pixelAligned: true
        section.property: "sourceName"
        section.delegate: Rectangle {
            id: backendItem

            required property string section

            height: Math.ceil(Math.max(Kirigami.Units.gridUnit * 3, backendContent.implicitHeight + Kirigami.Units.largeSpacing))

            readonly property Discover.AbstractSourcesBackend backend: Discover.SourcesModel.sourcesBackendByName(section)
            readonly property Discover.AbstractResourcesBackend resourcesBackend: backend.resourcesBackend
            readonly property bool isDefault: Discover.ResourcesModel.currentApplicationBackend === resourcesBackend

            width: sourcesView.width

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: isDefault ? Qt.rgba(0.2, 0.6, 1, 0.1) : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.05)
                }
                GradientStop {
                    position: 0.5
                    color: isDefault ? Qt.rgba(0.3, 0.5, 0.9, 0.15) : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.08)
                }
                GradientStop {
                    position: 1.0
                    color: isDefault ? Qt.rgba(0.2, 0.6, 1, 0.1) : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.05)
                }
            }

            border.width: isDefault ? 1 : 0
            border.color: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.3)
            radius: Kirigami.Units.smallSpacing

            Connections {
                target: backendItem.backend
                function onPassiveMessage(message) {
                    window.showPassiveNotification(message)
                }
                function onProceedRequest(title, description) {
                    const dialog = sourceProceedDialog.createObject(window, {
                        sourcesBackend: backendItem.backend,
                        title,
                        description,
                    })
                    dialog.open()
                }
            }

            RowLayout {
                id: backendContent
                anchors {
                    fill: parent
                    margins: Kirigami.Units.smallSpacing
                }
                spacing: Kirigami.Units.largeSpacing

                // Backend icon
                Rectangle {
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    radius: width / 2
                    color: backendItem.isDefault ? Qt.rgba(0.2, 0.6, 1, 0.2) : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: {
                            var name = resourcesBackend.displayName.toLowerCase()
                            if (name.includes("flatpak")) return "flatpak"
                            if (name.includes("snap")) return "snap"
                            if (name.includes("packagekit") || name.includes("rpm")) return "package-x-generic"
                            if (name.includes("fwupd")) return "system-software-update"
                            return "package-x-generic"
                        }
                        width: parent.width * 0.6
                        height: width
                        color: backendItem.isDefault ? "#3498db" : Kirigami.Theme.textColor
                    }
                }

                Kirigami.Heading {
                    text: resourcesBackend.displayName
                    level: 3
                    font.weight: backendItem.isDefault ? Font.Bold : Font.Normal
                }

                Kirigami.ActionToolBar {
                    id: actionBar

                    alignment: Qt.AlignRight

                    Kirigami.Action {
                        id: isDefaultbackendLabelAction

                        visible: backendItem.isDefault
                        displayHint: Kirigami.DisplayHint.KeepVisible
                        displayComponent: Kirigami.Heading {
                            text: i18n("Default Source")
                            level: 3
                            font.weight: Font.Bold
                        }
                    }

                    Kirigami.Action {
                        id: addSourceAction
                        text: i18n("Add Source…")
                        icon.name: "list-add"
                        visible: backendItem.backend && backendItem.backend.supportsAdding

                        onTriggered: {
                            const addSourceDialog = dialogComponent.createObject(window, {
                                displayName: backendItem.backend.resourcesBackend.displayName,
                            })
                            addSourceDialog.open()
                        }
                    }

                    Component {
                        id: dialogComponent
                        AddSourceDialog {
                            source: backendItem.backend

                            onClosed: {
                                destroy();
                            }
                        }
                    }

                    Kirigami.Action {
                        id: makeDefaultAction
                        visible: resourcesBackend && resourcesBackend.hasApplications && !backendItem.isDefault

                        text: i18n("Make Default")
                        icon.name: "favorite"
                        onTriggered: Discover.ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend
                    }

                    Component {
                        id: kirigamiAction
                        ConvertDiscoverAction {}
                    }

                    function mergeActions(moreActions) {
                        const actions = [
                            isDefaultbackendLabelAction,
                            makeDefaultAction,
                            addSourceAction
                        ]
                        for (const action of moreActions) {
                            actions.push(kirigamiAction.createObject(this, { action }))
                        }
                        return actions;
                    }
                    actions: mergeActions(backendItem.backend.actions)
                }
            }
        }

        Component {
            id: sourceProceedDialog
            Kirigami.OverlaySheet {
                id: sheet

                property Discover.AbstractSourcesBackend sourcesBackend
                property alias description: descriptionLabel.text
                property bool acted: false

                parent: page.QQC2.Overlay.overlay
                showCloseButton: false

                implicitWidth: Kirigami.Units.gridUnit * 30

                Kirigami.SelectableLabel {
                    id: descriptionLabel
                    width: parent.width
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.Wrap
                }

                footer: QQC2.DialogButtonBox {
                    QQC2.Button {
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.AcceptRole
                        text: i18n("Proceed")
                        icon.name: "dialog-ok"
                    }

                    QQC2.Button {
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                    }

                    onAccepted: {
                        sheet.sourcesBackend.proceed()
                        sheet.acted = true
                        sheet.close()
                    }

                    onRejected: {
                        sheet.sourcesBackend.cancel()
                        sheet.acted = true
                        sheet.close()
                    }
                }

                onOpened: {
                    descriptionLabel.forceActiveFocus(Qt.PopupFocusReason);
                }

                onClosed: {
                    if (!acted) {
                        sourcesBackend.cancel()
                    }
                    destroy();
                }
            }
        }

        delegate: Kirigami.SwipeListItem {
            id: delegate

            required property int index
            required property var model

            enabled: model.display.length > 0 && model.enabled
            highlighted: ListView.isCurrentItem
            supportsMouseEvents: false
            visible: model.display.indexOf(page.search) >= 0
            height: visible ? implicitHeight : 0

            Keys.onReturnPressed: enabledBox.clicked()
            Keys.onSpacePressed: enabledBox.clicked()
            actions: [
                Kirigami.Action {
                    icon.name: "go-up"
                    tooltip: i18n("Increase priority")
                    enabled: delegate.model.sourcesBackend.firstSourceId !== delegate.model.sourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onTriggered: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.sourceId, -1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to increase '%1' preference", delegate.model.display))
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: "go-down"
                    tooltip: i18n("Decrease priority")
                    enabled: delegate.model.sourcesBackend.lastSourceId !== delegate.model.sourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onTriggered: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.sourceId, +1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to decrease '%1' preference", delegate.model.display))
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: "edit-delete"
                    tooltip: i18n("Remove repository")
                    visible: delegate.model.sourcesBackend.supportsAdding
                    onTriggered: {
                        const backend = delegate.model.sourcesBackend
                        if (!backend.removeSource(delegate.model.sourceId)) {
                            console.warn("Failed to remove the source", delegate.model.display)
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: delegate.mirrored ? "go-next-symbolic-rtl" : "go-next-symbolic"
                    tooltip: i18n("Show contents")
                    visible: delegate.model.sourcesBackend.canFilterSources
                    onTriggered: {
                        Navigation.openApplicationListSource(delegate.model.sourceId)
                    }
                }
            ]

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.CheckBox {
                    id: enabledBox

                    readonly property var idx: sourcesView.model.index(index, 0)
                    readonly property /*Qt::CheckState*/int modelChecked: delegate.model.checkState
                    checked: modelChecked !== Qt.Unchecked
                    enabled: sourcesView.model.flags(idx) & Qt.ItemIsUserCheckable
                    onClicked: {
                        sourcesView.model.setData(idx, checkState, Qt.CheckStateRole)
                        checked = Qt.binding(() => (modelChecked !== Qt.Unchecked))
                    }
                }
                QQC2.Label {
                    text: delegate.model.display + (delegate.model.toolTip ? " - <i>" + delegate.model.toolTip + "</i>" : "")
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
                    Layout.fillWidth: true
                }
            }
        }

        footer: ColumnLayout {
            spacing: 0
            width: ListView.view.width

            Kirigami.ListSectionHeader {
                Layout.fillWidth: true

                visible: back.count > 0
                text: i18n("Missing Backends")
            }

            Repeater {
                id: back
                model: Discover.ResourcesProxyModel {
                    extending: "org.kde.discover.desktop"
                    filterMinimumState: false
                    stateFilter: Discover.AbstractResource.None
                }
                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property int index
                    required property var model
                    required property string name

                    Layout.fillWidth: true
                    background: null
                    hoverEnabled: false
                    down: false

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        KD.IconTitleSubtitle {
                            title: name
                            icon.source: delegate.model.icon
                            subtitle: delegate.model.comment
                            Layout.fillWidth: true
                        }
                        InstallApplicationButton {
                            application: delegate.model.application
                        }
                    }
                }
            }
        }
    }
}
