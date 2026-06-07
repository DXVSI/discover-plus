/*
 *   SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
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

/*!
  \qmltype ApplicationPageStickyHeader
  \inqmlmodule org.kde.discover

  \brief A sticky header for use in ApplicationPage
 */
Item {
    id: root

    /*!
     * The flickable containing this header
     */
    required property Flickable flickable

    /*!
     * The component to be used for the full-height header
     */
    required property Component fullComponent

    /*!
     * The component to be used when the header is sticky
     */
    required property Component stickyComponent

    /*!
     * The background color of the header
     */
    property color color: Kirigami.Theme.backgroundColor

    /*!
     * Helper function to immediately scroll the flickable
     * to the top, avoiding awkward sticky header behavior
     */
    function scrollToTop(): void {
        stickyHeaderFullOpacityBehavior.enabled = false;
        stickyHeaderStickyOpacityBehavior.enabled = false;

        // If settle timer has triggered and started a flick, it'll interfere
        root.flickable.cancelFlick();

        root.flickable.contentY = 0;

        stickyHeaderFullOpacityBehavior.enabled = true;
        stickyHeaderStickyOpacityBehavior.enabled = true;
    }

    implicitHeight: stickyHeader.implicitHeight

    Item {
        id: stickyHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: -stickyHeaderSeparator.height

        // root exists in the page's content, in a layout; this exists in the
        // flickable itself and contains the actual header anchored to the top
        // ignoring scrolling — root reserves the height for us when not sticky
        parent: root.flickable

        implicitHeight: stickyHeaderContainer.implicitHeight + stickyHeaderSeparator.implicitHeight
        height: stickyHeaderContainer.height + stickyHeaderSeparator.height

        Rectangle {
            id: stickyHeaderContainer
            anchors.left: parent.left
            anchors.right: parent.right

            clip: true

            color: root.color

            readonly property real fullHeight: stickyHeaderFullLoader.implicitHeight + (stickyHeaderFullLoader.anchors.margins * 2)
            readonly property real stickyHeight: stickyHeaderStickyLoader.implicitHeight + (stickyHeaderStickyLoader.anchors.margins * 2)

            // NOTE: It does not appear that we need to account for
            // flickable.originY as the page does not use a ListView/GridView.
            // Future upstreaming or copying this elsewhere might need to
            // consider this.

            implicitHeight: fullHeight
            height: Math.max(stickyHeight, fullHeight - root.flickable.contentY)

            readonly property bool isSticky: height < fullHeight

            // Store flickable's contentY values so we can determine which way
            // the user has been scrolling, so we know which way to settle
            readonly property list<real> yHistory: []

            onHeightChanged: {
                stickyHeaderContainer.yHistory = [
                    root.flickable.contentY,
                    ...stickyHeaderContainer.yHistory.slice(0, 10)
                ];

                settleTimer.restart();
            }

            // After a short delay, if the contentY leaves the sticky header
            // height between full and sticky height, flick either to the top
            // for the full header, or down so we have the sticky header without
            // extra height
            Timer {
                id: settleTimer
                // veryLongDuration gives us a comfortable margin where we can
                // be sure the user has stopped scrolling, but avoid taking too
                // long — longDuration is too short for some particularly casual
                // scrolling
                interval: Kirigami.Units.veryLongDuration

                // NOTE: Ideally we should only settle if the user is not
                // interacting with the touchpad, touching the screen or holding
                // the scrollbar, but flickable doesn't give us that info

                onTriggered: {
                    const flickable = root.flickable;

                    const stickyHeightDifference = stickyHeaderContainer.fullHeight - stickyHeaderContainer.stickyHeight;
                    const maxContentY = flickable.contentHeight + flickable.bottomMargin - flickable.height;

                    // contentY between 0 and stickyHeightDifference results in
                    // a sticky header with extra height; maxContentY is
                    // considered as we can't scroll lower than the bottom
                    if (flickable.contentY > 0
                        && Math.round(flickable.contentY) < Math.min(stickyHeightDifference, maxContentY)
                    ) {
                        // The sum of the differences between the most recent 10
                        // contentY values will give us a confident idea of the
                        // direction the user was scrolling before leaving us in
                        // an uncomfortable state, so we know how to correct it
                        // whilst respecting their intention
                        let differencesSum = 0;
                        for (let i = 0; i < stickyHeaderContainer.yHistory.length - 1; ++i) {
                            differencesSum += (stickyHeaderContainer.yHistory[i] - stickyHeaderContainer.yHistory[i + 1]);
                        }

                        if (differencesSum <= 0) {
                            // Go to top, full header
                            flickable.flickTo(Qt.point(0, 0));
                        } else {
                            // Go down, sticky header
                            flickable.flickTo(Qt.point(0, Math.min(stickyHeightDifference, maxContentY)));
                        }
                    }
                }
            }

            Loader {
                id: stickyHeaderFullLoader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                z: stickyHeaderContainer.isSticky ? 1 : 3
                opacity: stickyHeaderContainer.isSticky ? 0 : 1
                Behavior on opacity {
                    id: stickyHeaderFullOpacityBehavior
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.InOutQuad
                    }
                }
                enabled: opacity > 0

                sourceComponent: root.fullComponent
            }

            Loader {
                id: stickyHeaderStickyLoader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                z: stickyHeaderContainer.isSticky ? 3 : 1
                opacity: stickyHeaderContainer.isSticky ? 1 : 0
                Behavior on opacity {
                    id: stickyHeaderStickyOpacityBehavior
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.InOutQuad
                    }
                }
                enabled: opacity > 0

                sourceComponent: root.stickyComponent
            }

            MouseArea {
                anchors.fill: parent
                anchors.bottomMargin: -stickyHeaderSeparator.height // Go under the separator too
                z: 2
                hoverEnabled: true
            }
        }

        Kirigami.Separator {
            id: stickyHeaderSeparator
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }
}
