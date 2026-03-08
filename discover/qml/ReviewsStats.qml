/*
 *   SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

BasicAbstractCard {
    id: root

    required property Discover.AbstractResource application
    required property Discover.ReviewsModel reviewsModel
    required property Discover.ReviewsModel model
    required property int visibleReviews
    required property bool compact

    content: GridLayout {
        rows: root.compact ? 6 : 5
        columns: root.compact ? 2 : 3
        flow: GridLayout.TopToBottom
        rowSpacing: 0
        columnSpacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.maximumWidth: globalRating.implicitWidth
            Layout.fillHeight: true
            Layout.rowSpan: 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            minimumPointSize: 10
            font.pointSize: 70
            fontSizeMode: Text.Fit
            text: (Math.min(10, root.application.rating.rating) / 2.0).toFixed(1)
        }
        Rating {
            id: globalRating
            Layout.alignment: Qt.AlignCenter
            value: root.application.rating.rating
            precision: Rating.Precision.HalfStar
        }
        QQC2.Label {
            Layout.alignment: Qt.AlignCenter
            text: i18nc("how many reviews", "%1 reviews", root.application.rating.ratingCount)
        }
        Repeater {
            model: [5, 4, 3, 2, 1]
            delegate: RowLayout {
                id: delegate

                required property int index
                required property string modelData

                Layout.fillWidth: true
                Layout.preferredHeight: Math.ceil(Math.max(globalRating.height, implicitHeight))
                Layout.row: index
                Layout.column: 1
                QQC2.Label {
                    id: numberLabel
                    text: modelData
                    Layout.preferredWidth: Math.ceil(Math.max(implicitWidth, numberMetrics.width))
                    Layout.alignment: Qt.AlignCenter
                    horizontalAlignment: Text.AlignHCenter
                    TextMetrics {
                        id: numberMetrics
                        font: numberLabel.font
                        text: i18nc("widest character in the language", "M")
                    }
                }
                QQC2.ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: root.application.rating.starCounts[modelData] / root.application.rating.ratingCount
                    // This is to make the progressbar right margin from the card edge exactly the same as the top one
                    rightInset: topInset - Math.round((height - topInset - bottomInset) / 2) + Math.round((parent.height - height) / 2)
                }
            }
        }
        ListView {
            id: reviewsPreview
            Layout.row: root.compact ? 5 : 0
            Layout.column: root.compact ? 0 : 2
            Layout.columnSpan: root.compact ? 2 : 1
            Layout.rowSpan: 5
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 18
            Layout.preferredHeight: Kirigami.Units.gridUnit * 8
            visible: count > 0
            clip: true
            orientation: ListView.Horizontal
            currentIndex: 0
            pixelAligned: true
            snapMode: ListView.SnapToItem
            highlightRangeMode: ListView.StrictlyEnforceRange

            displayMarginBeginning: 0
            displayMarginEnd: 0

            preferredHighlightBegin: currentItem ? Math.round((width - currentItem.width) / 2) : 0
            preferredHighlightEnd: currentItem ? preferredHighlightBegin + currentItem.width : 0

            highlightMoveDuration: Kirigami.Units.longDuration
            highlightResizeDuration: Kirigami.Units.longDuration

            // Only show reviews here that someone cosidered useful to show
            model: DiscoverApp.LimitedRowCountProxyModel {
                sourceModel: KItemModels.KSortFilterProxyModel {
                    id: sortModel
                    sourceModel: root.model
                    filterRoleName: "usefulnessFavorable"
                    filterRowCallback: (sourceRow, sourceParent) => {
                        const index = sourceModel.index(sourceRow, 0, sourceParent);
                        const shouldShow = sourceModel.data(index, Discover.ReviewsModel.ShouldShow)
                        return shouldShow === true && sourceModel.data(index, Discover.ReviewsModel.UsefulnessFavorable) > 0;
                    }
                    // need to do it afterwads as direct binding won't work, because at startup sortRoleName will be empty
                    onSortRoleNameChanged: sortOrder = Qt.DescendingOrder
                }
                pageSize: visibleReviews
            }
            delegate: Item {
                id: delegate

                required property string summary
                required property string display
                required property string reviewer

                width: reviewsPreview.width
                height: reviewsPreview.height

                ColumnLayout {
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: delegate.summary
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        text: delegate.display
                    }
                    QQC2.Label {
                        Layout.alignment: Qt.AlignCenter
                        opacity: 0.75
                        text: delegate.reviewer || i18n("Unknown reviewer")
                    }
                }
            }

            Timer {
                running: root.visible && !reviewsPreview.moving
                repeat: true
                interval: 10000
                onTriggered: reviewsPreview.currentIndex = (reviewsPreview.currentIndex + 1) % reviewsPreview.count
            }

            QQC2.Label {
                id: openingQuote
                anchors {
                    left: parent.left
                    top: parent.top
                    topMargin: quoteMetrics.boundingRect.top - quoteMetrics.tightBoundingRect.top
                }
                parent: reviewsPreview
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 4
                text: i18nc("Opening upper air quote", "“")
                opacity: 0.4
                TextMetrics {
                    id: quoteMetrics
                    font: openingQuote.font
                    text: openingQuote.text
                }
            }
            QQC2.Label {
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                }
                parent: reviewsPreview
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 4
                text: i18nc("Closing lower air quote", "„")
                opacity: 0.4
            }
            RowLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                parent: reviewsPreview
                MouseArea {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onClicked: reviewsPreview.currentIndex = Math.max(reviewsPreview.currentIndex - 1, 0)
                }
                QQC2.PageIndicator {
                    count: reviewsPreview.count
                    currentIndex: reviewsPreview.currentIndex
                    interactive: true
                    onCurrentIndexChanged: reviewsPreview.currentIndex = currentIndex
                }
                MouseArea {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onClicked: reviewsPreview.currentIndex = Math.min(reviewsPreview.currentIndex + 1, reviewsPreview.count - 1)
                }
            }
        }
    }
}
