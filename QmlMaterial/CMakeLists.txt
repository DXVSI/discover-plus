cmake_minimum_required(VERSION 3.14)

set(QM_VERSION 0.1.3)
project(
  QmlMaterial
  VERSION ${QM_VERSION}
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)

option(QM_BUILD_EXAMPLE ON)

set(QM_ICON_FONT_URL
    ""
    CACHE
      STRING
      "default empty, there is an already embed file: qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2"
)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules")
list(APPEND QML_DIRS "${QT_QML_OUTPUT_DIRECTORY}")

if(UNIX)
  include(GNUInstallDirs)

  if("${QT_INSTALL_QML}" STREQUAL "")
    set(QT_INSTALL_QML "${CMAKE_INSTALL_LIBDIR}/qt6/qml")
  endif()
endif()

set(QML_INSTALL_DIR
    "${QT_INSTALL_QML}"
    CACHE STRING "")
message("QML_INSTALL_DIR: ${QML_INSTALL_DIR}")

# check lto
include(CheckIPOSupported)
check_ipo_supported(RESULT is_ipo_supported OUTPUT lto_error)
if(is_ipo_supported
   AND NOT EMSCRIPTEN
   AND NOT WIN32)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
endif()

if(UNIX AND NOT APPLE)
  set(LINUX CACHE INTERNAL TRUE "")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Gui ShaderTools DBus)

if(Qt6Quick_VERSION VERSION_GREATER_EQUAL 6.10)
  find_package(Qt6 REQUIRED COMPONENTS QuickPrivate)
endif()

find_package(PkgConfig REQUIRED)
if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0004 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBNAME qml_material)

set(SOURCES
    include/qml_material/helper.hpp
    include/qml_material/util/loggingcategory.hpp
    include/qml_material/scenegraph/geometry.h
    include/qml_material/scenegraph/rectangle_material.h
    include/qml_material/scenegraph/elevation_material.h
    src/scenegraph/geometry.cpp
    src/scenegraph/rectangle_material.cpp
    src/scenegraph/elevation_material.cpp
    src/kirigami/displayhint.cpp
    src/kirigami/toolbarlayout.cpp
    src/kirigami/toolbarlayoutdelegate.cpp
    src/kirigami/wheelhandler.cpp
    src/enum.cpp
    src/theme.cpp
    src/helper.cpp
    src/anim/circular_indicator_updator.cpp
    src/anim/linear_indicator_updator.cpp
    src/util/corner.cpp
    src/util/pool.cpp
    src/util/input_block.cpp
    src/util/image.cpp
    src/util/qml_util.cpp
    src/util/page_context.cpp
    src/util/width_provider.cpp
    src/util/color_picker.cpp
    src/model/palette_model.cpp
    src/model/snake_model.cpp
    src/token/color.cpp
    src/token/icon.cpp
    src/token/token.cpp
    src/token/type_scale.cpp
    src/item/item_proxy.cpp
    src/item/round_item.cpp
    src/item/state.cpp
    src/item/rectangle.cpp
    src/item/elevation.cpp)

