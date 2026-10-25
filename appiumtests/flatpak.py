#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

import subprocess
import unittest
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
from appium.options.common.app_option import AppOption
from selenium.webdriver.common.options import ArgOptions
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support import expected_conditions as EC


class ATSPIOptions(AppiumOptions, AppOption):
    pass


class FlatpakTest(unittest.TestCase):

    def setUp(self):
        subprocess.run(['flatpak', 'remote-add', '--user', '--if-not-exists', 'flathub', 'https://flathub.org/repo/flathub.flatpakrepo'])
        subprocess.run(['flatpak', 'uninstall', '--noninteractive', 'org.kde.kalzium'])

        options = ATSPIOptions()
        options.app = "plasma-discover --backends flatpak-backend"
        self.driver = webdriver.Remote(
            command_executor='http://127.0.0.1:4723',
            options=options)


    def tearDown(self):
        self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))
        self.driver.quit()


    def test_search_install_uninstall(self):
        WebDriverWait(self.driver, 30).until(
            EC.invisibility_of_element_located((AppiumBy.CLASS_NAME, "[label | Loading…]"))
        )

        searchElement = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="searchField")
        searchFocused = searchElement.get_attribute('focused')
        self.assertTrue(searchFocused)

        searchElement.send_keys("Kalzium")
        searchElement.send_keys(Keys.ENTER)

        listItem = WebDriverWait(self.driver, 123).until(
            EC.element_to_be_clickable((AppiumBy.CLASS_NAME, "[list item | Kalzium]"))
        )
        listItem.click()

        description = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="applicationDescription").text
        self.assertTrue(len(description) > 64) # arbitrary large number

        installButton = self.driver.find_element(by=AppiumBy.XPATH, value="//*[@name='Install from Flathub (user)' and contains(@accessibility-id, 'Button')]")
        installButton.click()

        removeButton = WebDriverWait(self.driver, 120).until(
            EC.element_to_be_clickable((AppiumBy.XPATH, "//*[@name='Remove' and contains(@accessibility-id, 'Button')]"))
        )
        removeButton.click()

        # should find install button again after removal
        WebDriverWait(self.driver, 30).until(
            EC.element_to_be_clickable((AppiumBy.XPATH, "//*[@name='Install from Flathub (user)' and contains(@accessibility-id, 'Button')]"))
        )


if __name__ == '__main__':
    unittest.main()
