function(add_unit_test name)
    add_executable(${name} ${ARGN})
    add_test(${name} dbus-run-session ${CMAKE_BINARY_DIR}/bin/${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        Discover::Common
        Qt::Test
        Qt::Core
        ${EXTRA_LIBS}
    )
endfunction()

# Shared logging category for backends which don't declare their own ones
ecm_qt_declare_logging_category(libdiscover-backend-logging-category_SRCS
    HEADER libdiscover_backend_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend
    DESCRIPTION "libdiscover (backend)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-logging-category STATIC
    ${libdiscover-backend-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-logging-category
    PRIVATE
        Qt::Core
)

if (KF6Attica_FOUND AND KF6NewStuff_FOUND AND AppStreamQt_FOUND)
   add_subdirectory(KNSBackend)
endif()


option(BUILD_PackageKitBackend "Build PackageKit support." "${packagekitqt6_FOUND}")
if (BUILD_PackageKitBackend AND AppStreamQt_FOUND)
    add_subdirectory(PackageKitBackend)
endif()

option(BUILD_DummyBackend "Build the DummyBackend" "OFF")
if (BUILD_DummyBackend)
    add_subdirectory(DummyBackend)
endif()

option(BUILD_FlatpakBackend "Build Flatpak support" "ON")
if (Flatpak_FOUND AND AppStreamQt_FOUND AND BUILD_FlatpakBackend)
    add_subdirectory(FlatpakBackend)
elseif (BUILD_FlatpakBackend)
    message(WARNING "BUILD_FlatpakBackend enabled but Flatpak=${Flatpak_FOUND} or AppStreamQt=${AppStreamQt_FOUND} not found")
endif()

find_package(Snapd 1.65)
set_package_properties(Snapd PROPERTIES
    DESCRIPTION "Library that exposes Snapd"
    URL "https://www.snapcraft.io"
    PURPOSE "Required to build the Snap backend"
    TYPE OPTIONAL
)

option(BUILD_HoloBackend "Build Holo support." "OFF")
if (BUILD_HoloBackend)
    add_subdirectory(HoloBackend)
endif()

option(BUILD_SystemdSysupdateBackend "Build support for Systemd's SysUpdate." "OFF")
if (BUILD_SystemdSysupdateBackend)
    add_subdirectory(SystemdSysupdateBackend)
endif()

option(BUILD_SnapBackend "Build Snap support." "ON")
if (BUILD_SnapBackend AND AppStreamQt_FOUND AND Snapd_FOUND)
    add_subdirectory(SnapBackend)
endif()

option(BUILD_FwupdBackend "Build Fwupd support." "ON")
if (BUILD_FwupdBackend AND TARGET PkgConfig::Fwupd)
    add_subdirectory(FwupdBackend)
endif()

option(BUILD_RpmOstreeBackend "Build rpm-ostree support." "${RpmOstree_FOUND}")
if (BUILD_RpmOstreeBackend AND Ostree_FOUND)
    add_subdirectory(RpmOstreeBackend)
endif()

# Optional library
find_package(ApkQt CONFIG)
set_package_properties(ApkQt PROPERTIES
    DESCRIPTION "C++/Qt interface library for Alpine package keeper"
    URL "https://gitlab.com/postmarketOS/libapk-qt"
    PURPOSE "Required to build the Alpine APK backend"
    TYPE OPTIONAL)

option(BUILD_AlpineApkBackend "Build Alpine APK support." "ON")
if(BUILD_AlpineApkBackend AND ApkQt_FOUND)
    add_subdirectory(AlpineApkBackend)
endif()
