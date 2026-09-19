set(appstream-preview-backend_SRCS
    AppStreamPreviewBackend.cpp
    AppStreamPreviewBackendUpdater.cpp
    AppStreamPreviewResource.cpp
    resources.qrc
)

kcoreaddons_add_plugin(appstream-preview-backend SOURCES ${appstream-preview-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(appstream-preview-backend
    Qt::Core
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
    AppStreamQt
)

install(FILES org.kde.discover.appstreampreview.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
