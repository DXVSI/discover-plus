/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

QQC2.ItemDelegate {
    id: root

    // type: roles of Discover.ReviewsModel
    required property var model

    property bool compact: false
    property bool separator: true

    signal markUseful(bool useful)

    leftPadding: Kirigami.Units.largeSpacing
    rightPadding: Kirigami.Units.largeSpacing
    bottomPadding: Kirigami.Units.largeSpacing
    background: null

    contentItem: ColumnLayout {
        id: mainContent

        spacing: Kirigami.Units.smallSpacing

        // Header with stars and date of review
        RowLayout {
            Layout.fillWidth: true
            Rating {
                id: rating
                Layout.fillWidth: true
                value: root.model.rating
                starSize: Kirigami.Units.gridUnit
                padding: 0
            }
            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.model.date.toLocaleDateString(Qt.locale(), "MMMM yyyy")
                opacity: 0.75
            }
        }

        // Review title and author
        QQC2.Label {
            id: content
            Layout.fillWidth: true

            elide: Text.ElideRight
            readonly property string author: root.model.reviewer || i18n("unknown reviewer")
            text: root.model.summary ? i18n("<b>%1</b> by %2", root.model.summary, author) : i18n("Comment by %1", author)
        }

        // Review text
        QQC2.Label {
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.smallSpacing

            text: root.model.display
            wrapMode: Text.Wrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: root.model.packageVersion ? i18n("Version: %1", root.model.packageVersion) : i18n("Version: unknown")
            elide: Text.ElideRight
            opacity: 0.75
        }

        Loader {
            Layout.fillWidth: true
            active: !root.compact
            sourceComponent: RowLayout {
                spacing: Kirigami.Units.smallSpacing
                visible: !root.compact

                QQC2.Label {
                    visible: root.model.usefulnessTotal !== 0
                    text: i18n("Votes: %1 out of %2", root.model.usefulnessFavorable, root.model.usefulnessTotal)
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: root.model.usefulnessTotal === 0 ? Kirigami.Units.largeSpacing : 0
                    horizontalAlignment: Text.AlignRight
                    text: i18n("Was this review useful?")
                    elide: Text.ElideLeft
                }

                // Useful/Not Useful buttons
                QQC2.Button {
                    id: yesButton
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                    Layout.alignment: Qt.AlignVCenter

                    text: i18nc("Keep this string as short as humanly possible", "Yes")

                    checkable: true
                    checked: root.model.usefulChoice === Discover.ReviewsModel.Yes
                    onClicked: {
                        noButton.checked = false
                        root.markUseful(true)
                    }
                }
                QQC2.Button {
                    id: noButton
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                    Layout.alignment: Qt.AlignVCenter

                    text: i18nc("Keep this string as short as humanly possible", "No")

                    checkable: true
                    checked: root.model.usefulChoice === Discover.ReviewsModel.No
                    onClicked: {
                        yesButton.checked = false
                        root.markUseful(false)
                    }
                }
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
            visible: root.separator
        }
    }
}
