# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

find_package(SeleniumWebDriverATSPI)
set_package_properties(SeleniumWebDriverATSPI PROPERTIES
    DESCRIPTION "Server component for selenium tests using Linux accessibility infrastructure"
    PURPOSE "Needed for GUI tests"
    URL "https://invent.kde.org/sdk/selenium-webdriver-at-spi"
    TYPE OPTIONAL
)
if(NOT SeleniumWebDriverATSPI_FOUND AND NOT "$ENV{KDECI_BUILD}" STREQUAL "TRUE") # always run on CI
    return()
endif()

add_test(
    NAME appium-flatpak
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/flatpak.py
)
set_tests_properties(appium-flatpak PROPERTIES TIMEOUT 600)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set_tests_properties(appium-flatpak PROPERTIES
        TIMEOUT 600
        # Temporarily disabled due to test failures in test_search_install_uninstall() on arm64
        # See issue: https://invent.kde.org/plasma/discover/-/work_items/51
        DISABLED TRUE
    )
endif()
