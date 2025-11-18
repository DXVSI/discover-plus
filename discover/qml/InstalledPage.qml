import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import "." as Local

Local.ApplicationsListPage {
    id: page

    stateFilter: Discover.AbstractResource.Installed
    allBackends: true
    sortProperty: "installedPageSorting"
    sortRole: DiscoverApp.DiscoverSettings.installedPageSorting

    name: search.length > 0 ? i18n("Installed: %1", search) : i18n("Installed")
    compact: true
    showRating: false
    showSize: true
    canNavigate: false

    // Custom header with search field
    listHeader: Rectangle {
        width: page.width
        height: 64
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            height: 48
            radius: 24
            color: "#2B2A2E"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8

                Kirigami.Icon {
                    source: "search"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    color: "#CAC4D0"
                }

                QQC2.TextField {
                    id: installedSearchField
                    Layout.fillWidth: true
                    placeholderText: i18n("Search in installed applications...")
                    font.pixelSize: 14
                    selectByMouse: true

                    background: Rectangle {
                        color: "transparent"
                    }

                    onAccepted: {
                        page.search = text.trim()
                    }

                    Component.onCompleted: {
                        if (page.search.length > 0) {
                            text = page.search
                        }
                    }

                    Connections {
                        target: page
                        function onSearchChanged() {
                            if (installedSearchField.text !== page.search) {
                                installedSearchField.text = page.search
                            }
                        }
                    }
                }

                QQC2.ToolButton {
                    visible: installedSearchField.text.length > 0
                    icon.name: "edit-clear"
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    onClicked: {
                        installedSearchField.text = ""
                        page.search = ""
                        installedSearchField.forceActiveFocus()
                    }
                }
            }
        }
    }
}