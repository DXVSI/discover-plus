/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

Kirigami.OverlaySheet {
    id: page

    property Discover.ReviewsModel model
    property alias sortModel: sortModel
    property alias sortRole: sortModel.sortRoleName

    readonly property Discover.AbstractReviewsBackend reviewsBackend: resource.backend.reviewsBackend
    readonly property Discover.AbstractResource resource: model.resource

    readonly property ReviewDialog rd: ReviewDialog {
        id: reviewDialog

        application: page.resource
        backend: page.reviewsBackend
        onAccepted: backend.submitReview(page.resource, summary, review, rating, name)
    }

    function openReviewDialog() {
        visible = false
        reviewDialog.open()
    }

    implicitWidth: Math.min(Kirigami.Units.gridUnit * 40, Math.max(Kirigami.Units.gridUnit * 30, parent.width * 0.8))

    title: i18n("Reviews for %1", page.resource.name)

    showCloseButton: false

    header: T.Control {
        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                implicitContentHeight + topPadding + bottomPadding)

        padding: 0

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            // title and close button
            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Layout.fillWidth: true

                Kirigami.Heading {
                    id: heading
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: page.title.length === 0 ? " " : page.title // always have text to ensure header height
                    elide: Text.ElideRight

                    // use tooltip for long text that is elided
                    QQC2.ToolTip.visible: truncated && titleHoverHandler.hovered
                    QQC2.ToolTip.text: page.title
                    HoverHandler { id: titleHoverHandler }
                }

                QQC2.ToolButton {
                    id: closeIcon

                    // We want to position the close button in the top-right
                    // corner if the header is very tall, but we want to
                    // vertically center it in a short header
                    readonly property bool tallHeader: parent.height > (Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.largeSpacing * 2)
                    Layout.alignment: tallHeader ? Qt.AlignRight | Qt.AlignTop : Qt.AlignRight | Qt.AlignVCenter
                    Layout.topMargin: tallHeader ? Kirigami.Units.largeSpacing : 0

                    icon.name: closeIcon.hovered ? "window-close" : "window-close-symbolic"
                    text: qsTr("Close", "@action:button close dialog")
                    onClicked: mouse => page.close()
                    display: QQC2.AbstractButton.IconOnly
                }
            }

            // Review row
            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Layout.fillWidth: true

                QQC2.Button {
                    id: reviewButton

                    visible: page.reviewsBackend !== null
                    enabled: page.resource.isInstalled
                    text: i18n("Write a Review…")
                    onClicked: page.openReviewDialog()
                }

                // This layout is intended as a mere container of messageLabel, don't put anything else in it
                RowLayout {
                    id: inlineMessageParent
                    spacing: Kirigami.Units.smallSpacing
                    Layout.fillWidth: true
                    QQC2.Label {
                        id: messageLabel
                        Layout.fillWidth: true
                        parent: inlineMessageParent.width >= messageLabelMetrics.width ? inlineMessageParent : newlineMessageParent
                        text: i18n("Install this app to write a review")
                        wrapMode: Text.WordWrap
                        visible: !reviewButton.enabled
                        opacity: 0.75
                        TextMetrics {
                            id: messageLabelMetrics
                            font: messageLabel.font
                            text: messageLabel.text
                        }
                    }
                }

                QQC2.ActionGroup {
                    id: sortGroup
                    exclusive: true
                }
                Kirigami.ActionToolBar {
                    id: actionToolBar
                    Layout.minimumWidth: implicitWidth + 1

                    alignment: Qt.AlignRight
                    Layout.fillWidth: false
                    actions: Kirigami.Action {
                        text: i18n("Sort: %1", sortGroup.checkedAction.text)
                        icon.name: "view-sort-symbolic"
                        displayHint: Kirigami.DisplayHint.KeepVisible
                        Kirigami.Action {
                            text: i18nc("@label:listbox Most relevant reviews", "Most Relevant")
                            QQC2.ActionGroup.group: sortGroup
                            checkable: true
                            checked: sortModel.sortRoleName === "wilsonScore"
                            onTriggered: sortModel.sortRoleName = "wilsonScore"
                        }
                        Kirigami.Action {
                            text: i18nc("@label:listbox Most recent reviews", "Most Recent")
                            QQC2.ActionGroup.group: sortGroup
                            checkable: true
                            checked: sortModel.sortRoleName === "date"
                            onTriggered: sortModel.sortRoleName = "date"
                        }
                        Kirigami.Action {
                            text: i18nc("@label:listbox Reviews with the highest ratings", "Highest Ratings")
                            QQC2.ActionGroup.group: sortGroup
                            checkable: true
                            checked: sortModel.sortRoleName === "rating"
                            onTriggered: sortModel.sortRoleName = "rating"
                        }
                    }
                }
            }

            // This layout is intended as a mere container of messageLabel, don't put anything else in it
            RowLayout {
                id: newlineMessageParent

                spacing: Kirigami.Units.smallSpacing
                visible: children.length > 0

                Layout.fillWidth: true
            }
        }
    }

    ListView {
        id: reviewsView

        model: KItemModels.KSortFilterProxyModel {
            id: sortModel
            sourceModel: page.model
            filterRoleName: "shouldShow"
            filterRowCallback: (sourceRow, sourceParent) => {
                return sourceModel.data(sourceModel.index(sourceRow, 0, sourceParent), filterRole) === true;
            }
            // need to do it afterwads as direct binding won't work, because at startup sortRoleName will be empty
            onSortRoleNameChanged: sortOrder = Qt.DescendingOrder
        }
        clip: true

        topMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.largeSpacing

        spacing: Kirigami.Units.smallSpacing
        implicitWidth: Math.max(Kirigami.Units.gridUnit * 25, Math.round(page.parent.width / 3))
        implicitHeight: Math.max(Kirigami.Units.gridUnit * 25, page.parent.height - Kirigami.Units.gridUnit * 4)

        // Still preload some items to make the scrollbar behave better, but can't preload all the comments as some apps like Firefox have thousands of them which will freeze Discover for minutes
        cacheBuffer: height * 2
        reuseItems: true

        delegate: ReviewDelegate {
            required property int index

            width: ListView.view.width
            separator: index !== ListView.view.count - 1
            onMarkUseful: useful => {
                page.model.markUseful(index, useful);
            }
        }
    }
}