set(QML_FILES
    qml/Token.qml
    qml/Util.qml
    qml/control/Button.qml
    qml/control/Control.qml
    qml/control/ComboBox.qml
    qml/control/ButtonGroup.qml
    qml/control/CheckBox.qml
    qml/control/RadioButton.qml
    qml/control/Dialog.qml
    qml/control/DialogButtonBox.qml
    qml/control/Drawer.qml
    qml/control/StackView.qml
    qml/control/SplitView.qml
    qml/control/Slider.qml
    qml/control/Menu.qml
    qml/control/MenuItem.qml
    qml/control/Pane.qml
    qml/control/Page.qml
    qml/control/PageIndicator.qml
    qml/control/Action.qml
    qml/control/ActionGroup.qml
    qml/control/MenuSeparator.qml
    qml/control/ToolSeparator.qml
    qml/control/ApplicationWindow.qml
    qml/control/Popup.qml
    qml/control/Switch.qml
    qml/control/TabBar.qml
    qml/control/TabButton.qml
    qml/control/TableViewDelegate.qml
    qml/control/ScrollBar.qml
    qml/control/ScrollIndicator.qml
    qml/control/ItemDelegate.qml
    qml/control/HorizontalHeaderView.qml
    qml/control/HorizontalHeaderViewDelegate.qml
    qml/control/Label.qml
    qml/component/MState.qml
    qml/component/Image.qml
    qml/component/Loader.qml
    qml/component/FontMetrics.qml
    qml/component/Text.qml
    qml/component/TextEdit.qml
    qml/component/TextInput.qml
    qml/component/TextMetrics.qml
    qml/component/TextField.qml
    qml/component/appbar/ActionMenu.qml
    qml/component/appbar/SearchBar.qml
    qml/component/appbar/ActionToolBar.qml
    qml/component/appbar/SnakeBar.qml
    qml/component/appbar/AppBar.qml
    qml/component/appbar/BarItem.qml
    qml/component/button/BusyIconButton.qml
    qml/component/button/BusyButton.qml
    qml/component/button/FAB.qml
    qml/component/button/IconButton.qml
    qml/component/button/StandardIconButton.qml
    qml/component/button/SmallIconButton.qml
    qml/component/button/RadioIndicatorShape.qml
    qml/component/checkbox/CheckShape.qml
    qml/component/card/Card.qml
    qml/component/chip/AssistChip.qml
    qml/component/chip/EmbedChip.qml
    qml/component/chip/FilterChip.qml
    qml/component/chip/InputChip.qml
    qml/component/chip/SuggestionChip.qml
    qml/component/navigation/RailItem.qml
    qml/component/navigation/StandardDrawer.qml
    qml/component/navigation/DrawerItem.qml
    qml/component/progressindicator/CircleProgressBar.qml
    qml/component/progressindicator/CircularIndicator.qml
    qml/component/progressindicator/CircleProgressShape.qml
    qml/component/progressindicator/CircleIndicatorShape.qml
    qml/component/progressindicator/LinearIndicator.qml
    qml/component/progressindicator/LinearIndicatorShape.qml
    qml/component/textfield/FloatingPlaceholderText.qml
    qml/component/textfield/FilledTextFieldShape.qml
    qml/component/textfield/OutlineTextFieldShape.qml
    qml/component/extra/ActionLabel.qml
    qml/component/extra/ColorRadio.qml
    qml/component/extra/Divider.qml
    qml/component/extra/DragHandle.qml
    qml/component/extra/IconLabel.qml
    qml/component/extra/Icon.qml
    qml/component/extra/IconSvg.qml
    qml/component/extra/Space.qml
    qml/component/extra/SdfRectangle.qml
    qml/component/extra/SnakeView.qml
    qml/delegate/ListItem.qml
    qml/delegate/ListGridBaseDelegate.qml
    qml/flickable/Flickable.qml
    qml/flickable/VerticalFlickable.qml
    qml/flickable/VerticalGridView.qml
    qml/flickable/VerticalListView.qml
    qml/flickable/HorizontalListView.qml
    qml/flickable/FlickablePane.qml
    qml/flickable/TableView.qml
    qml/flickable/GridView.qml
    qml/flickable/ListView.qml
    qml/motion/FadeInMotion.qml
    qml/motion/FadeOutMotion.qml
    qml/motion/FadeInThroughMotion.qml
    qml/motion/FadeOutThroughMotion.qml
    qml/state/StateAppBar.qml
    qml/state/StateSnakeBar.qml
    qml/state/StateButton.qml
    qml/state/StateCard.qml
    qml/state/StateDragHandle.qml
    qml/state/StateDrawerItem.qml
    qml/state/StateTabButton.qml
    qml/state/StateTableViewDelegate.qml
    qml/state/StateListItem.qml
    qml/state/StateSlider.qml
    qml/state/StateSwitch.qml
    qml/state/StateIconButton.qml
    qml/state/StateStandardIconButton.qml
    qml/state/StateFAB.qml
    qml/state/StateAssistChip.qml
    qml/state/StateFilterChip.qml
    qml/state/StateInputChip.qml
    qml/state/StateSuggestionChip.qml
    qml/state/StateBarItem.qml
    qml/state/StateSearchBar.qml
    qml/state/StateTextField.qml
    qml/state/StateRailItem.qml
    qml/state/StateEmbedChip.qml
    qml/state/StateComboBox.qml
    qml/state/StateMenuItem.qml
    qml/state/StateRadioButton.qml
    qml/state/StateCheckBox.qml
    qml/internal/ListBusyFooter.qml
    qml/internal/Ripple2.qml
    qml/internal/CursorDelegate.qml
    qml/internal/SliderHandle.qml
    qml/internal/TextFieldEmbed.qml
    qml/internal/RoundClip.qml
    qml/internal/Elevation.qml
    qml/internal/Rectangle.qml
    qml/internal/ElevationRectangle.qml
    qml/internal/Shape.qml
    qml/internal/RoundPath.qml
    qml/internal/RectPath.qml
    qml/internal/ItemHolder.qml)

