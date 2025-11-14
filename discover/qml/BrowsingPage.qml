/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

DiscoverPage {
    id: page

    title: i18nc("@title:window the name of a top-level 'home' page", "Home")
    objectName: "featured"

    actions: window.wideScreen ? [ searchAction ] : []

    header: DiscoverInlineMessage {
        id: message

        inlineMessage: Discover.ResourcesModel.inlineMessage ? Discover.ResourcesModel.inlineMessage : app.homePageMessage
    }

    readonly property bool isHome: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    DiscoverApp.FeaturedModel {
        id: featuredModel
    }

    Kirigami.LoadingPlaceholder {
        visible: featuredModel.isFetching
        anchors.centerIn: parent
    }

    Loader {
        active: !featuredModel.isFetching && [featuredModel, popRep, recentlyUpdatedRepeater, gamesRep, devRep].every((model) => model.count === 0)

        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        sourceComponent: Kirigami.PlaceholderMessage {
            readonly property Discover.InlineMessage helpfulError: featuredModel.currentApplicationBackend?.explainDysfunction() ?? null

            icon.name: helpfulError?.iconName ?? ""
            text: i18n("Unable to load applications")
            explanation: helpfulError?.message ?? ""

            Repeater {
                model: helpfulError?.actions ?? null
                delegate: QQC2.Button {
                    id: delegate

                    required property Discover.DiscoverAction modelData

                    Layout.alignment: Qt.AlignHCenter

                    action: ConvertDiscoverAction {
                        action: delegate.modelData
                    }
                }
            }
        }
    }

    signal clearSearch()

    Kirigami.CardsLayout {
        id: apps

        maximumColumns: 4
        rowSpacing: page.padding
        columnSpacing: page.padding
        maximumColumnWidth: Kirigami.Units.gridUnit * 6

        // Modern section header with gradient
        Rectangle {
            id: popHeading
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            Layout.preferredHeight: popHeadingContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            visible: popRep.count > 0 && !featuredModel.isFetching

            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.6, 1, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.3, 0.5, 0.9, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.6, 1, 0.08) }
            }

            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            RowLayout {
                id: popHeadingContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.largeSpacing

                Rectangle {
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.2, 0.6, 1, 0.15)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "rating"
                        width: parent.width * 0.6
                        height: width
                        color: "#3498db"
                    }
                }

                Kirigami.Heading {
                    Layout.fillWidth: true
                    text: i18nc("@title:group", "🔥 Most Popular")
                    wrapMode: Text.Wrap
                    level: 2
                }

                QQC2.Label {
                    text: i18np("%1 app", "%1 apps", popRep.count)
                    opacity: 0.6
                    font: Kirigami.Theme.smallFont
                }
            }
        }

        Repeater {
            id: popRep
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns * 2
                sourceModel: DiscoverApp.OdrsAppsModel {
                    // filter: FOSS
                }
            }
            delegate: GridApplicationDelegate {
                visible: !featuredModel.isFetching
                count: popRep.count
                columns: apps.columns
                maxUp: 0
            }
            property int numberItemsOnLastRow: (count % apps.columns) || apps.columns
        }

        Rectangle {
            id: recentlyUpdatedHeading
            Layout.topMargin: page.padding
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            Layout.preferredHeight: recentlyUpdatedHeadingContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            visible: recentlyUpdatedRepeater.count > 0 && !featuredModel.isFetching

            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.1, 0.8, 0.5, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.2, 0.7, 0.4, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.1, 0.8, 0.5, 0.08) }
            }

            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            RowLayout {
                id: recentlyUpdatedHeadingContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.largeSpacing

                Rectangle {
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.1, 0.8, 0.5, 0.15)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "clock"
                        width: parent.width * 0.6
                        height: width
                        color: "#2ecc71"
                    }
                }

                Kirigami.Heading {
                    Layout.fillWidth: true
                    text: i18nc("@title:group", "✨ Newly Published & Recently Updated")
                    wrapMode: Text.Wrap
                    level: 2
                }

                QQC2.Label {
                    text: i18np("%1 app", "%1 apps", recentlyUpdatedRepeater.count)
                    opacity: 0.6
                    font: Kirigami.Theme.smallFont
                }
            }
        }

        Repeater {
            id: recentlyUpdatedRepeater
            model: recentlyUpdatedModelInstantiator.object
            delegate: GridApplicationDelegate {
                numberItemsOnPreviousLastRow: ((popHeading.visible && popRep.numberItemsOnLastRow) || 0)
                visible: !featuredModel.isFetching
                count: recentlyUpdatedRepeater.count
                columns: apps.columns
            }
            property int numberItemsOnLastRow: (count % apps.columns) || apps.columns
        }

        Instantiator {
            id: recentlyUpdatedModelInstantiator

            active: {
                const backend = Discover.ResourcesModel.currentApplicationBackend;
                if (!backend) {
                    return [];
                }
                // TODO: Add packagekit-backend of rolling distros
                return [
                    "flatpak-backend",
                    "snap-backend",
                ].includes(backend.name);
            }

            DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns * 2
                sourceModel: Discover.ResourcesProxyModel {
                    filteredCategoryName: "All Applications"
                    backendFilter: Discover.ResourcesModel.currentApplicationBackend
                    sortRole: Discover.ResourcesProxyModel.ReleaseDateRole
                    sortOrder: Qt.DescendingOrder
                }
            }
        }

        Rectangle {
            id: featuredHeading
            Layout.topMargin: page.padding
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            Layout.preferredHeight: featuredHeadingContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            visible: featuredRep.count > 0 && !featuredModel.isFetching

            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.9, 0.6, 0.1, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.8, 0.5, 0.2, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.9, 0.6, 0.1, 0.08) }
            }

            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            RowLayout {
                id: featuredHeadingContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.largeSpacing

                Rectangle {
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.9, 0.6, 0.1, 0.15)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "favorite"
                        width: parent.width * 0.6
                        height: width
                        color: "#f39c12"
                    }
                }

                Kirigami.Heading {
                    Layout.fillWidth: true
                    text: i18nc("@title:group", "⭐ Editor's Choice")
                    wrapMode: Text.Wrap
                    level: 2
                }

                QQC2.Label {
                    text: i18np("%1 app", "%1 apps", featuredRep.count)
                    opacity: 0.6
                    font: Kirigami.Theme.smallFont
                }
            }
        }

        Repeater {
            id: featuredRep
            model: featuredModel
            delegate: GridApplicationDelegate {
                numberItemsOnPreviousLastRow: ((recentlyUpdatedHeading.visible && recentlyUpdatedRepeater.numberItemsOnLastRow) ||
                                              (popHeading.visible && popRep.numberItemsOnLastRow) || 0)
                count: featuredRep.count
                columns: apps.columns
                visible: !featuredModel.isFetching
            }
            property int numberItemsOnLastRow: (count % apps.columns) || apps.columns
        }

        Rectangle {
            id: gamesHeading
            Layout.topMargin: page.padding
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            Layout.preferredHeight: gamesHeadingContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            visible: gamesRep.count > 0 && !featuredModel.isFetching

            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.9, 0.3, 0.3, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.8, 0.2, 0.4, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.9, 0.3, 0.3, 0.08) }
            }

            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            RowLayout {
                id: gamesHeadingContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.largeSpacing

                Rectangle {
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.9, 0.3, 0.3, 0.15)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "applications-games"
                        width: parent.width * 0.6
                        height: width
                        color: "#e74c3c"
                    }
                }

                Kirigami.Heading {
                    Layout.fillWidth: true
                    text: i18nc("@title:group", "🎮 Highest-Rated Games")
                    wrapMode: Text.Wrap
                    level: 2
                }

                QQC2.Label {
                    text: i18np("%1 game", "%1 games", gamesRep.count)
                    opacity: 0.6
                    font: Kirigami.Theme.smallFont
                }
            }
        }

        Repeater {
            id: gamesRep
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns
                sourceModel: Discover.ResourcesProxyModel {
                    filteredCategoryName: "Games"
                    backendFilter: Discover.ResourcesModel.currentApplicationBackend
                    sortRole: Discover.ResourcesProxyModel.SortableRatingRole
                    sortOrder: Qt.DescendingOrder
                }
            }
            delegate: GridApplicationDelegate {
                visible: !featuredModel.isFetching
                numberItemsOnPreviousLastRow: ((featuredHeading.visible && featuredRep.numberItemsOnLastRow) ||
                                              (recentlyUpdatedHeading.visible && recentlyUpdatedRepeater.numberItemsOnLastRow ) ||
                                              (popHeading.visible && popRep.numberItemsOnLastRow) || 0)
                count: gamesRep.count
                columns: apps.columns
                maxDown: 1
            }
            property int numberItemsOnLastRow: (count % apps.columns) || apps.columns
        }

        QQC2.Button {
            text: i18nc("@action:button", "See More")
            icon.name: "go-next-view"
            Layout.columnSpan: apps.columns
            onClicked: Navigation.openCategory(Discover.CategoryModel.findCategoryByName("Games"))
            visible: gamesRep.count > 0 && !featuredModel.isFetching
            Keys.onUpPressed: {
                var target = this
                for (var i = 0; i<gamesRep.numberItemsOnLastRow; i++) {
                    target = target.nextItemInFocusChain(false)
                }
                target.forceActiveFocus(Qt.TabFocusReason)
            }
            Keys.onDownPressed: nextItemInFocusChain(true).forceActiveFocus(Qt.TabFocusReason)
            onFocusChanged: {
                if (focus) {
                    page.ensureVisible(this)
                }
            }
        }

        Rectangle {
            id: devHeading
            Layout.topMargin: page.padding
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            Layout.preferredHeight: devHeadingContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            visible: devRep.count > 0 && !featuredModel.isFetching

            radius: Kirigami.Units.smallSpacing
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0.2, 0.4, 0.8, 0.08) }
                GradientStop { position: 0.5; color: Qt.rgba(0.3, 0.5, 0.9, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(0.2, 0.4, 0.8, 0.08) }
            }

            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

            RowLayout {
                id: devHeadingContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                }
                spacing: Kirigami.Units.largeSpacing

                Rectangle {
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    radius: width / 2
                    color: Qt.rgba(0.2, 0.4, 0.8, 0.15)

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: "applications-development"
                        width: parent.width * 0.6
                        height: width
                        color: "#3498db"
                    }
                }

                Kirigami.Heading {
                    Layout.fillWidth: true
                    text: i18nc("@title:group", "💻 Highest-Rated Developer Tools")
                    wrapMode: Text.Wrap
                    level: 2
                }

                QQC2.Label {
                    text: i18np("%1 tool", "%1 tools", devRep.count)
                    opacity: 0.6
                    font: Kirigami.Theme.smallFont
                }
            }
        }

        Repeater {
            id: devRep
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns
                sourceModel: Discover.ResourcesProxyModel {
                    filteredCategoryName: "Development"
                    backendFilter: Discover.ResourcesModel.currentApplicationBackend
                    sortRole: Discover.ResourcesProxyModel.SortableRatingRole
                    sortOrder: Qt.DescendingOrder
                }
            }
            delegate: GridApplicationDelegate {
                visible: !featuredModel.isFetching
                numberItemsOnPreviousLastRow: ((gamesHeading.visible && gamesRep.numberItemsOnLastRow) ||
                                              (featuredHeading.visible && featuredRep.numberItemsOnLastRow) ||
                                              (recentlyUpdatedHeading.visible && recentlyUpdatedRepeater.numberItemsOnLastRow) ||
                                              (popHeading.visible && popRep.numberItemsOnLastRow) || 0)
                count: devRep.count
                columns: apps.columns
                maxUp: 1
                maxDown: 1
            }
            property int numberItemsOnLastRow: (count % apps.columns) || apps.columns
        }

        QQC2.Button {
            text: i18nc("@action:button", "See More")
            icon.name: "go-next-view"
            Layout.columnSpan: apps.columns
            onClicked: Navigation.openCategory(Discover.CategoryModel.findCategoryByName("Development"))
            visible: devRep.count > 0 && !featuredModel.isFetching
            Keys.onUpPressed: {
                var target = this
                for (var i = 0; i<devRep.numberItemsOnLastRow; i++) {
                    target = target.nextItemInFocusChain(false)
                }
                target.forceActiveFocus(Qt.TabFocusReason)
            }
            onFocusChanged: {
                if (focus) {
                    page.ensureVisible(this)
                }
            }
        }
    }
}
