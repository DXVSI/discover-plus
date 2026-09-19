#!/bin/sh
set -eu

usage() {
    echo "usage: $0 [--installed] [--load-plugins] <root> <libdir> <kde-base-version>" >&2
    echo "  <root>         staged package root, or / together with --installed" >&2
    echo "  <libdir>       library directory inside the root, for example /usr/lib64" >&2
    echo "  --installed    check the installed package without any library path override" >&2
    echo "  --load-plugins also start the programs headless and watch Qt load every plugin" >&2
    exit 2
}

installed=0
load_plugins=0
while [ "$#" -gt 0 ]; do
    case $1 in
        --installed) installed=1 ;;
        --load-plugins) load_plugins=1 ;;
        --*) usage ;;
        *) break ;;
    esac
    shift
done
if [ "$#" -ne 3 ]; then
    usage
fi

root=${1%/}
libdir=$2
kde_base_version=$3

if [ "$installed" -eq 1 ]; then
    if [ -n "$root" ]; then
        echo "--installed checks the running system, the root must be /" >&2
        exit 2
    fi
elif [ -z "$root" ] || [ ! -d "$root" ]; then
    echo "staged root is not a directory: $1" >&2
    exit 2
fi
case $libdir in
    /*) ;;
    *) echo "libdir must be an absolute path: $libdir" >&2; exit 2 ;;
esac
if ! printf '%s\n' "$kde_base_version" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "KDE base version must look like X.Y.Z: $kde_base_version" >&2
    exit 2
fi

private_libdir=$libdir/plasma-discover
qt_plugindir=$libdir/qt6/plugins
discover_bin=/usr/bin/plasma-discover
notifier_bin=/usr/libexec/DiscoverNotifier

backend_plugins="
appstream-preview-backend
flatpak-backend
fwupd-backend
kns-backend
packagekit-backend
"
notifier_plugins="
DiscoverPackageKitNotifier
FlatpakNotifier
"

# The binaries carry the absolute RUNPATH of the installed location. A staged
# root therefore needs explicit search paths, otherwise the checks would either
# fail or silently use a Discover that happens to be installed on the host.
run_in_root() {
    if [ "$installed" -eq 1 ]; then
        "$@"
    else
        LD_LIBRARY_PATH=$root$private_libdir QT_PLUGIN_PATH=$root$qt_plugindir "$@"
    fi
}

require_file() {
    if [ ! -f "$root$1" ]; then
        echo "missing package file: $1" >&2
        exit 1
    fi
}

forbid_path() {
    if [ -e "$root$1" ] || [ -L "$root$1" ]; then
        echo "package must not contain: $1" >&2
        exit 1
    fi
}

require_line() {
    if ! grep -Fxq -- "$2" "$root$1"; then
        echo "$1 must contain the line: $2" >&2
        exit 1
    fi
}

require_file "$discover_bin"
require_file "$notifier_bin"
require_file "$private_libdir/libDiscoverCommon.so"
require_file "$private_libdir/libDiscoverNotifiers.so"
for plugin in $backend_plugins; do
    require_file "$qt_plugindir/discover/$plugin.so"
done
for plugin in $notifier_plugins; do
    require_file "$qt_plugindir/discover-notifier/$plugin.so"
done
require_file "$qt_plugindir/plasma/kcms/systemsettings/kcm_updates.so"
require_file /usr/share/applications/org.kde.discover.desktop
require_file /usr/share/applications/org.kde.discover.urlhandler.desktop
require_file /usr/share/applications/org.kde.discover.flatpak.desktop
require_file /usr/share/applications/org.kde.discover.appstreampreview.desktop
require_file /usr/share/applications/org.kde.discover.notifier.desktop
require_file /usr/share/applications/kcm_updates.desktop
require_file /etc/xdg/autostart/org.kde.discover.notifier.desktop
require_file /etc/xdg/discoverrc
require_file /usr/share/metainfo/org.kde.discover.appdata.xml
require_file /usr/share/metainfo/org.kde.discover.flatpak.appdata.xml
require_file /usr/share/metainfo/org.kde.discover.packagekit.appdata.xml
require_file /usr/share/metainfo/org.kde.discover.appstreampreview.metainfo.xml
require_file /usr/share/config.kcfg/discoversettings.kcfg
require_file /usr/share/icons/hicolor/scalable/apps/plasmadiscover.svg
require_file /usr/share/kxmlgui5/plasmadiscover/plasmadiscoverui.rc
require_file /usr/share/knotifications6/discoverabstractnotifier.notifyrc
require_file /usr/share/qlogging-categories6/discover.categories
require_file /usr/share/libdiscover/categories/packagekit-backend-categories.xml
require_file /usr/share/libdiscover/categories/flatpak-backend-categories.xml
require_file /usr/share/locale/ru/LC_MESSAGES/plasma-discover.mo
require_file /usr/share/locale/ru/LC_MESSAGES/libdiscover.mo

# Backends that are switched off in the spec, and the libexec location used by
# a plain "cmake --install" (install.sh) instead of the Fedora one.
forbid_path /usr/share/applications/org.kde.discover.snap.desktop
forbid_path "$qt_plugindir/discover/snap-backend.so"
forbid_path "$qt_plugindir/discover/rpm-ostree-backend.so"
forbid_path "$qt_plugindir/discover/alpineapk-backend.so"
forbid_path "$qt_plugindir/discover/dummy-backend.so"
forbid_path "$qt_plugindir/discover-notifier/rpm-ostree-notifier.so"
forbid_path "$libdir/libexec"

require_line /usr/share/applications/org.kde.discover.desktop 'Icon=plasmadiscover'
if ! grep -Eq '^Exec=plasma-discover( |$)' \
    "$root/usr/share/applications/org.kde.discover.desktop"; then
    echo "desktop launcher must start plasma-discover" >&2
    exit 1
fi
require_line /etc/xdg/autostart/org.kde.discover.notifier.desktop \
    "Exec=$notifier_bin"
require_line /usr/share/applications/org.kde.discover.notifier.desktop \
    "Exec=$notifier_bin"
require_line /etc/xdg/discoverrc '[Software]'
require_line /etc/xdg/discoverrc 'UseOfflineUpdates=true'

# The local package handler is generated from the tools found at build time:
# it must offer RPM files and must not pick up dpkg from the build environment.
mime_types=$(sed -n 's/^MimeType=//p' \
    "$root/usr/share/applications/org.kde.discover.desktop")
case ";$mime_types" in
    *";application/x-rpm;"*) ;;
    *) echo "desktop launcher must handle application/x-rpm" >&2; exit 1 ;;
esac
case ";$mime_types" in
    *";application/vnd.debian.binary-package;"*)
        echo "desktop launcher must not handle Debian packages" >&2
        exit 1
        ;;
esac

# A build from a Git checkout gets a random interface id and the plugins would
# never load in the released binary.
backend_iid="org.kde.discover.$kde_base_version.AbstractResourcesBackendFactory"
notifier_iid="org.kde.discover.$kde_base_version.BackendNotifierModule"
for plugin in $backend_plugins; do
    if ! grep -aFq "$backend_iid" "$root$qt_plugindir/discover/$plugin.so"; then
        echo "$plugin.so does not carry the interface id $backend_iid" >&2
        exit 1
    fi
done
for plugin in $notifier_plugins; do
    if ! grep -aFq "$notifier_iid" "$root$qt_plugindir/discover-notifier/$plugin.so"; then
        echo "$plugin.so does not carry the interface id $notifier_iid" >&2
        exit 1
    fi
done

elf_files="$discover_bin
$notifier_bin
$private_libdir/libDiscoverCommon.so
$private_libdir/libDiscoverNotifiers.so
$qt_plugindir/plasma/kcms/systemsettings/kcm_updates.so"
for plugin in $backend_plugins; do
    elf_files="$elf_files
$qt_plugindir/discover/$plugin.so"
done
for plugin in $notifier_plugins; do
    elf_files="$elf_files
$qt_plugindir/discover-notifier/$plugin.so"
done

if [ "$installed" -eq 0 ]; then
    for elf_file in "$discover_bin" "$notifier_bin"; do
        if ! readelf -d "$root$elf_file" |
            grep -E '\((RUNPATH|RPATH)\)' | grep -Fq "[$private_libdir]"; then
            echo "$elf_file must search $private_libdir at run time" >&2
            exit 1
        fi
    done
fi

# "ldd -r" also performs the relocations, so it reports symbols that a plain
# "ldd" misses and that would only fail when Qt loads the plugin.
work_dir=$(mktemp -d)
cleanup() {
    rm -rf -- "$work_dir"
}
trap cleanup 0 1 2 15

printf '%s\n' "$elf_files" | while IFS= read -r elf_file; do
    if ! run_in_root ldd -r "$root$elf_file" > "$work_dir/ldd.log" 2>&1; then
        cat "$work_dir/ldd.log" >&2
        echo "ldd failed for $elf_file" >&2
        exit 1
    fi
    if grep -E 'not found|undefined symbol' "$work_dir/ldd.log" >&2; then
        echo "unresolved libraries or symbols in $elf_file" >&2
        exit 1
    fi
done

actual_version=$(run_in_root env QT_QPA_PLATFORM=offscreen LC_ALL=C \
    "$root$discover_bin" --version)
if [ "$actual_version" != "discover $kde_base_version" ]; then
    echo "unexpected --version output: $actual_version" >&2
    exit 1
fi

# --listbackends only lists the plugin files Qt can see. It proves the plugin
# search path, not that a plugin can be loaded.
run_in_root env QT_QPA_PLATFORM=offscreen LC_ALL=C \
    "$root$discover_bin" --listbackends > "$work_dir/backends.log"
for plugin in $backend_plugins; do
    if ! grep -Fxq " * $plugin" "$work_dir/backends.log"; then
        cat "$work_dir/backends.log" >&2
        echo "--listbackends does not list $plugin" >&2
        exit 1
    fi
done

if [ "$load_plugins" -eq 0 ]; then
    echo "package contents verified: KDE Discover $kde_base_version"
    exit 0
fi

# Real plugin loading. The programs have no self-test mode, so they run
# headless for a fixed time inside a private D-Bus session: they must still be
# alive when the timeout fires (exit status 124) and Qt must report every
# plugin as loaded.
if [ "$(id -u)" -eq 0 ]; then
    echo "--load-plugins must run as a regular user" >&2
    exit 2
fi
load_seconds=30

run_headless() {
    log_file=$1
    shift
    status=0
    run_in_root dbus-run-session -- env \
        QT_QPA_PLATFORM=offscreen \
        QT_FORCE_STDERR_LOGGING=1 \
        QT_DEBUG_PLUGINS=1 \
        LC_ALL=C.UTF-8 \
        timeout "$load_seconds" "$@" > "$log_file" 2>&1 || status=$?
    if [ "$status" -ne 124 ]; then
        cat "$log_file" >&2
        echo "$1 exited with status $status before the $load_seconds s timeout" >&2
        exit 1
    fi
}

require_loaded() {
    log_file=$1
    plugin_path=$2
    if ! grep -F "$plugin_path" "$log_file" | grep -Fq 'loaded library'; then
        grep -F "$plugin_path" "$log_file" >&2 || true
        echo "Qt did not load $plugin_path" >&2
        exit 1
    fi
}

forbid_load_errors() {
    if grep -E "doesn't have the right IID|error loading|Couldn't find the backend|Didn't find any Discover backend" "$1" >&2; then
        echo "Discover reported a plugin loading error" >&2
        exit 1
    fi
}

run_headless "$work_dir/discover.log" "$root$discover_bin" --mode Browsing
forbid_load_errors "$work_dir/discover.log"
for plugin in flatpak-backend fwupd-backend kns-backend packagekit-backend; do
    require_loaded "$work_dir/discover.log" "$qt_plugindir/discover/$plugin.so"
done

# The AppStream preview backend is only loaded on request.
run_headless "$work_dir/preview.log" "$root$discover_bin" \
    --backends appstream-preview-backend
forbid_load_errors "$work_dir/preview.log"
require_loaded "$work_dir/preview.log" \
    "$qt_plugindir/discover/appstream-preview-backend.so"

run_headless "$work_dir/notifier.log" "$root$notifier_bin"
for plugin in $notifier_plugins; do
    require_loaded "$work_dir/notifier.log" \
        "$qt_plugindir/discover-notifier/$plugin.so"
done

echo "package contents and plugin loading verified: KDE Discover $kde_base_version"
