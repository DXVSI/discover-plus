find_package(KF6Auth ${KF6_MIN_VERSION} CONFIG REQUIRED)

add_subdirectory(libsnapclient)

set(snap-backend_SRCS
    SnapResource.cpp
    SnapBackend.cpp
    SnapTransaction.cpp
    snapui.qrc
)
ecm_qt_declare_logging_category(snap-backend_SRCS HEADER libdiscover_snap_debug.h IDENTIFIER LIBDISCOVER_SNAP_LOG CATEGORY_NAME org.kde.plasma.libdiscover.snap DESCRIPTION "libdiscover snap backend" EXPORT DISCOVER)
kcoreaddons_add_plugin(snap-backend SOURCES ${snap-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(snap-backend
    Qt::Gui
    Qt::Core
    Qt::Concurrent
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::KIOGui
    Discover::Common
    Snapd::Core
)

install(FILES org.kde.discover.snap.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

ecm_install_icons(
    ICONS
        sc-apps-snapdiscover.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
