/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

DiscoverPage {
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
    property alias count: appsView.count
    property alias listHeader: appsView.header
    property alias listHeaderPositioning: appsView.headerPositioning
    property string sortProperty: "appsListPageSorting"
    property bool showRating: true
    property bool showSize: false
    property bool searchPage: false

    property bool canNavigate: true
    readonly property alias subcategories: appsModel.subcategories
    readonly property Discover.Category categoryObject: Discover.CategoryModel.get(page.category)

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
                    // Do *not* save the sort role on searches
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
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.NameRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Popularity")
                icon.name: "rating"
                onTriggered: {
                    DiscoverApp.DiscoverSettings[page.sortProperty] = Discover.ResourcesProxyModel.SortableRatingRole
                    appsModel.tempSortRole = -1
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
                }
                checkable: true
                checked: appsModel.sortRole === Discover.ResourcesProxyModel.ReleaseDateRole
            }
        }
    ]

    Kirigami.CardsListView {
        id: appsView
        footerPositioning: ListView.InlineFooter
        activeFocusOnTab: true
        currentIndex: -1
        focus: true
        footer: Item {
            id: appViewFooter
            height: appsModel.busy ? Kirigami.Units.gridUnit * 8 : Kirigami.Units.gridUnit
            width: parent.width
        }
        onActiveFocusChanged: if (activeFocus && currentIndex === -1) {
            currentIndex = 0;
        }

        model: Discover.ResourcesProxyModel {
            id: appsModel
            property int tempSortRole: -1
            sortRole: tempSortRole >= 0 ? tempSortRole : DiscoverApp.DiscoverSettings.appsListPageSorting
            sortOrder: sortRole === Discover.ResourcesProxyModel.NameRole ? Qt.AscendingOrder : Qt.DescendingOrder

            onBusyChanged: {
                if (busy) {
                    appsView.currentIndex = -1
                }
            }
        }

        delegate: ApplicationDelegate {
            compact: !applicationWindow().wideScreen
            showRating: page.showRating
            showSize: page.showSize
        }

        Item {
            readonly property bool nothingFound: appsView.count == 0 && !appsModel.busy && !Discover.ResourcesModel.isInitializing && (!page.searchPage || appsModel.search.length > 0)

            anchors.fill: parent
            opacity: nothingFound ? 1 : 0
            visible: opacity > 0
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
                        Navigation.clearStack()
                        Navigation.openApplicationList({ search: page.search });
                    }
                }
                Kirigami.Action {
                    id: searchTheWebAction
                    text: i18nc("@action:button %1 is the name of an application", "Search the Web for \"%1\"", appsModel.search)
                    icon.name: "internet-web-browser"
                    onTriggered: {
                        const searchTerm = encodeURIComponent(Qt.platform.os + " " + appsModel.search);
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

                // If we're in a category, first direct the user to search globally,
                // because they might not have realized they were in a category and
                // therefore the results were limited to just what was in the category
                helpfulAction: page.categoryObject ? searchAllCategoriesAction : searchTheWebAction
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)

            visible: opacity !== 0
            opacity: appsView.count === 0 && page.searchPage && appsModel.search.length === 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad } }

            icon.name: "search"
            text: i18n("Search")
        }

        Item {
            id: loadingHolder
            parent: appsView.count === 0 ? appsView : appsView.footerItem
            anchors.fill: parent
            visible: appsModel.busy && appsView.atYEnd
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
}
