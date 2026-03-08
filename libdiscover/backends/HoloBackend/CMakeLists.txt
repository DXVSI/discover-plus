set(holo-backend_SRCS
    HoloResource.h
    HoloResource.cpp
    HoloBackend.h
    HoloBackend.cpp
    HoloTransaction.h
    HoloTransaction.cpp
    dbushelpers.h
)

set(atomupd1_xml com.steampowered.Atomupd1.xml)
set_source_files_properties(${atomupd1_xml} PROPERTIES
    INCLUDE "dbushelpers.h"
    NA_NAMESPACE TRUE
)
qt_add_dbus_interface(holo-backend_SRCS ${atomupd1_xml} atomupd1)
qt_add_dbus_interface(holo-backend_SRCS org.freedesktop.DBus.Properties.xml dbusproperties_interface)

ecm_qt_declare_logging_category(holo-backend_SRCS
    HEADER libdiscover_holo_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_HOLO_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.holo
    DESCRIPTION "libdiscover (backend - Holo)"
    EXPORT DISCOVER
)

kcoreaddons_add_plugin(holo-backend SOURCES ${holo-backend_SRCS} INSTALL_NAMESPACE "discover")

add_library(libdiscover-backend-holo-logging-category STATIC
    ${libdiscover-backend-holo-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-holo-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-holo-logging-category
    PRIVATE
        Qt::Core
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

target_link_libraries(holo-backend
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
)
