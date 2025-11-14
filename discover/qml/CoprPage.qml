/*
 *   SPDX-FileCopyrightText: 2025
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.discover as Discover

ApplicationsListPage {
    id: page

    title: i18n("COPR Packages")

    // Filter to show only COPR packages
    originFilter: "COPR"

    // Show all packages from COPR repos
    allBackends: true

    // Enable search mode to show placeholder
    searchPage: true
}
