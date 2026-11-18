#!/usr/bin/env bash
# Build the per-user release tarball: synclens-<version>-linux-x86_64.tar.gz
#
#   scripts/build-tarball.sh [SOURCE_DIR] [OUT_DIR]
#
# SOURCE_DIR defaults to this repository and OUT_DIR to ./dist. The tarball
# holds a relocatable tree for scripts/install.sh --user:
#
#   bin/synclens
#   share/applications/synclens.desktop
#   share/icons/hicolor/scalable/{apps,status}/synclens*.svg
#   share/systemd/user/synclens.service   ($XDG_DATA_HOME/systemd/user)
#   share/licenses/synclens/LICENSE
#   VERSION
#
# The binary links against the system Qt, so build it against the oldest Qt
# the tarball should run on (CI uses Debian 13, Qt 6.8).
set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
src="$(cd "${1:-${here}}" && pwd)"
out="${2:-${PWD}/dist}"
mkdir -p "${out}"
out="$(cd "${out}" && pwd)"

version="$(sed -n 's/^project(SyncLens VERSION \([0-9.]*\).*/\1/p' "${src}/CMakeLists.txt")"
[[ -n ${version} ]] || {
	echo "error: no version in ${src}/CMakeLists.txt" >&2
	exit 1
}
machine="$(uname -m)"
name="synclens-${version}-linux-${machine}"

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
stage="${work}/${name}"

generator=()
if command -v ninja >/dev/null; then generator=(-G Ninja); fi
cmake -S "${src}" -B "${work}/build" "${generator[@]}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${work}/build"
cmake --install "${work}/build" --prefix "${stage}" --strip

# systemd looks for per-user units in $XDG_DATA_HOME/systemd/user, not lib/.
mkdir -p "${stage}/share/systemd/user"
mv "${stage}/lib/systemd/user/synclens.service" "${stage}/share/systemd/user/"
rm -rf "${stage:?}/lib"

install -Dm644 "${src}/LICENSE" "${stage}/share/licenses/synclens/LICENSE"
echo "${version}" >"${stage}/VERSION"

tar -C "${work}" -czf "${out}/${name}.tar.gz" "${name}"
(cd "${out}" && sha256sum "${name}.tar.gz" >"${name}.tar.gz.sha256")
echo "${out}/${name}.tar.gz"
