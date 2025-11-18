/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.Material
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import "." as Local

Local.DiscoverPage {
    id: page

    readonly property var model: appsModel
    property alias category: appsModel.filteredCategory
    property alias sortRole: appsModel.sortRole
    property alias sortOrder: appsModel.sortOrder
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property alias extending: appsModel.extending
    property alias search: appsModel.search
    property alias resourcesUrl: appsModel.resourcesUrl
    property alias busy: appsModel.busy
    property alias allBackends: appsModel.allBackends
    property int count: {
        if (!viewLoader.item) return 0
        if (viewLoader.item.hasOwnProperty("count")) {
            const itemCount = viewLoader.item.count
            if (typeof itemCount === "object" && itemCount.hasOwnProperty("number")) {
                return itemCount.number
            }
            return itemCount || 0
        } else if (viewLoader.item.hasOwnProperty("model") && viewLoader.item.model) {
            const modelCount = viewLoader.item.model.count
            if (typeof modelCount === "object" && modelCount.hasOwnProperty("number")) {
                return modelCount.number
            }
            return modelCount || 0
        }
        return 0
    }
    property var listHeader: viewLoader.item ? viewLoader.item.header : null
    property int listHeaderPositioning: viewLoader.item && viewLoader.item.headerPositioning ? viewLoader.item.headerPositioning : ListView.InlineHeader
    property string sortProperty: "appsListPageSorting"
    property bool showRating: true
    property bool showSize: false
    property bool searchPage: false

    property bool canNavigate: true
    readonly property alias subcategories: appsModel.subcategories
    readonly property Discover.Category categoryObject: Discover.CategoryModel.get(page.category)

    // View mode: "list" or "grid"
    property string viewMode: "list"

    function stripHtml(input) {
        var regex = /(<([^>]+)>)/ig
        return input.replace(regex, "");
    }

    property string name: categoryObject?.name ?? ""

    title: {
        const count = appsModel.count;
        if (search.length > 0 && count.number > 0) {
            if (count.valid) {
                return i18np("Search: %2 - %3 item", "Search: %2 - %3 items", count.number, stripHtml(search), count.string)
            } else {
                return i18n("Search: %1", stripHtml(search))
            }
        } else if (name.length > 0 && count.number > 0) {
            if (count.valid) {
                return i18np("%2 - %1 item", "%2 - %1 items", count.number, name)
            } else {
                return name
            }
        } else {
            if (count.valid && count.number > 0) {
                return i18np("Search - %1 item", "Search - %1 items", count.number)
            } else {
                return i18n("Search")
            }
        }
    }

    signal clearSearch()

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    // Material Design 3 colors
    Material.theme: Material.Dark
    Material.background: "#1C1B1F"
    Material.foreground: "#E6E1E5"

    onSearchChanged: {
        if (search.length > 0) {
            appsModel.tempSortRole = Discover.ResourcesProxyModel.SearchRelevanceRole
        } else {
            appsModel.tempSortRole = -1
        }
    }

    supportsRefreshing: true
    onRefreshingChanged: if (refreshing) {
        appsModel.invalidateFilter()
        refreshing = false
    }

    QQC2.ActionGroup {
        id: sortGroup
        exclusive: true
    }

    actions: [
        Kirigami.Action {
            text: i18n("Sort: %1", sortGroup.checkedAction.text)
            icon.name: "view-sort"
            Kirigami.Action {
                visible: appsModel.search.length > 0
                QQC2.ActionGroup.group: sortGroup
                text: i18nc("Search results most relevant to the search query", "Relevance")
                icon.name: "file-search-symbolic"
                onTriggered: {
                    appsModel.tempSortRole = Discover.ResourcesProxyModel.SearchRelevanceRole
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.SearchRelevanceRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Name")
                icon.name: "sort-name"
                onTriggered: {
                    DiscoverApp.DiscoverSettings[page.sortProperty] = Discover.ResourcesProxyModel.NameRole
                    appsModel.tempSortRole = -1
                    appsModel.invalidateFilter()
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.NameRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Rating")
                icon.name: "rating"
                onTriggered: {
                    DiscoverApp.DiscoverSettings[page.sortProperty] = Discover.ResourcesProxyModel.SortableRatingRole
                    appsModel.tempSortRole = -1
                    appsModel.invalidateFilter()
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.SortableRatingRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Size")
                icon.name: "download"
                onTriggered: {
                    DiscoverApp.DiscoverSettings[page.sortProperty] = Discover.ResourcesProxyModel.SizeRole
                    appsModel.tempSortRole = -1
                    appsModel.invalidateFilter()
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.SizeRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Release date")
                icon.name: "change-date-symbolic"
                onTriggered: {
                    DiscoverApp.DiscoverSettings[page.sortProperty] = Discover.ResourcesProxyModel.ReleaseDateRole
                    appsModel.tempSortRole = -1
                    appsModel.invalidateFilter()
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.ReleaseDateRole
            }
        }
    ]

    // Header with view mode toggle
    header: Rectangle {
        width: parent.width
        height: 56
        color: "#1C1B1F"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 16

            Item {
                Layout.fillWidth: true
            }

            // Material Design 3 styled toggle
            Rectangle {
                id: viewModeToggle
                Layout.alignment: Qt.AlignRight
                width: 100
                height: 40
                radius: 20
                color: "#2B2A2E"

                border.width: 1
                border.color: Qt.rgba(255, 255, 255, 0.1)

                Row {
                    anchors.fill: parent

                    // List view button
                    Rectangle {
                        width: parent.width / 2
                        height: parent.height
                        radius: 20
                        color: page.viewMode === "list" ? "#D0BCFF" : "transparent"

                        Behavior on color {
                            ColorAnimation { duration: 200; easing.type: Easing.InOutQuad }
                        }

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: 20
                            height: 20
                            source: "view-list-details"
                            color: page.viewMode === "list" ? "#1C1B1F" : "#E6E1E5"

                            Behavior on color {
                                ColorAnimation { duration: 200 }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                page.viewMode = "list"
                            }
                        }
                    }

                    // Grid view button
                    Rectangle {
                        width: parent.width / 2
                        height: parent.height
                        radius: 20
                        color: page.viewMode === "grid" ? "#D0BCFF" : "transparent"

                        Behavior on color {
                            ColorAnimation { duration: 200; easing.type: Easing.InOutQuad }
                        }

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: 20
                            height: 20
                            source: "view-grid"
                            color: page.viewMode === "grid" ? "#1C1B1F" : "#E6E1E5"

                            Behavior on color {
                                ColorAnimation { duration: 200 }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                page.viewMode = "grid"
                            }
                        }
                    }
                }
            }
        }
    }

    // Model
    Discover.ResourcesProxyModel {
        id: appsModel
        property int tempSortRole: -1
        sortRole: tempSortRole >= 0 ? tempSortRole : (DiscoverApp.DiscoverSettings[page.sortProperty] || Discover.ResourcesProxyModel.NameRole)
        sortOrder: sortRole === Discover.ResourcesProxyModel.NameRole ? Qt.AscendingOrder : Qt.DescendingOrder

        onBusyChanged: {
            if (busy && viewLoader.item) {
                if (viewLoader.item.hasOwnProperty("currentIndex")) {
                    viewLoader.item.currentIndex = -1
                }
            }
        }
    }

    // Dynamic view loader
    Loader {
        id: viewLoader
        anchors.fill: parent
        sourceComponent: page.viewMode === "grid" ? gridViewComponent : listViewComponent

        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
    }

    // List view component
    Component {
        id: listViewComponent

        Kirigami.CardsListView {
            id: appsView
            footerPositioning: ListView.InlineFooter
            activeFocusOnTab: true
            currentIndex: -1
            focus: true

            // Better spacing for full width cards
            spacing: 8

            footer: Item {
                id: appViewFooter
                height: appsModel.busy ? Kirigami.Units.gridUnit * 8 : Kirigami.Units.gridUnit
                width: parent.width
            }

            onActiveFocusChanged: if (activeFocus && currentIndex === -1) {
                currentIndex = 0;
            }

            model: appsModel

            delegate: Local.ApplicationDelegate {
                width: ListView.view.width - 16
                compact: !applicationWindow().wideScreen
                showRating: page.showRating
                showSize: page.showSize
            }
        }
    }

    // Grid view component
    Component {
        id: gridViewComponent

        GridView {
            id: appsGrid
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 8
            anchors.bottomMargin: 8

            readonly property int columns: Math.max(2, Math.floor((width - anchors.leftMargin - anchors.rightMargin) / 240))
            cellWidth: Math.floor((width - anchors.leftMargin - anchors.rightMargin) / columns)
            cellHeight: 280

            model: appsModel
            currentIndex: -1
            focus: true
            clip: true

            // Performance optimizations
            cacheBuffer: cellHeight * 4

            // Add property for count compatibility
            property int count: {
                if (!model) return 0
                const modelCount = model.count
                if (typeof modelCount === "object" && modelCount.hasOwnProperty("number")) {
                    return modelCount.number
                }
                return modelCount || 0
            }

            // Scroll indicators
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                active: true
                policy: QQC2.ScrollBar.AsNeeded
            }

            footer: Item {
                height: appsModel.busy ? Kirigami.Units.gridUnit * 8 : Kirigami.Units.gridUnit
                width: parent.width
            }

            delegate: Item {
                required property var model
                required property int index

                width: GridView.view.cellWidth
                height: GridView.view.cellHeight

                Local.GridApplicationDelegate {
                    anchors.fill: parent
                    anchors.margins: 8

                    application: model ? model.application : null
                    index: parent.index
                    count: appsGrid.count
                    columns: appsGrid.columns
                }
            }
        }
    }

    // Empty state and loading indicators
    Item {
        readonly property bool nothingFound: {
            const itemCount = page.count
            return itemCount === 0 && !appsModel.busy && !Discover.ResourcesModel.isInitializing && (!page.searchPage || appsModel.search.length > 0)
        }

        anchors.fill: parent
        opacity: nothingFound ? 1 : 0
        visible: opacity > 0
        z: nothingFound ? 1 : -1
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad } }

        Kirigami.PlaceholderMessage {
            visible: !searchedForThingNotFound.visible
            anchors.centerIn: visible ? parent : undefined
            width: parent.width - (Kirigami.Units.largeSpacing * 8)

            icon.name: "edit-none"
            text: i18n("Nothing found")
        }

        Kirigami.PlaceholderMessage {
            id: searchedForThingNotFound

            Kirigami.Action {
                id: searchAllCategoriesAction
                text: i18nc("@action:button", "Search in All Categories")
                icon.name: "search"
                onTriggered: {
                    window.globalDrawer.resetMenu();
                    Local.Navigation.clearStack()
                    Local.Navigation.openApplicationList({ search: page.search });
                }
            }
            Kirigami.Action {
                id: searchTheWebAction
                text: i18nc("@action:button %1 is the name of an application", "Search the Web for \"%1\"", appsModel.search)
                icon.name: "internet-web-browser"
                onTriggered: {
                    const searchTerm = encodeURIComponent("Linux " + appsModel.search);
                    Qt.openUrlExternally(app.searchUrl(searchTerm));
                }
            }

            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 8)

            visible: appsModel.search.length > 0 && stateFilter !== Discover.AbstractResource.Installed

            icon.name: "edit-none"
            text: page.categoryObject ? i18nc("@info:placeholder %1 is the name of an application; %2 is the name of a category of apps or add-ons",
                                        "\"%1\" was not found in the \"%2\" category", appsModel.search, page.categoryObject.name)
                                : i18nc("@info:placeholder %1 is the name of an application",
                                        "\"%1\" was not found in the available sources", appsModel.search)
            explanation: page.categoryObject ? "" : i18nc("@info:placeholder %1 is the name of an application", "\"%1\" may be available on the web. Software acquired from the web has not been reviewed by your distributor for functionality or stability. Use with caution.", appsModel.search)

            helpfulAction: page.categoryObject ? searchAllCategoriesAction : searchTheWebAction
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)

        visible: opacity !== 0
        opacity: page.count === 0 && page.searchPage && appsModel.search.length === 0 ? 1 : 0
        z: visible ? 1 : -1
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad } }

        icon.name: "search"
        text: i18n("Search")
    }

    Item {
        id: loadingHolder
        parent: page.viewMode === "list" && viewLoader.item?.footerItem ? viewLoader.item.footerItem : page
        anchors.fill: parent
        visible: appsModel.busy && (page.viewMode === "list" ? (viewLoader.item?.atYEnd ?? false) : true)

        ColumnLayout {
            anchors.centerIn: parent
            opacity: parent.visible ? 0.5 : 0

            Kirigami.Heading {
                id: headingText
                Layout.alignment: Qt.AlignCenter
                level: 2
                text: i18n("Still looking…")
            }

            QQC2.BusyIndicator {
                id: busyIndicator
                Layout.alignment: Qt.AlignCenter
                running: parent.visible
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
            }

            Behavior on opacity {
                PropertyAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
            }
        }
    }
}