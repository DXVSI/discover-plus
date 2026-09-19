#!/bin/sh
# SPDX-FileCopyrightText: 2026 DXVSI <https://github.com/DXVSI>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

set -eu

if [ "$#" -gt 1 ]; then
    echo "usage: $0 [plus-vX.Y.Z]" >&2
    exit 2
fi

script_dir=$(unset CDPATH; cd -- "$(dirname -- "$0")" && pwd)
project_root=$(unset CDPATH; cd -- "$script_dir/../.." && pwd)
version_file=$project_root/VERSION
spec_file=$project_root/packaging/rpm/discover-plus.spec

version=$(sed -n '1p' "$version_file")
if ! grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$' "$version_file" ||
   [ "$(wc -l < "$version_file")" -ne 1 ]; then
    echo "VERSION must contain one semantic version line" >&2
    exit 1
fi

rpm_version=$(awk '$1 == "Version:" { print $2; exit }' "$spec_file")
if [ "$rpm_version" != "$version" ]; then
    echo "RPM version $rpm_version does not match VERSION $version" >&2
    exit 1
fi

# The plugin interface id and the "--version" output come from PROJECT_VERSION,
# which stays the KDE Discover version. The spec keeps its own copy for the
# package checks, so the two must not drift apart after an upstream merge.
kde_base_version=$(awk '$1 == "%global" && $2 == "kde_base_version" { print $3; exit }' \
    "$spec_file")
project_version=$(sed -n \
    's/^set(PROJECT_VERSION "\([^"]*\)")[[:space:]]*$/\1/p' \
    "$project_root/CMakeLists.txt")
if [ -z "$project_version" ] ||
   [ "$(printf '%s\n' "$project_version" | wc -l)" -ne 1 ]; then
    echo "CMakeLists.txt must set PROJECT_VERSION exactly once" >&2
    exit 1
fi
if [ "$kde_base_version" != "$project_version" ]; then
    echo "spec kde_base_version $kde_base_version does not match CMake PROJECT_VERSION $project_version" >&2
    exit 1
fi

# Release is the packaging counter of one VERSION: an integer followed by the
# dist tag. CI builds the package file names from it.
rpm_release=$(awk '$1 == "Release:" { print $2; exit }' "$spec_file")
case $rpm_release in
    *'%{?dist}') rpm_release=${rpm_release%'%{?dist}'} ;;
    *) echo "spec Release must end with %{?dist}: $rpm_release" >&2; exit 1 ;;
esac
if ! printf '%s\n' "$rpm_release" | grep -Eq '^[1-9][0-9]*$'; then
    echo "spec Release must be a positive integer before %{?dist}: $rpm_release" >&2
    exit 1
fi

changelog_version=$(awk '
    found && /^\*/ { print $NF; exit }
    $1 == "%changelog" { found = 1 }
' "$spec_file")
if [ "$changelog_version" != "$version-$rpm_release" ]; then
    echo "first %changelog entry $changelog_version does not match $version-$rpm_release" >&2
    exit 1
fi

# CI and %check run the scripts directly, so a lost executable bit must fail
# here and not as "Permission denied" in the middle of a release.
for script in \
    check-release-contract.sh \
    create-source-archive.sh \
    verify-package-contents.sh
do
    if [ ! -x "$script_dir/$script" ]; then
        echo "packaging/scripts/$script is not executable" >&2
        exit 1
    fi
done
if git -C "$project_root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    non_executable=$(git -C "$project_root" ls-files --stage -- \
        'packaging/scripts/*.sh' | awk '$1 != "100755" { print $4 }')
    if [ -n "$non_executable" ]; then
        echo "scripts without the executable bit in the Git index:" >&2
        printf '%s\n' "$non_executable" >&2
        exit 1
    fi
fi

if [ "$#" -eq 1 ] && [ "$1" != "plus-v$version" ]; then
    echo "tag $1 does not match VERSION $version" >&2
    exit 1
fi

echo "release contract verified: $version-$rpm_release (KDE Discover $kde_base_version)"
