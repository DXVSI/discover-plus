/*
 *  SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *  SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma Singleton

import QtQml
import org.kde.discover as Discover

QtObject {
    // Initialized from C++
    property DiscoverWindow window

    function clearStack() {
        window.currentTopLevel = "";
        window.pageStack.clear();
    }

    function openApplicationListSource(origin: string) {
        openApplicationList({
            originFilter: origin,
            title: origin,
            allBackends: true,
        });
    }

    function openApplicationMime(mime: string) {
        clearStack();

        const mimeTypeComment = app.mimeTypeComment(mime);
        openApplicationList({
            mimeTypeFilter: mime,
            title: i18n("Resources for '%1'", mimeTypeComment),
        });
    }

    function openCategory(category: string, search = "") {
        clearStack()
        openApplicationList({ category, search })
    }

    function openApplicationList(props) {
        const page = window.pageStack.push(Qt.resolvedUrl("ApplicationsListPage.qml"), props);
        if (props.search === "") {
            page.clearSearch();
        }
    }

    function openApplication(application: Discover.AbstractResource) {
        console.assert(application)
        window.pageStack.push(Qt.resolvedUrl("ApplicationPage.qml"), { application })
    }

    function openExtends(extending: string, appname: string) {
        window.pageStack.push(Qt.resolvedUrl("ApplicationsListPage.qml"), {
            extending,
            title: i18n("Addons for %1", appname),
        });
    }

    function openHome() {
        if (window.globalDrawer.currentSubMenu) {
            window.globalDrawer.resetMenu();
        }
        clearStack();
        const pageUrl = Qt.resolvedUrl(window.topBrowsingComp);
        const page = window.pageStack.push(pageUrl);
        page.clearSearch()
    }
}