set(RESOURCES assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2)

set(QML_REG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/anim"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/item"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/token"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/model"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/util")

set(SHADER_DEFINES)

set(QML_MATERIAL_BUILD_TYPE SHARED)

set(QM_ICON_FILL_0_FONT_FILE)
set(QM_ICON_FILL_1_FONT_FILE)

if(EMSCRIPTEN)
  list(APPEND SOURCES src/platform/wasm/wasm.cpp)
  list(APPEND SHADER_DEFINES "GLES=1")
  set(QM_ICON_FILL_0_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.ttf)
  set(QM_ICON_FILL_1_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.ttf)

  set(QML_MATERIAL_BUILD_TYPE STATIC)
  add_compile_options(-flto -oz)
  add_link_options("-sEXPORT_NAME=qml_material" -s FULL_ES3=1 -flto)
else()

  set(QM_ICON_FILL_0_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.woff2)
  set(QM_ICON_FILL_1_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.woff2)

  if(UNIX)
    if(ANDROID)
      list(APPEND SOURCES src/platform/android/android.cpp)
      list(APPEND SHADER_DEFINES "GLES=1")
    else()
      list(APPEND SOURCES src/platform/linux/xdp.cpp)
    endif()
  elseif(WIN32)
    set(QM_ICON_FILL_0_FONT_FILE
        assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.ttf)
    set(QM_ICON_FILL_1_FONT_FILE
        assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.ttf)

    set(QML_MATERIAL_BUILD_TYPE STATIC)
    list(APPEND SOURCES src/platform/win/win.cpp)
  endif()
endif()

list(APPEND RESOURCES "${QM_ICON_FILL_0_FONT_FILE}"
     "${QM_ICON_FILL_1_FONT_FILE}")

set_source_files_properties(qml/Token.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/Util.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
include_directories(include)

qt_add_qml_module(
  ${LIBNAME}
  URI
  Qcm.Material
  ${QML_MATERIAL_BUILD_TYPE}
  IMPORTS
  QtQuick.Layouts
  QtQuick.Shapes
  DEPENDENCIES
  QtQuick
  OUTPUT_TARGETS
  qml_material_targets
  RESOURCE_PREFIX
  /
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)
add_library("${LIBNAME}::${LIBNAME}" ALIAS ${LIBNAME})

target_compile_features(${LIBNAME} PRIVATE cxx_std_20)
target_compile_definitions(${LIBNAME} PRIVATE QML_MATERIAL_EXPORT)
set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}_qmltyperegistrations.cpp"
  TARGET_DIRECTORY "${LIBNAME}" PROPERTIES INCLUDE_DIRECTORIES
                                           "${QML_REG_INCLUDE_DIRS}")

