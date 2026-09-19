# KDE Discover version this fork is based on. It must stay equal to
# PROJECT_VERSION in CMakeLists.txt (packaging/scripts/check-release-contract.sh
# enforces that) because the plugin interface id and "--version" derive from it.
%global kde_base_version 6.8.80

# libDiscoverCommon.so and libDiscoverNotifiers.so are private libraries and
# the Qt plugins are not linkable at all. They have the same names as the ones
# in Fedora's plasma-discover-libs, so they must not show up as capabilities
# that could satisfy a dependency of the Fedora packages.
%global __provides_exclude_from ^(%{_libdir}/plasma-discover|%{_libdir}/qt6/plugins)/.*\\.so$
%global __requires_exclude ^libDiscover(Common|Notifiers)\\.so.*$

Name:           discover-plus
Version:        1.0.0
# A packaging-only rebuild of the same VERSION raises this number together
# with a new %%changelog entry; the scripts and workflows read it from here.
Release:        1%{?dist}
Summary:        Fedora-focused fork of the KDE Discover software center

# The list of Fedora's plasma-discover plus two identifiers that Fedora misses:
# LGPL-3.0-or-later (discover/PowerManagementInterface.*) and LGPL-2.1-or-later
# (notifier/org.freedesktop.login1.Manager.xml) are compiled into the binaries.
License:        BSD-3-Clause AND CC0-1.0 AND GPL-2.0-only AND GPL-2.0-or-later AND GPL-3.0-only AND LGPL-2.0-or-later AND LGPL-2.1-only AND LGPL-2.1-or-later AND LGPL-3.0-only AND LGPL-3.0-or-later AND (GPL-2.0-only OR GPL-3.0-only) AND (LGPL-2.1-only OR LGPL-3.0-only)
URL:            https://github.com/DXVSI/discover-plus
Source0:        %{url}/releases/download/plus-v%{version}/%{name}-%{version}.tar.xz

# Packages are built and tested only on the architecture covered by the CI.
ExclusiveArch:  x86_64

BuildRequires:  extra-cmake-modules
BuildRequires:  kf6-rpm-macros
BuildRequires:  gettext
BuildRequires:  desktop-file-utils
BuildRequires:  appstream

BuildRequires:  cmake(KF6Archive)
BuildRequires:  cmake(KF6Attica)
BuildRequires:  cmake(KF6Config)
BuildRequires:  cmake(KF6CoreAddons)
BuildRequires:  cmake(KF6Crash)
BuildRequires:  cmake(KF6DBusAddons)
BuildRequires:  cmake(KF6I18n)
BuildRequires:  cmake(KF6IconThemes)
BuildRequires:  cmake(KF6IdleTime)
BuildRequires:  cmake(KF6ItemModels)
BuildRequires:  cmake(KF6KCMUtils)
BuildRequires:  cmake(KF6KIO)
BuildRequires:  cmake(KF6Kirigami)
BuildRequires:  cmake(KF6KirigamiAddons) >= 1.10.0
BuildRequires:  cmake(KF6NewStuff)
BuildRequires:  cmake(KF6Notifications)
BuildRequires:  cmake(KF6Purpose)
BuildRequires:  cmake(KF6StatusNotifierItem)
BuildRequires:  cmake(KF6UserFeedback)
BuildRequires:  cmake(KF6XmlGui)
BuildRequires:  cmake(QCoro6)

BuildRequires:  appstream-qt-devel >= 1.0.4
BuildRequires:  flatpak-devel >= 0.11.8
BuildRequires:  pkgconfig(fwupd) >= 2.1.1
BuildRequires:  pkgconfig(libmarkdown)
BuildRequires:  pkgconfig(packagekitqt6)

