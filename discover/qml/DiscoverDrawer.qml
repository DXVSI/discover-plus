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

    function createCategoryActions(categories) {
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
            preferredSize = Kirigami.Units.gridUnit * 15
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

    header: ColumnLayout {
        visible: drawer.wideScreen
        spacing: Kirigami.Units.largeSpacing

        // Search field
        SearchField {
            id: searchField
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing

            focus: !Kirigami.InputMethod.willShowOnActive
            visible: window.leftPage && (window.leftPage.searchFor !== null || window.leftPage.hasOwnProperty("search"))
            page: window.leftPage

            onCurrentSearchTextChanged: {
                if (drawer.currentSearchText === currentSearchText) return;
                drawer.currentSearchText = currentSearchText
                var curr = window.leftPage;
                if (pageStack.depth > 1) pageStack.pop()
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
            }

            Keys.onDownPressed: featuredActionListItem.forceActiveFocus(Qt.TabFocusReason)
        }
    }

    topContent: [
        // Home
        SidebarItem {
            id: featuredActionListItem
            action: featuredAction
            iconColor: "#4A90D9"
            visible: enabled && drawer.wideScreen
            Keys.onUpPressed: searchField.forceActiveFocus(Qt.TabFocusReason)
        },
        // Installed
        SidebarItem {
            action: installedAction
            iconColor: "#5CB85C"
            visible: enabled && drawer.wideScreen
        },
        // COPR
        SidebarItem {
            action: coprAction
            iconColor: "#9B59B6"
            visible: enabled && drawer.wideScreen
        },
        // Updates
        SidebarItem {
            objectName: "updateButton"
            action: updateAction
            iconColor: "#F39C12"
            visible: enabled && drawer.wideScreen
            badge: Discover.ResourcesModel.updatesCount > 0 ? Discover.ResourcesModel.updatesCount : 0
        },
        // Sources
        SidebarItem {
            action: sourcesAction
            iconColor: "#6C757D"
        },
        // About
        SidebarItem {
            action: aboutAction
            iconColor: "#17A2B8"
        },

        // Categories section header
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.largeSpacing * 2
            visible: drawer.wideScreen
        },
        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            text: "CATEGORIES"
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1
            color: Kirigami.Theme.disabledTextColor
            visible: drawer.wideScreen
        },
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.smallSpacing
            visible: drawer.wideScreen
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
            icon.name: category?.icon ?? ""
            checked: itsMe
            enabled: {
                if (currentSearchText.length === 0) return true
                const subcats = window?.leftPage?.model?.subcategories
                if (subcats && category) return category.contains(subcats)
                return false
            }

            visible: category?.visible
            onTriggered: {
                if (!window.leftPage.canNavigate) {
                    Navigation.openCategory(categoryPtr, currentSearchText)
                } else {
                    if (pageStack.depth > 1) pageStack.pop()
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
