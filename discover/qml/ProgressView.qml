pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

QQC2.ToolButton {
    id: listItem

    padding: Kirigami.Units.largeSpacing
    contentItem: ColumnLayout {
        spacing: 0

        QQC2.Label {
            id: label
            Layout.fillWidth: true
            text: Discover.TransactionModel.count ? i18n("Tasks (%1%)", Discover.TransactionModel.progress) : i18n("Tasks")
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: listItem.pressed || listItem.down ? Kirigami.Theme.highlightedTextColor: Kirigami.Theme.textColor
        }

        QQC2.ProgressBar {
            Layout.fillWidth: true
            value: Discover.TransactionModel.progress / 100
        }
    }

    property Kirigami.OverlaySheet sheetObject

    onClicked: {
        if (!sheetObject) {
            sheetObject = sheet.createObject()
        }

        if (!sheetObject.visible) {
            sheetObject.open()
        }
    }

    Component {
        id: sheet
        Kirigami.OverlaySheet {
            parent: listItem.QQC2.Overlay.overlay

            title: i18n("Tasks")

            onVisibleChanged: if (!visible) {
                sheetObject.destroy(100)
            }

            ListView {
                id: tasksView
                spacing: 0
                implicitWidth: Kirigami.Units.gridUnit * 30

                Component {
                    id: listenerComp
                    Discover.TransactionListener {}
                }
                model: KItemModels.KSortFilterProxyModel {
                    sourceModel: Discover.TransactionModel
                    filterRoleName: "visible"
                    filterRowCallback: (sourceRow, sourceParent) => {
                        const index = sourceModel.index(sourceRow, 0, sourceParent);
                        return sourceModel.data(index, Discover.TransactionModel.VisibleRole) === true;
                    }
                }

                Connections {
                    target: Discover.TransactionModel
                    function onRowsRemoved() {
                        if (Discover.TransactionModel.count === 0) {
                            sheetObject.close();
                        }
                    }
                }

                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property int index
                    required property var model

                    readonly property Discover.TransactionListener listener: listenerComp.createObject(this,
                        (model.transaction.resource
                            ? { resource: model.transaction.resource }
                            : { transaction: model.transaction }))

                    width: tasksView.width

                    // Don't need a highlight or hover effects as it can make the
                    // progress bar a bit hard to see
                    highlighted: false
                    hoverEnabled: false
                    down: false

                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Layout.fillWidth: true

                            Kirigami.Icon {
                                Layout.fillHeight: true
                                Layout.minimumWidth: height
                                source: delegate.model.transaction.icon
                            }

                            QQC2.Label {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                text: {
                                    const tr = delegate.model.transaction;
                                    const li = delegate.listener;

                                    if (li.isActive && tr.remainingTime > 0) {
                                        return i18nc(
                                            "TransactioName - TransactionStatus: speed, remaining time", "%1 - %2: %3, %4 remaining",
                                            tr.name,
                                            li.statusText,
                                            tr.downloadSpeedString,
                                            tr.remainingTime
                                        );
                                    } else if (li.isActive && tr.downloadSpeed > 0) {
                                        return i18nc(
                                            "TransactioName - TransactionStatus: speed", "%1 - %2: %3",
                                            tr.name,
                                            li.statusText,
                                            tr.downloadSpeedString
                                        );
                                    } else if (li.isActive) {
                                        return i18nc(
                                            "TransactioName - TransactionStatus", "%1 - %2",
                                            tr.name,
                                            li.statusText
                                        );
                                    } else {
                                        return tr.name;
                                    }
                                }
                            }
                            QQC2.ToolButton {
                                icon.name: "dialog-cancel"
                                text: i18n("Cancel")
                                visible: delegate.listener.isCancellable
                                onClicked: delegate.listener.cancel()
                            }
                            QQC2.ToolButton {
                                icon.name: "system-run"
                                visible: delegate.model.application !== undefined && delegate.model.application.isInstalled && !delegate.listener.isActive && delegate.model.application.canExecute
                                onClicked: {
                                    delegate.model.application.invokeApplication()
                                    delegate.model.remove(index)
                                }
                            }
                        }
                        QQC2.ProgressBar {
                            Layout.fillWidth: true
                            visible: delegate.listener.isActive
                            value: delegate.listener.progress / 100
                        }
                    }
                }
            }
        }
    }
}
