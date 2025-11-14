/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
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

    // Modern drawer background
    background: Rectangle {
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.lighter(Kirigami.Theme.backgroundColor, 1.05) }
            GradientStop { position: 0.1; color: Kirigami.Theme.backgroundColor }
            GradientStop { position: 1.0; color: Qt.darker(Kirigami.Theme.backgroundColor, 1.02) }
        }

        // Subtle right border
        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15)
        }
    }

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
        CategoryListItem {
            id: featuredActionListItem
            action: featuredAction
            visible: enabled && drawer.wideScreen
            isCategory: false
            Keys.onUpPressed: searchField.forceActiveFocus(Qt.TabFocusReason)
        },
        CategoryListItem {
            action: installedAction
            visible: enabled && drawer.wideScreen
            isCategory: false
        },
        CategoryListItem {
            action: coprAction
            visible: enabled && drawer.wideScreen
            isCategory: false
            subtitle: i18n("Community packages")
        },
        CategoryListItem {
            objectName: "updateButton"
            action: updateAction
            visible: enabled && drawer.wideScreen
            isCategory: false
            subtitle: Discover.ResourcesModel.updatesCount > 0 ? i18np("%1 update available", "%1 updates available", Discover.ResourcesModel.updatesCount) : ""
        },
        CategoryListItem {
            action: sourcesAction
            isCategory: false
        },
        CategoryListItem {
            action: aboutAction
            isCategory: false
        },
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.1; color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1) }
                GradientStop { position: 0.9; color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1) }
                GradientStop { position: 1.0; color: "transparent" }
            }
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.1; color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15) }
                GradientStop { position: 0.9; color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15) }
                GradientStop { position: 1.0; color: "transparent" }
            }
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
            icon.name: {
                // Custom icons for categories with modern feel
                if (text.toLowerCase().includes("game")) {
                    return "applications-games"
                } else if (text.toLowerCase().includes("development")) {
                    return "applications-development"
                } else if (text.toLowerCase().includes("graphics")) {
                    return "applications-graphics"
                } else if (text.toLowerCase().includes("internet") || text.toLowerCase().includes("network")) {
                    return "applications-internet"
                } else if (text.toLowerCase().includes("multimedia") || text.toLowerCase().includes("audio") || text.toLowerCase().includes("video")) {
                    return "applications-multimedia"
                } else if (text.toLowerCase().includes("office") || text.toLowerCase().includes("productivity")) {
                    return "applications-office"
                } else if (text.toLowerCase().includes("science") || text.toLowerCase().includes("education")) {
                    return "applications-science"
                } else if (text.toLowerCase().includes("system") || text.toLowerCase().includes("utilities")) {
                    return "applications-utilities"
                } else {
                    return category?.icon ?? ""
                }
            }
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
