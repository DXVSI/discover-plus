ecm_qt_declare_logging_category(libdiscover-backend-flatpak-logging-category_SRCS
    HEADER libdiscover_backend_flatpak_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_FLATPAK_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.flatpak
    DESCRIPTION "libdiscover (backend - Flatpak)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-flatpak-logging-category STATIC
    ${libdiscover-backend-flatpak-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-flatpak-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-flatpak-logging-category
    PRIVATE
        Qt::Core
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(flatpak-backend_SRCS
    FlatpakResource.cpp
    FlatpakBackend.cpp
    FlatpakFetchDataJob.cpp
    FlatpakSourcesBackend.cpp
    FlatpakJobTransaction.cpp
    FlatpakTransactionThread.cpp
    FlatpakRefreshAppstreamMetadataJob.cpp
    FlatpakPermission.cpp
    resources.qrc
)

kcoreaddons_add_plugin(flatpak-backend SOURCES ${flatpak-backend_SRCS} INSTALL_NAMESPACE "discover")
kde_target_enable_exceptions(flatpak-backend PRIVATE)
target_link_libraries(flatpak-backend
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Concurrent
        KF6::CoreAddons
        KF6::ConfigCore
        KF6::KIOGui
        KF6::KIOWidgets
        Discover::Common
        AppStreamQt
        PkgConfig::Flatpak
        QCoro::Core
        libdiscover-backend-flatpak-logging-category
)

if (NOT Flatpak_VERSION VERSION_LESS 1.1.2)
    target_compile_definitions(flatpak-backend PRIVATE -DFLATPAK_VERBOSE_PROGRESS -DFLATPAK_LIST_UNUSED_REFS)
endif()

install(FILES flatpak-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

kcoreaddons_add_plugin(FlatpakNotifier SOURCES FlatpakNotifier.cpp INSTALL_NAMESPACE "discover-notifier")
target_link_libraries(FlatpakNotifier
    PRIVATE
        Discover::Notifiers
        Qt::Concurrent
        PkgConfig::Flatpak
        libdiscover-backend-flatpak-logging-category

)
set_target_properties(FlatpakNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

if (Flatpak_VERSION VERSION_LESS 1.10.2)
    target_compile_definitions(flatpak-backend PRIVATE -DFLATPAK_EXTERNC_REQUIRED)
    target_compile_definitions(FlatpakNotifier PRIVATE -DFLATPAK_EXTERNC_REQUIRED)
endif()

install(FILES org.kde.discover.flatpak.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

ecm_install_icons(
    ICONS
        sc-apps-flatpak-discover.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
