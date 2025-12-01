import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Page {
    id: root
    property int pageIndex: 0
    padding: 8
    title: 'Material 3'

    onPageIndexChanged: {
        m_container.replace(m_container.currentItem, m_drawer.model[pageIndex].source, {});
    }

    Component.onCompleted: {
        pageIndexChanged();
        MD.Token.color.useSysColorSM = false;
    }

    MD.MProp.page: m_page_ctx

    MD.PageContext {
        id: m_page_ctx
        showHeader: true
        headerBackgroundOpacity: 0
    }

    contentItem: RowLayout {
        id: content
        spacing: 12

        MD.StandardDrawer {
            id: m_drawer
            Layout.fillHeight: true

            Behavior on implicitWidth {
                NumberAnimation {
                    duration: 200
                }
            }

            model: [
                {
                    name: 'components',
                    source: 'qrc:/Qcm/Material/Example/Components.qml',
                    icon: MD.Token.icon.widgets
                },
                {
                    name: 'color',
                    source: 'qrc:/Qcm/Material/Example/Color.qml',
                    icon: MD.Token.icon.palette
                },
                {
                    name: 'shape',
                    source: 'qrc:/Qcm/Material/Example/Shape.qml',
                    icon: MD.Token.icon.shapes
                },
                {
                    name: 'elevation',
                    source: 'qrc:/Qcm/Material/Example/Elevation.qml',
                    icon: MD.Token.icon.shadow
                },
                {
                    name: 'typography',
                    source: 'qrc:/Qcm/Material/Example/Typography.qml',
                    icon: MD.Token.icon.description
                },
                {
                    name: 'about',
                    source: 'qrc:/Qcm/Material/Example/About.qml',
                    icon: MD.Token.icon.info
                },
            ]

            onClicked: function (model) {
                root.pageIndex = model.index;
            }
            showDivider: false

            drawerContent: ColumnLayout {
                spacing: 0

                MD.Space {
                    spacing: 16
                }

                Item {
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                }

                ColumnLayout {
                    spacing: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    RowLayout {
                        MD.Label {
                            text: 'Brightness'
                            typescale: MD.Token.typescale.label_large
                        }
                        Item {
                            Layout.fillWidth: true
                        }

                        MD.Switch {
                            checked: !MD.Token.isDarkTheme
                            onToggled: {
                                const tk = MD.Token;
                                if (tk.isDarkTheme) {
                                    tk.themeMode = MD.Enum.Light;
                                } else {
                                    tk.themeMode = MD.Enum.Dark;
                                }
                            }
                        }
                    }

                    MD.HorizontalListView {
                        id: m_palette_view
                        Layout.fillWidth: true
                        expand: true
                        spacing: 8
                        implicitHeight: 40
                        model: MD.PaletteModel {}
                        Component.onCompleted: currentIndex = MD.Token.color.paletteType
                        MD.ActionGroup {
                            id: m_palette_group
                        }
                        delegate: MD.InputChip {
                            required property int index
                            required property var model
                            action: MD.Action {
                                T.ActionGroup.group: m_palette_group
                                icon.name: ''
                                checkable: true
                                checked: m_palette_view.currentIndex == index
                                onTriggered: function (c) {
                                    m_palette_view.currentIndex = index;
                                    MD.Token.color.paletteType = index;
                                }
                                text: model.name
                            }
                        }
                    }

                    MD.Divider {}

                    Grid {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 16
                        rows: 3
                        columns: 3
                        Repeater {
                            model: [
                                {
                                    name: 'Red',
                                    color: '#F44336'
                                },
                                {
                                    name: 'Pink',
                                    color: '#E91E63'
                                },
                                {
                                    name: 'Purple',
                                    color: '#9C27B0'
                                },
                                {
                                    name: 'Indigo',
                                    color: '#3F51B5'
                                },
                                {
                                    name: 'Teal',
                                    color: '#009688'
                                },
                                {
                                    name: 'LightGreen',
                                    color: '#8BC34A'
                                },
                                {
                                    name: 'Yellow',
                                    color: '#FFEB3B'
                                },
                                {
                                    name: 'Amber',
                                    color: '#FFC107'
                                },
                                {
                                    name: 'Oragen',
                                    color: '#FF9800'
                                },
                            ]
                            MD.ColorRadio {
                                size: 28
                                color: modelData.color
                                checked: MD.Token.color.accentColor == color
                                onClicked: {
                                    MD.Token.color.accentColor = color;
                                }
                            }
                        }
                    }
                }
            }
        }

        MD.StackView {
            id: m_container
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            initialItem: Item {}
            MD.MProp.page: m_page_ctx_c

            MD.PageContext {
                id: m_page_ctx_c
                showHeader: false
                backgroundRadius: 12
                showBackground: true
            }
        }
    }
}