BuildRequires:  pkgconfig(Qt6Concurrent)
BuildRequires:  pkgconfig(Qt6DBus)
BuildRequires:  pkgconfig(Qt6Network)
BuildRequires:  pkgconfig(Qt6Qml)
BuildRequires:  pkgconfig(Qt6Quick)
BuildRequires:  pkgconfig(Qt6QuickControls2)
# Qt6 Test is a REQUIRED component in CMakeLists.txt even though %%cmake_kf6
# builds with BUILD_TESTING=FALSE.
BuildRequires:  pkgconfig(Qt6Test)
BuildRequires:  pkgconfig(Qt6WebView)
BuildRequires:  pkgconfig(Qt6Widgets)

# Discover Plus installs the same paths under /usr, the same private libraries
# (libDiscoverCommon.so, libDiscoverNotifiers.so) and plugins with an
# incompatible interface id, so it cannot coexist with any Fedora Discover
# subpackage. The Obsoletes let a plain "dnf install" of the package file
# replace the stock packages in one transaction. They are unversioned on
# purpose: the fork has its own version line that is never comparable with
# Fedora's EVR, and a versioned Obsoletes would stop working as soon as Fedora
# ships a newer Discover. There is deliberately no "Provides: plasma-discover".
Obsoletes:      plasma-discover
Obsoletes:      plasma-discover-libs
Obsoletes:      plasma-discover-flatpak
Obsoletes:      plasma-discover-kns
Obsoletes:      plasma-discover-notifier
Obsoletes:      plasma-discover-offline-updates
Obsoletes:      plasma-discover-packagekit
Obsoletes:      plasma-discover-rpm-ostree
Obsoletes:      plasma-discover-snap
Conflicts:      plasma-discover
Conflicts:      plasma-discover-libs
Conflicts:      plasma-discover-flatpak
Conflicts:      plasma-discover-kns
Conflicts:      plasma-discover-notifier
Conflicts:      plasma-discover-offline-updates
Conflicts:      plasma-discover-packagekit
Conflicts:      plasma-discover-rpm-ostree
Conflicts:      plasma-discover-snap

# Informational marker of the KDE Discover base; the clean-install CI job reads
# the expected "--version" output from it.
Provides:       bundled(plasma-discover) = %{kde_base_version}

# Shared-library dependencies are generated automatically. The explicit ones
# below cover programs the fork executes, QML imports and the build-time
# KF6/Qt minimums.
# pkexec runs "dnf", "dnf copr", "flatpak" and a bash setup script.
Requires:       polkit
Requires:       dnf5
Requires:       dnf5-plugins
Requires:       rpm
Requires:       flatpak
Requires:       bash
Requires:       coreutils
Requires:       grep
Requires:       sed
Requires:       PackageKit
Requires:       hicolor-icon-theme
# Owner of config.kcfg, qlogging-categories6 and the other shared KDE
# directories the package installs into.
Requires:       kf6-filesystem

# The KF6 and Qt versions of the build become runtime minimums. KF6 libraries
# do not version their symbols, so the generated libKF6*.so.6 dependencies
# would also be satisfied by the older KF6 of the Fedora installation media.
# The macros are undefined while "dnf builddep" parses this spec on a system
# without kf6-rpm-macros; %%prep refuses to build in that state.
%if "%{?_kf6_version}" != ""
Requires:       kf6-kirigami%{?_isa} >= %{_kf6_version}
Requires:       kf6-kitemmodels%{?_isa} >= %{_kf6_version}
Requires:       kf6-purpose%{?_isa} >= %{_kf6_version}
Requires:       kf6-qqc2-desktop-style%{?_isa} >= %{_kf6_version}
Requires:       kf6-kcoreaddons%{?_isa} >= %{_kf6_version}
Requires:       kf6-kconfig%{?_isa} >= %{_kf6_version}
Requires:       kf6-kio-core%{?_isa} >= %{_kf6_version}
Requires:       kf6-kcmutils%{?_isa} >= %{_kf6_version}
Requires:       kf6-knewstuff%{?_isa} >= %{_kf6_version}
# QML import of Feedback.qml. The build enables WITH_FEEDBACK, but nothing
# links the library, so no dependency is generated for it.
Requires:       kf6-kuserfeedback%{?_isa} >= %{_kf6_version}
%endif
Requires:       kf6-kirigami-addons%{?_isa} >= 1.10.0
%if "%{?_qt6_version}" != ""
Requires:       qt6-qtbase%{?_isa} >= %{_qt6_version}
Requires:       qt6-qtdeclarative%{?_isa} >= %{_qt6_version}
Requires:       qt6-qtwebview%{?_isa} >= %{_qt6_version}
Requires:       qt6-qt5compat%{?_isa} >= %{_qt6_version}
%endif

