#!/usr/bin/env bash
# Write manifest.json for a release from the assets in a directory.
#
#   scripts/write-manifest.sh TAG ASSET_DIR
#
# Installers read the latest release's manifest from a fixed URL, with no
# GitHub API call:
#   https://github.com/uairhahs/SyncLens/releases/latest/download/manifest.json
#
# One line per target, kept flat so shell scripts can read it without jq:
#   {"version":"0.2.5","targets":{
#     "arch-x86_64":{"url":"...pkg.tar.zst","sha256":"..."},
#     "linux-x86_64":{"url":"...tar.gz","sha256":"..."}}}
set -euo pipefail

tag="${1:?usage: write-manifest.sh TAG ASSET_DIR}"
dir="${2:?usage: write-manifest.sh TAG ASSET_DIR}"
repo="${GITHUB_REPOSITORY:-uairhahs/SyncLens}"
base="https://github.com/${repo}/releases/download/${tag}"

entry() { # target, file
	local sum
	sum="$(sha256sum "${dir}/$2" | cut -d' ' -f1)"
	printf '"%s":{"url":"%s/%s","sha256":"%s"}' "$1" "${base}" "$2" "${sum}"
}

targets=()
seen=" "
add() { # target, file
	if [[ ${seen} == *" $1 "* ]]; then
		echo "error: more than one asset for target $1 (${2})" >&2
		exit 1
	fi
	seen+="$1 "
	targets+=("$(entry "$1" "$2")")
}
shopt -s nullglob
# synclens-<digit>... only: makepkg can also build synclens-debug-*.
for f in "${dir}"/synclens-[0-9]*-x86_64.pkg.tar.zst; do
	add arch-x86_64 "${f##*/}"
done
for f in "${dir}"/synclens-[0-9]*-linux-*.tar.gz; do
	file="${f##*/}"
	machine="${file%.tar.gz}"
	machine="${machine##*-linux-}"
	add "linux-${machine}" "${file}"
done
[[ ${#targets[@]} -gt 0 ]] || {
	echo "error: no release assets in ${dir}" >&2
	exit 1
}

joined="$(
	IFS=,
	echo "${targets[*]}"
)"
printf '{"version":"%s","targets":{%s}}\n' "${tag#v}" "${joined}" >"${dir}/manifest.json"
cat "${dir}/manifest.json"