# QT_QMLCACHEGEN_ARGUMENTS "--only-bytecode"
set_target_properties(${LIBNAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

set(TOKEN_COMPILE_DEFINITIONS
    QM_ICON_FONT_URL="${QM_ICON_FONT_URL}"
    QM_ICON_FILL_0_FONT_URL="qrc:/Qcm/Material/${QM_ICON_FILL_0_FONT_FILE}"
    QM_ICON_FILL_1_FONT_URL="qrc:/Qcm/Material/${QM_ICON_FILL_1_FONT_FILE}"
    QM_VERSION="${QM_VERSION}")

set_source_files_properties(
  src/token/token.cpp PROPERTIES COMPILE_DEFINITIONS
                                 "${TOKEN_COMPILE_DEFINITIONS}")

set(SHADER_GLSLS "assets/shader/common.glsl" "assets/shader/sdf.glsl")
set(SHADER_SOURCES
    "assets/shader/default.vert"
    "assets/shader/round_clip.frag"
    "assets/shader/sdf_rectangle.frag"
    "assets/shader/shadow_.frag"
    "assets/shader/rectangle.vert"
    "assets/shader/rectangle.frag"
    "assets/shader/shadow.vert"
    "assets/shader/shadow.frag"
    "assets/shader/rect_glow.frag")
set(SHADER_OPT BATCHABLE OPTIMIZED)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  list(APPEND SHADER_OPT DEBUGINFO)
endif()
if(NOT WIN32)
  list(APPEND SHADER_OPT NOHLSL)
endif()

qt_add_shaders(
  ${LIBNAME}
  "shader"
  PREFIX
  "/Qcm/Material"
  GLSL
  "300es,330"
  ${SHADER_OPT}
  DEFINES
  ${SHADER_DEFINES}
  FILES
  ${SHADER_SOURCES})

foreach(file ${SHADER_SOURCES})
  set(output_file ".qsb/${file}.qsb")
  add_custom_command(
    OUTPUT ${output_file}
    DEPENDS ${SHADER_GLSLS}
    APPEND)
endforeach()

target_include_directories(
  ${LIBNAME}
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}include>"
  PRIVATE "include/${LIBNAME}" "include/${LIBNAME}/kirigami" src)

add_subdirectory(third_party)
if(QM_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick
         Qt6::QuickControls2 # QQuickAttachedPropertyPropagator
         Qt6::QuickPrivate # QQuickStateGroup
  PRIVATE material_color Qt6::DBus)

if(NOT QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config-version.cmake"
    VERSION ${QM_VERSION}
    COMPATIBILITY SameMajorVersion)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${LIBNAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config-version.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

  install(
    DIRECTORY include/${LIBNAME}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")

  install(TARGETS ${qml_material_targets} EXPORT ${LIBNAME}-targets)
  install(TARGETS ${LIBNAME} EXPORT ${LIBNAME}-targets)
  install(
    EXPORT ${LIBNAME}-targets
    FILE ${LIBNAME}-targets.cmake
    NAMESPACE ${LIBNAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

  function(qt_install_qml_module target_)
    get_target_property(QML_TARGET_PATH ${target_} QT_QML_MODULE_TARGET_PATH)
    get_target_property(QML_TYEPINFO ${target_} QT_QML_MODULE_TYPEINFO)
    get_target_property(QML_PLUGIN_TARGET ${target_}
                        QT_QML_MODULE_PLUGIN_TARGET)
    set(QML_TARGET_INSTALL_DIR "${QML_INSTALL_DIR}/${QML_TARGET_PATH}")

    install(TARGETS ${QML_PLUGIN_TARGET}
            LIBRARY DESTINATION ${QML_TARGET_INSTALL_DIR})

    install(
      DIRECTORY qml
      DESTINATION ${QML_TARGET_INSTALL_DIR}
      FILES_MATCHING
      PATTERN "*.qml"
      PATTERN "*.mjs")

    install(
      FILES "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/qmldir"
            "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/${QML_TYEPINFO}"
      DESTINATION ${QML_TARGET_INSTALL_DIR})
  endfunction()

  qt_install_qml_module(${LIBNAME})

endif()