Recommends:     fedora-appstream-metadata
# Exec= of kcm_updates.desktop
Recommends:     plasma-systemsettings

%description
Discover Plus is a fork of KDE Discover for Fedora KDE. It keeps the regular
Discover user interface and adds Fedora package workflows: first-run
repository setup, RPM Fusion, Flatpak sources and Fedora COPR.

This package bundles the PackageKit, Flatpak, KNewStuff, fwupd and AppStream
preview backends, the update notifier and the Updates settings module, and it
replaces the plasma-discover packages of Fedora.


%prep
%autosetup
test ! -e .git
test "$(tr -d '\r\n' < VERSION)" = "%{version}"
%if "%{?_kf6_version}" == "" || "%{?_qt6_version}" == ""
echo "KF6 or Qt build version is unknown; versioned runtime requirements cannot be generated" >&2
exit 1
%endif


%build
%cmake_kf6 \
  -DPACKAGEKIT_AUTOREMOVE:BOOL=ON \
  -DBUILD_PackageKitBackend:BOOL=ON \
  -DBUILD_FlatpakBackend:BOOL=ON \
  -DBUILD_FwupdBackend:BOOL=ON \
  -DBUILD_AppStreamPreviewBackend:BOOL=ON \
  -DBUILD_SnapBackend:BOOL=OFF \
  -DBUILD_RpmOstreeBackend:BOOL=OFF \
  -DBUILD_AlpineApkBackend:BOOL=OFF \
  -DBUILD_DummyBackend:BOOL=OFF \
  -DBUILD_HoloBackend:BOOL=OFF \
  -DBUILD_SystemdSysupdateBackend:BOOL=OFF
%cmake_build


%install
%cmake_install

# Enable offline updates by default, like plasma-discover-offline-updates.
install -d -m 0755 %{buildroot}%{_kf6_sysconfdir}/xdg
printf '[Software]\nUseOfflineUpdates=true\n' > %{buildroot}%{_kf6_sysconfdir}/xdg/discoverrc
chmod 0644 %{buildroot}%{_kf6_sysconfdir}/xdg/discoverrc

# Installed unconditionally by upstream although the Snap backend is disabled.
rm -fv %{buildroot}%{_kf6_datadir}/applications/org.kde.discover.snap.desktop

%find_lang libdiscover
%find_lang kcm_updates
%find_lang plasma-discover --with-html
%find_lang plasma-discover-notifier
cat libdiscover.lang kcm_updates.lang plasma-discover.lang plasma-discover-notifier.lang | sort -u > %{name}.lang


%check
packaging/scripts/verify-package-contents.sh %{buildroot} %{_libdir} %{kde_base_version}
for desktop_file in \
    org.kde.discover.desktop \
    org.kde.discover.urlhandler.desktop \
    org.kde.discover.flatpak.desktop \
    org.kde.discover.appstreampreview.desktop \
    org.kde.discover.notifier.desktop \
    kcm_updates.desktop
do
    desktop-file-validate %{buildroot}%{_kf6_datadir}/applications/$desktop_file
done
for metainfo_file in \
    org.kde.discover.appdata.xml \
    org.kde.discover.flatpak.appdata.xml \
    org.kde.discover.packagekit.appdata.xml \
    org.kde.discover.appstreampreview.metainfo.xml
