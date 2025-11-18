add_definitions(-DTRANSLATION_DOMAIN=\"libdiscover\")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-paths.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h)

add_subdirectory(backends)
add_subdirectory(notifiers)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if (TARGET AppStreamQt)
    add_library(DiscoverAppStreamIntegration STATIC
        appstream/AppStreamIntegration.cpp
    )
    target_link_libraries(DiscoverAppStreamIntegration KF6::CoreAddons KF6::ConfigCore AppStreamQt)
    set_property(TARGET DiscoverAppStreamIntegration PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

set(discovercommon_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
    Category/CategoriesReader.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/ReviewsModel.cpp
    Transaction/AddonList.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp
    Transaction/TransactionModel.cpp
    UpdateModel/UpdateItem.cpp
    UpdateModel/UpdateModel.cpp
    resources/DiscoverAction.cpp
    resources/ResourcesModel.cpp
    resources/ResourcesProxyModel.cpp
    resources/PackageState.cpp
    resources/ResourcesUpdatesModel.cpp
    resources/StandardBackendUpdater.cpp
    resources/SourcesModel.cpp
    resources/AbstractResourcesBackend.cpp
    resources/AbstractResource.cpp
    resources/AbstractBackendUpdater.cpp
    resources/AbstractSourcesBackend.cpp
    resources/StoredResultsStream.cpp
    DiscoverBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
    CachedNetworkAccessManager.cpp
    LazyIconResolver.cpp
    SearchHistory.cpp
    SearchHistory.h

    utils.h
    utilscoro.cpp
    utilscoro.h
)

ecm_qt_declare_logging_category(discovercommon_SRCS
    HEADER libdiscover_debug.h
    IDENTIFIER LIBDISCOVER_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover
    DESCRIPTION "libdiscover"
    EXPORT DISCOVER
)

add_library(DiscoverCommon ${discovercommon_SRCS})
if(TARGET AppStreamQt)
    target_sources(DiscoverCommon PRIVATE
        appstream/OdrsReviewsBackend.cpp
        appstream/OdrsReviewsJob.cpp
        appstream/AppStreamConcurrentPool.cpp
        appstream/AppStreamIntegration.cpp
        appstream/AppStreamUtils.cpp
    )
    target_link_libraries(DiscoverCommon PRIVATE
        Qt::Concurrent
        KF6::IconThemes
        AppStreamQt
    )
    kde_source_files_enable_exceptions(appstream/AppStreamUtils.cpp)
endif()

target_link_libraries(DiscoverCommon
PUBLIC
    Qt::Core
    Qt::Qml
    Qt::Gui
    KF6::I18n
    QCoro::Core
PRIVATE
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::KIOCore
)
add_library(Discover::Common ALIAS DiscoverCommon)

generate_export_header(DiscoverCommon)

target_include_directories(DiscoverCommon PRIVATE ${PHONON_INCLUDES} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS DiscoverCommon DESTINATION ${KDE_INSTALL_LIBDIR}/plasma-discover)
install(FILES resources/discoverabstractnotifier.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
