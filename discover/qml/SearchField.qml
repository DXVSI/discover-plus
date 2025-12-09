/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2019 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover 2.0

Kirigami.SearchField {
    id: root

    // for appium tests
    objectName: "searchField"

    // Search operations are network-intensive, so we can't have search-as-you-type.
    // This means we should turn off auto-accept entirely, rather than having it on
    // with a delay. The result just isn't good. See Bug 445142.
    autoAccept: false

    property QtObject page
    property string currentSearchText

    placeholderText: (!enabled || !page || page.hasOwnProperty("isHome") || window.leftPage.name.length === 0) ? i18n("Search…") : i18n("Search in '%1'…", window.leftPage.name)

    SearchHistory {
        id: searchHistory
    }

    Component.onCompleted: {
        // Инициализируем модель истории при запуске
        historyModel = searchHistory.suggestionsForTerm("");
    }

    onAccepted: {
        text = text.trim();
        currentSearchText = text;
        console.log("SearchField accepted:", text, "currentSearchText:", currentSearchText);
        if (text.length > 0) {
            searchHistory.addSearchTerm(text);
            historyPopup.close();
        }
    }

    function clearText() {
        text = "";
        currentSearchText = "";
    }

    onTextChanged: {
        // Обновляем модель истории на основе текущего текста
        if (text.length === 0) {
            historyModel = searchHistory.suggestionsForTerm("");
            // Если поле стало пустым и есть фокус, показываем историю
            if (activeFocus && historyModel.length > 0) {
                historyPopup.open();
            }
        } else {
            // При вводе текста всегда закрываем историю
            historyPopup.close();
        }
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            // При получении фокуса показываем историю, если поле пустое
            if (text.length === 0 && historyModel.length > 0) {
                historyPopup.open();
            }
        } else {
            // При потере фокуса закрываем историю
            historyPopup.close();
        }
    }

    property var historyModel: []

    function showHistory() {
        historyModel = searchHistory.suggestionsForTerm(text);
        if (historyModel.length > 0) {
            historyPopup.open();
        }
    }

    Popup {
        id: historyPopup
        y: root.height
        width: root.width
        height: Math.min(historyView.contentHeight, 300)
        padding: 0

        background: Kirigami.ShadowedRectangle {
            color: Kirigami.Theme.backgroundColor
            border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.2)
            border.width: 1
            radius: 3

            shadow {
                size: Kirigami.Units.largeSpacing
                color: Qt.rgba(0, 0, 0, 0.2)
                yOffset: 2
            }
        }

        ListView {
            id: historyView
            anchors.fill: parent
            model: root.historyModel
            currentIndex: -1
            clip: true

            delegate: ItemDelegate {
                width: parent ? parent.width : 0
                height: 40

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "view-history"
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    }

                    Label {
                        text: modelData
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    ToolButton {
                        icon.name: "edit-clear"
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        onClicked: {
                            searchHistory.removeSearchTerm(modelData);
                            root.historyModel = searchHistory.suggestionsForTerm("");
                            if (root.historyModel.length === 0) {
                                historyPopup.close();
                            }
                        }
                    }
                }

                onClicked: {
                    root.text = modelData;
                    root.accepted();
                    historyPopup.close();
                }

                hoverEnabled: true
                background: Rectangle {
                    color: parent.hovered ? Kirigami.Theme.hoverColor : "transparent"
                    Kirigami.Theme.inherit: true
                }
            }

            header: Item {
                width: parent ? parent.width : 0
                height: 30

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: Kirigami.Units.smallSpacing
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18n("Recent searches")
                    font.bold: true
                    opacity: 0.7
                }
            }
        }
    }

    // Add keyboard navigation for history
    Keys.onDownPressed: (event) => {
        if (historyPopup.opened && historyView.count > 0) {
            if (historyView.currentIndex < historyView.count - 1) {
                historyView.currentIndex++;
            } else {
                historyView.currentIndex = 0;
            }
            root.text = historyModel[historyView.currentIndex];
            event.accepted = true;
        }
    }

    Keys.onUpPressed: (event) => {
        if (historyPopup.opened && historyView.count > 0) {
            if (historyView.currentIndex > 0) {
                historyView.currentIndex--;
            } else {
                historyView.currentIndex = historyView.count - 1;
            }
            root.text = historyModel[historyView.currentIndex];
            event.accepted = true;
        }
    }

    Keys.onReturnPressed: (event) => {
        if (historyPopup.opened && historyView.currentIndex >= 0) {
            root.text = historyModel[historyView.currentIndex];
            root.accepted();
            historyPopup.close();
            event.accepted = true;
        } else if (!historyPopup.opened) {
            // Если история не открыта, выполняем поиск
            root.accepted();
            event.accepted = true;
        }
    }

    Keys.onEnterPressed: (event) => {
        // Обрабатываем Enter с цифровой клавиатуры так же
        if (historyPopup.opened && historyView.currentIndex >= 0) {
            root.text = historyModel[historyView.currentIndex];
            root.accepted();
            historyPopup.close();
            event.accepted = true;
        } else if (!historyPopup.opened) {
            root.accepted();
            event.accepted = true;
        }
    }

    Keys.onEscapePressed: (event) => {
        if (historyPopup.opened) {
            historyPopup.close();
            event.accepted = true;
        }
    }

    Connections {
        ignoreUnknownSignals: true
        target: root.page

        function onClearSearch() {
            root.clearText();
        }
    }

    Connections {
        target: applicationWindow()
        function onCurrentTopLevelChanged() {
            if (applicationWindow().currentTopLevel.length > 0) {
                root.clearText();
            }
        }
    }
}