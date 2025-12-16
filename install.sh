#!/bin/bash

set -e

echo "=== Discover Plus Installer ==="
echo ""

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ "$EUID" -eq 0 ]; then
    echo -e "${RED}Do not run this script as root. It will ask for sudo when needed.${NC}"
    exit 1
fi

if [ ! -f /etc/fedora-release ]; then
    echo -e "${RED}This script is designed for Fedora Linux.${NC}"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${YELLOW}Step 1: Removing conflicting system packages...${NC}"
sudo dnf remove -y plasma-discover plasma-discover-flatpak plasma-discover-snap plasma-discover-packagekit plasma-discover-libs 2>/dev/null || true

echo ""
echo -e "${YELLOW}Step 2: Installing build dependencies...${NC}"
sudo dnf install -y cmake extra-cmake-modules gcc-c++ kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kcrash-devel kf6-kdbusaddons-devel kf6-ki18n-devel kf6-karchive-devel kf6-kxmlgui-devel kf6-kio-devel kf6-kcmutils-devel kf6-kidletime-devel kf6-purpose-devel kf6-kiconthemes-devel kf6-kstatusnotifieritem-devel kf6-kauth-devel kf6-knotifications-devel kf6-kirigami-devel kf6-kirigami-addons-devel PackageKit-Qt6-devel appstream-qt-devel qcoro-qt6-devel qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwebview-devel flatpak-devel fwupd-devel libmarkdown-devel

echo ""
echo -e "${YELLOW}Step 3: Cleaning old build...${NC}"
rm -rf build

echo ""
echo -e "${YELLOW}Step 4: Building Discover Plus...${NC}"
mkdir build
cd build

cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_TESTING=OFF \
    -DBUILD_FlatpakBackend=ON \
    -DBUILD_PackageKitBackend=ON \
    -DBUILD_FwupdBackend=ON \
    -DBUILD_SnapBackend=ON \
    -DBUILD_AlpineApkBackend=ON \
    -DBUILD_DummyBackend=OFF \
    -DBUILD_RpmOstreeBackend=OFF \
    -DBUILD_SteamOSBackend=OFF \
    -DBUILD_WITH_QT6=ON

make -j$(nproc)

echo ""
echo -e "${YELLOW}Step 5: Installing Discover Plus...${NC}"
sudo make install

echo ""
echo -e "${YELLOW}Step 6: Configuring offline updates...${NC}"
sudo mkdir -p /etc/xdg
echo '[Software]
UseOfflineUpdates=true' | sudo tee /etc/xdg/discoverrc > /dev/null

echo ""
echo -e "${GREEN}=== Installation complete! ===${NC}"
echo ""
echo "Run: plasma-discover"
echo ""