do
    appstreamcli validate --no-net %{buildroot}%{_kf6_metainfodir}/$metainfo_file
done


%files -f %{name}.lang
%license LICENSES/*.txt
%doc README.md
%{_bindir}/plasma-discover
%{_libexecdir}/DiscoverNotifier
%dir %{_libdir}/plasma-discover
%{_libdir}/plasma-discover/libDiscoverCommon.so
%{_libdir}/plasma-discover/libDiscoverNotifiers.so
%dir %{_kf6_qtplugindir}/discover
%{_kf6_qtplugindir}/discover/appstream-preview-backend.so
%{_kf6_qtplugindir}/discover/flatpak-backend.so
%{_kf6_qtplugindir}/discover/fwupd-backend.so
%{_kf6_qtplugindir}/discover/kns-backend.so
%{_kf6_qtplugindir}/discover/packagekit-backend.so
%dir %{_kf6_qtplugindir}/discover-notifier
%{_kf6_qtplugindir}/discover-notifier/DiscoverPackageKitNotifier.so
%{_kf6_qtplugindir}/discover-notifier/FlatpakNotifier.so
# No Fedora package owns the System Settings module directories.
%dir %{_kf6_qtplugindir}/plasma
%dir %{_kf6_qtplugindir}/plasma/kcms
%dir %{_kf6_qtplugindir}/plasma/kcms/systemsettings
%{_kf6_qtplugindir}/plasma/kcms/systemsettings/kcm_updates.so
%{_kf6_datadir}/applications/org.kde.discover.desktop
%{_kf6_datadir}/applications/org.kde.discover.urlhandler.desktop
%{_kf6_datadir}/applications/org.kde.discover.flatpak.desktop
%{_kf6_datadir}/applications/org.kde.discover.appstreampreview.desktop
%{_kf6_datadir}/applications/org.kde.discover.notifier.desktop
%{_kf6_datadir}/applications/kcm_updates.desktop
%{_kf6_sysconfdir}/xdg/autostart/org.kde.discover.notifier.desktop
%{_kf6_metainfodir}/org.kde.discover.appdata.xml
%{_kf6_metainfodir}/org.kde.discover.flatpak.appdata.xml
%{_kf6_metainfodir}/org.kde.discover.packagekit.appdata.xml
%{_kf6_metainfodir}/org.kde.discover.appstreampreview.metainfo.xml
%{_kf6_datadir}/config.kcfg/discover*.kcfg
%{_kf6_datadir}/icons/hicolor/*/apps/plasmadiscover.*
%{_kf6_datadir}/icons/hicolor/*/apps/flatpak-discover.*
# Owned by kf6-kxmlgui, which the package does not link against.
%dir %{_kf6_datadir}/kxmlgui5
%{_kf6_datadir}/kxmlgui5/plasmadiscover/
%{_kf6_datadir}/knotifications6/discoverabstractnotifier.notifyrc
%{_kf6_datadir}/qlogging-categories6/discover.categories
%dir %{_kf6_datadir}/libdiscover
%dir %{_kf6_datadir}/libdiscover/categories
%{_kf6_datadir}/libdiscover/categories/flatpak-backend-categories.xml
%{_kf6_datadir}/libdiscover/categories/packagekit-backend-categories.xml
# Unlike Fedora this is a config file: a package update must not undo a local
# decision to turn offline updates off.
%config(noreplace) %{_kf6_sysconfdir}/xdg/discoverrc


%changelog
* Sat Sep 19 2026 DXVSI <DXVSI@users.noreply.github.com> - 1.0.0-1
- First RPM release of Discover Plus, based on KDE Discover 6.8.80
- Removing an application now also removes its unused dependencies
  (PACKAGEKIT_AUTOREMOVE), like the stock Fedora Discover
