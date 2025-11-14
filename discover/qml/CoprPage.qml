/*
 *   SPDX-FileCopyrightText: 2025
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

ApplicationsListPage {
    id: page

    title: i18n("COPR Packages")

    // Show COPR packages from already enabled repos
    originFilter: "COPR"
    allBackends: true

    // Don't allow navigation to other categories within COPR page
    // This ensures proper transition when switching from COPR to other categories
    canNavigate: false

    header: Kirigami.InlineMessage {
        width: parent.width
        type: Kirigami.MessageType.Information
        text: i18n("COPR - Community projects. Note: Packages from COPR repositories are not officially supported by Fedora.")
        visible: true
        showCloseButton: false
    }
}
