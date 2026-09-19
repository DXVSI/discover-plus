#!/bin/sh
# SPDX-FileCopyrightText: 2026 DXVSI <https://github.com/DXVSI>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <output-directory>" >&2
    exit 2
fi

script_dir=$(unset CDPATH; cd -- "$(dirname -- "$0")" && pwd)
project_root=$(unset CDPATH; cd -- "$script_dir/../.." && pwd)
output_dir=$1

"$script_dir/check-release-contract.sh"

worktree_status=$(
    git -C "$project_root" status --porcelain --untracked-files=all
)
if [ -n "$worktree_status" ]; then
    echo "source archive requires a clean Git worktree" >&2
    exit 1
fi

# git runs inside the project root below while mkdir, tar and mv run in the
# directory of the caller: a relative output directory must mean the same
# place for all of them.
mkdir -p -- "$output_dir"
output_dir=$(unset CDPATH; cd -- "$output_dir" && pwd)

version=$(sed -n '1p' "$project_root/VERSION")
archive_name="discover-plus-$version.tar.xz"
archive_path=$output_dir/$archive_name
temporary_path=$archive_path.tmp
temporary_tar=$archive_path.tar.tmp

cleanup() {
    rm -f -- "$temporary_path" "$temporary_tar"
}
trap cleanup 0 1 2 15

# The package must be built from this archive and never from a Git checkout:
# with a .git directory present CMake generates a random plugin interface id.
git -C "$project_root" archive \
    --format=tar \
    --prefix="discover-plus-$version/" \
    --output="$temporary_tar" \
    HEAD

archive_members=$(tar -tf "$temporary_tar")
while IFS= read -r required_member; do
    archive_member="discover-plus-$version/$required_member"
    if ! printf '%s\n' "$archive_members" |
        grep -Fqx "$archive_member"; then
        echo "source archive is missing member: $required_member" >&2
        exit 1
    fi
done <<'MEMBERS'
CMakeLists.txt
VERSION
README.md
LICENSES/
po/
discover/FedoraRepoManager.cpp
libdiscover/backends/PackageKitBackend/CoprTransaction.cpp
packaging/rpm/discover-plus.spec
packaging/scripts/verify-package-contents.sh
MEMBERS

if printf '%s\n' "$archive_members" |
    grep -Eq "^discover-plus-$version/\.git(/|\$)"; then
    echo "source archive must not contain a .git entry" >&2
    exit 1
fi

xz -T1 -9 --check=crc64 --stdout "$temporary_tar" > "$temporary_path"
mv "$temporary_path" "$archive_path"
rm -f -- "$temporary_tar"
trap - 0 1 2 15

sha256sum "$archive_path"
