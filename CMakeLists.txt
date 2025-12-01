cmake_minimum_required(VERSION 3.16)

project(discover)
set(PROJECT_VERSION "6.5.80")

set(QT_MIN_VERSION "6.9.0")
set(KF6_MIN_VERSION "6.18.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.85")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH}  "${CMAKE_SOURCE_DIR}/cmake")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMAddTests)
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMFindQmlModule)
include(ECMDeprecationSettings)
include(ECMSourceVersionControl)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Widgets Test Network Concurrent DBus Quick QuickControls2)
find_package(Qt6 ${QT_MIN_VERSION} CONFIG OPTIONAL_COMPONENTS WebView)

# Add QmlMaterial library for Material Design 3 (using local copy)
# Temporarily remove strict QString checks for QmlMaterial compatibility
set(_saved_definitions ${CMAKE_CXX_FLAGS})
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)
add_subdirectory(QmlMaterial EXCLUDE_FROM_ALL)
add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)

find_package(PkgConfig REQUIRED)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED Kirigami CoreAddons Config Crash DBusAddons I18n Archive XmlGui KIO KCMUtils IdleTime Notifications Purpose Auth StatusNotifierItem IconThemes)
find_package(KF6 ${KF6_MIN_VERSION} OPTIONAL_COMPONENTS UserFeedback)

find_package(packagekitqt6 1.0.1 CONFIG)
find_package(AppStreamQt 1.0.4 CONFIG REQUIRED)
find_package(KF6Attica ${KF6_MIN_VERSION} CONFIG)
find_package(KF6NewStuff ${KF6_MIN_VERSION} CONFIG)

find_package(KF6KirigamiAddons 1.10.0 REQUIRED)
set_package_properties(KF6KirigamiAddons PROPERTIES
    TYPE REQUIRED
    PURPOSE "Provides additional visual components"
)

pkg_check_modules(Flatpak IMPORTED_TARGET flatpak>=0.11.8)
pkg_check_modules(Fwupd IMPORTED_TARGET fwupd>=1.9.4)
pkg_check_modules(Markdown IMPORTED_TARGET libmarkdown)
if(Markdown_FOUND AND Markdown_VERSION VERSION_GREATER_EQUAL 3)
    add_definitions(-DMARKDOWN3)
endif()
pkg_check_modules(Ostree IMPORTED_TARGET ostree-1)
pkg_check_modules(RpmOstree IMPORTED_TARGET rpm-ostree-1)

find_package(QCoro6 REQUIRED COMPONENTS Core DBus Network)

qcoro_enable_coroutines()

ecm_find_qmlmodule(org.kde.kitemmodels 1.0 REQUIRED)

list(APPEND CMAKE_AUTOMOC_MACRO_NAMES "DISCOVER_BACKEND_PLUGIN")

ecm_set_disabled_deprecation_versions(QT 6.8.1
    KF 6.9.0
)

add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS -DQT_NO_URL_CAST_FROM_STRING)

# When building from git use a random iid to prevent loading incompatible plugins
# from the distro or older git builds.
set(DISCOVER_PLUGIN_IID ${PROJECT_VERSION})
set(DISCOVER_NOTIFIER_IID ${PROJECT_VERSION})
if(ECM_SOURCE_UNDER_VERSION_CONTROL)
    string(RANDOM LENGTH 16 DISCOVER_PLUGIN_IID)
    string(RANDOM LENGTH 16 DISCOVER_NOTIFIER_IID)
endif()
string(PREPEND DISCOVER_PLUGIN_IID "org.kde.discover.")
string(APPEND DISCOVER_PLUGIN_IID ".AbstractResourcesBackendFactory")

string(PREPEND DISCOVER_NOTIFIER_IID "org.kde.discover.")
string(APPEND DISCOVER_NOTIFIER_IID ".BackendNotifierModule")

configure_file(DiscoverConfig.h.in DiscoverConfig.h)
configure_file(DiscoverVersion.h.in DiscoverVersion.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(BUILD_TESTING AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(appiumtests)
endif()
add_subdirectory(libdiscover)
add_subdirectory(discover)
add_subdirectory(exporter)

option(WITH_KCM "Build and install the updates KCM" ON)
if(WITH_KCM)
    add_subdirectory(kcm)
endif()

option(WITH_NOTIFIER "Build and install the notifier plasmoid" ON)
if(WITH_NOTIFIER)
    add_subdirectory(notifier)
endif()

set_package_properties(KF6Attica PROPERTIES
    DESCRIPTION "KDE Framework that implements the Open Collaboration Services API"
    PURPOSE "Required to build the KNewStuff3 backend"
    TYPE OPTIONAL)
set_package_properties(KF6Kirigami2 PROPERTIES
    DESCRIPTION "KDE's lightweight user interface framework for mobile and convergent applications"
    URL "https://techbase.kde.org/Kirigami"
    PURPOSE "Required by discover qml components"
    TYPE RUNTIME)
set_package_properties(KF6NewStuff PROPERTIES
    DESCRIPTION "Qt library that allows to interact with KNewStuff implementations"
    PURPOSE "Required to build the KNS backend"
    TYPE OPTIONAL)
set_package_properties(KF6UserFeedback PROPERTIES
    DESCRIPTION "Framework for collecting feedback from application users via telemetry and targeted surveys"
    URL "https://invent.kde.org/frameworks/kuserfeedback"
    PURPOSE "Required for submission of telemetry data"
    TYPE OPTIONAL)
set_package_properties(packagekitqt6 PROPERTIES
    DESCRIPTION "Library that exposes PackageKit resources"
    URL "https://www.freedesktop.org/software/PackageKit/"
    PURPOSE "Required to build the PackageKit backend"
    TYPE OPTIONAL)
set_package_properties(AppStreamQt PROPERTIES
    DESCRIPTION "Library that lists Appstream resources"
    URL "https://www.freedesktop.org"
    PURPOSE "Required to build the PackageKit, Flatpak and Snap backends"
    TYPE OPTIONAL)
add_feature_info(Flatpak Flatpak_FOUND
                "Library that exposes flatpak repositories. Required to build the Flatpak backend"
)
add_feature_info(Fwupd Fwupd_FOUND "Exposes fwupd")
add_feature_info(Ostree Ostree_FOUND
                "Library to manage ostree repository. Required to build the rpm-ostree backend"
)
add_feature_info(RpmOstree RpmOstree_FOUND
                "rpm-ostree binary to manage the system. Required to build the rpm-ostree backend"
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

ecm_qt_install_logging_categories(
    EXPORT DISCOVER
    FILE discover.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

ki18n_install(po)
