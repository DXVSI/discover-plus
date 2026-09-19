/*
 *   SPDX-FileCopyrightText: 2025 JakobDev <jakobdev@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    Layout.fillWidth: true
    Discover.Activatable.active: true

    text: i18nc("@info", "This is a preview of an AppStream file");
}
