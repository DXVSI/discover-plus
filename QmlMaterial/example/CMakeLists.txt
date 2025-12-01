qt_add_resources(
  ${LIBNAME}
  "example"
  PREFIX
  /Qcm/Material/Example
  # BASE <PATH>
  FILES
  Components.qml
  Color.qml
  Elevation.qml
  Example.qml
  Typography.qml
  Shape.qml
  About.qml)

qt_add_executable(qm_example main.cpp)
qt_add_resources(
  qm_example "example_main" PREFIX /
  # BASE <PATH>
  FILES main.qml)

set_target_properties(qm_example PROPERTIES AUTOMOC ON)
target_link_libraries(qm_example PRIVATE Qt6::Core Qt6::Quick)

if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  qt_import_qml_plugins(qm_example)
  target_link_libraries(qm_example PRIVATE qml_materialplugin)
  target_compile_definitions(qm_example PRIVATE QML_STATIC)
endif()
