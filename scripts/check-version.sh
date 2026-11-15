#!/usr/bin/env bash
# Check that every file carrying the release version agrees with CMakeLists.txt.
#
#   scripts/check-version.sh           compare the files with each other
#   scripts/check-version.sh v0.2.5    also require them to match this tag
set -euo pipefail
cd "$(dirname "$0")/.."

version="$(sed -n 's/^project(SyncLens VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[[ -n $version ]] || {
	echo "error: no version in CMakeLists.txt" >&2
	exit 1
}

fail=0
check() { # file, value found, what it is
	if [[ $2 != "$version" ]]; then
		echo "mismatch: $1 has $3 '${2:-<none>}', CMakeLists.txt has '$version'" >&2
		fail=1
	else
		echo "ok: $1 ($3 $2)"
	fi
}

pkgver="$(sed -n 's/^pkgver=//p' packaging/PKGBUILD)"
check packaging/PKGBUILD "${pkgver}" pkgver
nixver="$(sed -n 's/^ *version = "\(.*\)";/\1/p' nix/package.nix)"
check nix/package.nix "${nixver}" version
refs="$(grep -o 'SyncLens/v[0-9.]*[0-9]' README.md || true)"
while read -r ref; do
	[[ -n ${ref} ]] || continue
	check README.md "${ref#SyncLens/v}" "tag reference"
done <<<"${refs}"

if [[ $# -gt 0 ]]; then
	check "git tag" "${1#v}" tag
fi

exit "$fail"
