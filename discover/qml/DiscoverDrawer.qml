/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

Kirigami.GlobalDrawer {
    id: drawer

    property bool wideScreen: false
    property string currentSearchText

    function suggestSearchText(text) {
        if (searchField.visible) {
            searchField.text = text
            forceSearchFieldFocus()
        }
    }

    function forceSearchFieldFocus() {
        if (searchField.visible && wideScreen) {
            searchField.forceActiveFocus();
        }
    }

    function createCategoryActions(categories /*list<Discover.Category>*/) /*list<Kirigami.Action>*/ {
        const ret = []

        for (const c of categories) {
            const category = Discover.CategoryModel.get(c)
            const categoryAction = categoryActionComponent.createObject(drawer, { category: category, categoryPtr: c })
            categoryAction.children = createCategoryActions(category.subcategories)
            ret.push(categoryAction)
        }
        return ret;
    }
    actions: createCategoryActions(Discover.CategoryModel.rootCategories)

    interactiveResizeEnabled: true
    Component.onCompleted: {
        if (app.sidebarWidth > 0) {
            preferredSize = app.sidebarWidth
        } else {
            preferredSize =  Kirigami.Units.gridUnit * 14
        }
    }

    onPreferredSizeChanged: app.sidebarWidth = preferredSize

    padding: 0
    topPadding: undefined
    leftPadding: undefined
    rightPadding: undefined
    bottomPadding: undefined
    verticalPadding: undefined
    horizontalPadding: undefined

    resetMenuOnTriggered: false
    modal: !drawer.wideScreen

    onCurrentSubMenuChanged: {
        if (currentSubMenu) {
            currentSubMenu.trigger()
        } else if (currentSearchText.length > 0) {
            window.leftPage.category = null
        }
    }

    header: Kirigami.AbstractApplicationHeader {
        visible: drawer.wideScreen

        contentItem: SearchField {
            id: searchField

            anchors {
                left: parent.left
                leftMargin: Kirigami.Units.smallSpacing
                right: parent.right
                rightMargin: Kirigami.Units.smallSpacing
            }

            // Give the search field keyboard focus by default, unless it would
            // make the virtual keyboard appear, because we don't want that
            focus: !Kirigami.InputMethod.willShowOnActive

            visible: window.leftPage && (window.leftPage.searchFor !== null || window.leftPage.hasOwnProperty("search"))

            page: window.leftPage

            onCurrentSearchTextChanged: {
                var curr = window.leftPage;

                if (pageStack.depth > 1) {
                    pageStack.pop()
                }

                if (currentSearchText === "" && window.currentTopLevel === "" && !window.leftPage.category) {
                    Navigation.openHome()
                } else if (!curr.hasOwnProperty("search")) {
                    if (currentSearchText) {
                        Navigation.clearStack()
                        Navigation.openApplicationList({ search: currentSearchText })
                    }
                } else {
                    curr.search = currentSearchText;
                    curr.forceActiveFocus()
                }
                drawer.currentSearchText = currentSearchText
            }

            Keys.onDownPressed: featuredActionListItem.forceActiveFocus(Qt.TabFocusReason)
        }
    }

    topContent: [
        ActionListItem {
            id: featuredActionListItem
            action: featuredAction
            visible: enabled && drawer.wideScreen
            Keys.onUpPressed: searchField.forceActiveFocus(Qt.TabFocusReason)
        },
        ActionListItem {
            action: installedAction
            visible: enabled && drawer.wideScreen
        },
        ActionListItem {
            action: coprAction
            visible: enabled && drawer.wideScreen
        },
        ActionListItem {
            objectName: "updateButton"
            action: updateAction
            visible: enabled && drawer.wideScreen
            stateIconName: Discover.ResourcesModel.fetchingUpdatesProgress < 100 ? "view-refresh" : Discover.ResourcesModel.updatesCount > 0 ? "emblem-important" : ""
        },
        ActionListItem {
            action: sourcesAction
        },
        ActionListItem {
            action: aboutAction
        },
        Kirigami.Separator {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
        }
    ]

    footer: ColumnLayout {
        readonly property int transactions: Discover.TransactionModel.count
        readonly property bool currentPageShowsTransactionProgressInline:
               applicationWindow().pageStack.currentItem instanceof ApplicationPage
            || applicationWindow().pageStack.currentItem instanceof ApplicationsListPage
            || applicationWindow().pageStack.currentItem instanceof UpdatesPage

        spacing: 0
        visible: transactions > 1 || (transactions === 1 && !currentPageShowsTransactionProgressInline)

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        ProgressView {
            Layout.fillWidth: true
        }

        states: [
            State {
                name: "full"
                when: drawer.wideScreen
                PropertyChanges { drawer.drawerOpen: true }
            },
            State {
                name: "compact"
                when: !drawer.wideScreen
                PropertyChanges { drawer.drawerOpen: false }
            }
        ]
    }

    Component {
        id: categoryActionComponent
        Kirigami.Action {
            required property Discover.Category category
            required property var categoryPtr

            readonly property bool itsMe: window?.leftPage?.category === category

            text: category?.name ?? ""
            icon.name: category?.icon + "-symbolic" ?? ""
            checked: itsMe
            enabled: (currentSearchText.length === 0
                      || (category?.contains(window?.leftPage?.model?.subcategories) ?? false))

            visible: category?.visible
            onTriggered: {
                if (!window.leftPage.canNavigate) {
                    Navigation.openCategory(categoryPtr, currentSearchText)
                } else {
                    if (pageStack.depth > 1) {
                        pageStack.pop()
                    }
                    pageStack.currentIndex = 0
                    window.leftPage.category = categoryPtr
                }

                if (!drawer.wideScreen && category.subcategories.length === 0) {
                    drawer.close();
                }
            }
        }
    }
}
