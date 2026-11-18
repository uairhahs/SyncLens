#!/usr/bin/env bash
# Install SyncLens from the latest GitHub release, or update it. Run the same
# command again to update.
#
#   curl -fsSL https://raw.githubusercontent.com/uairhahs/SyncLens/main/scripts/install.sh | bash
#
# Two ways to install, picked automatically:
#   Arch package   on Arch (pacman), via sudo pacman -U. Default there.
#   Per-user       anywhere else, or with --user: into ~/.local, no root.
#                  Needs the distribution's Qt 6.4+ with QtWebEngine.
#
# Options:
#   --user            per-user install even on Arch
#   --version vX.Y.Z  install that release instead of the latest
#   --force           reinstall, or downgrade
#   --uninstall       remove a per-user install
#
# Downloads are checked against the SHA-256 in the release's manifest.json.
set -euo pipefail

REPO="uairhahs/SyncLens"
RELEASES="${SYNCLENS_RELEASES_URL:-https://github.com/${REPO}/releases}"
PREFIX="${SYNCLENS_PREFIX:-${HOME}/.local}"
DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
STATE_DIR="${DATA_HOME}/synclens"
FILES_LIST="${STATE_DIR}/installed-files"

TAG=""
FORCE=0
NEW_ICON_DIRS=0
MODE=""
UNINSTALL=0

say() { printf '%s\n' "$*"; }
die() {
	say "error: $*" >&2
	exit 1
}
usage() { sed -n '2,18s/^# \{0,1\}//p' "$0"; }

while [[ $# -gt 0 ]]; do
	case "$1" in
	--user)
		MODE=user
		shift
		;;
	--version)
		TAG="${2:?--version needs a tag, e.g. v0.2.5}"
		[[ ${TAG} == v* ]] || TAG="v${TAG}"
		shift 2
		;;
	--force)
		FORCE=1
		shift
		;;
	--uninstall)
		UNINSTALL=1
		shift
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		say "Unknown option: $1" >&2
		usage >&2
		exit 2
		;;
	esac
done

refresh_user_caches() {
	if command -v update-desktop-database >/dev/null; then
		update-desktop-database -q "${DATA_HOME}/applications" 2>/dev/null || true
	fi
	if command -v gtk-update-icon-cache >/dev/null && [[ -f "${DATA_HOME}/icons/hicolor/index.theme" ]]; then
		gtk-update-icon-cache -q "${DATA_HOME}/icons/hicolor" 2>/dev/null || true
	fi
	if command -v systemctl >/dev/null; then
		systemctl --user daemon-reload 2>/dev/null || true
	fi
}

# Icon folders the install puts files in.
icon_dirs() {
	local base="${DATA_HOME}/icons/hicolor"
	[[ ${MODE} == arch ]] && base=/usr/share/icons/hicolor
	printf '%s\n' "${base}/scalable/apps" "${base}/scalable/status"
}

# A running Plasma caches icon lookups, misses included, so without this the
# tray and taskbar icons stay blank until the next login.
refresh_plasma() {
	pgrep -u "${UID}" -x plasmashell >/dev/null 2>&1 || return 0
	local sycoca
	for sycoca in kbuildsycoca6 kbuildsycoca5; do
		if command -v "${sycoca}" >/dev/null; then
			"${sycoca}" >/dev/null 2>&1 || true
			break
		fi
	done
	if command -v dbus-send >/dev/null; then
		dbus-send --session --type=signal /KIconLoader org.kde.KIconLoader.iconChanged int32:0 2>/dev/null || true
	fi
	# Plasma picks the icon folders to search when it starts, so the signal
	# above can't reach icons in a folder this install created.
	if [[ ${NEW_ICON_DIRS} -eq 1 ]]; then
		say "==> Plasma only searches icon folders that existed when it started, so the"
		say "    SyncLens icons may be blank until you log in again, or run:"
		say "    systemctl --user restart plasma-plasmashell.service"
	fi
}

# ---- per-user uninstall ----------------------------------------------------

if [[ ${UNINSTALL} -eq 1 ]]; then
	[[ -f ${FILES_LIST} ]] || die "no per-user install found (${FILES_LIST} missing)"
	while read -r f; do
		[[ -n ${f} ]] && rm -f "${f}"
	done <"${FILES_LIST}"
	rm -rf "${STATE_DIR}"
	refresh_user_caches
	say "==> Removed the per-user install. Settings in ~/.config/SyncLens were kept."
	exit 0
fi

# ---- pick the install mode -------------------------------------------------

command -v curl >/dev/null || die "curl is required"
command -v sha256sum >/dev/null || die "sha256sum is required"
machine="$(uname -m)"
[[ ${machine} == "x86_64" ]] || die "release builds are x86_64 only; build from source: https://github.com/${REPO}#from-source"

pacman_owned=0
if command -v pacman >/dev/null && pacman -Q synclens >/dev/null 2>&1; then
	pacman_owned=1
fi
if [[ -z ${MODE} ]]; then
	if command -v pacman >/dev/null; then MODE=arch; else MODE=user; fi
fi
# One copy per machine: two would shadow each other on PATH.
if [[ ${MODE} == user && ${pacman_owned} -eq 1 ]]; then
	die "SyncLens is installed as an Arch package; update it without --user, or remove it first (sudo pacman -R synclens)"
fi
if [[ ${MODE} == arch && -f ${FILES_LIST} ]]; then
	die "a per-user install exists in ${PREFIX}; update it with --user, or remove it first (--uninstall)"
fi

# ---- read the manifest -----------------------------------------------------

if [[ -n ${TAG} ]]; then
	manifest_url="${RELEASES}/download/${TAG}/manifest.json"
else
	manifest_url="${RELEASES}/latest/download/manifest.json"
fi
say "==> Reading ${manifest_url}"
manifest="$(curl -fsSL "${manifest_url}")" || die "cannot read the release manifest (does the release exist?)"
# Flat enough for sed once whitespace is gone (see scripts/write-manifest.sh).
flat="$(tr -d '\n ' <<<"${manifest}")"
want="$(sed -n 's/^{"version":"\([^"]*\)".*/\1/p' <<<"${flat}")"
if [[ ${MODE} == arch ]]; then target="arch-${machine}"; else target="linux-${machine}"; fi
matches="$(grep -o "\"${target}\":{" <<<"${flat}" || true)"
count="$(wc -l <<<"${matches}")"
[[ ${count} -le 1 ]] || die "the release manifest lists ${target} more than once; not guessing which to install"
entry="$(sed -n "s/.*\"${target}\":{\([^}]*\)}.*/\1/p" <<<"${flat}")"
url="$(sed -n 's/.*"url":"\([^"]*\)".*/\1/p' <<<"${entry}")"
sum="$(sed -n 's/.*"sha256":"\([^"]*\)".*/\1/p' <<<"${entry}")"
[[ -n ${want} && -n ${url} && -n ${sum} ]] || die "release ${TAG:-latest} has no ${target} build"

# ---- compare versions ------------------------------------------------------

have=""
if [[ ${MODE} == arch && ${pacman_owned} -eq 1 ]]; then
	have="$(pacman -Q synclens | awk '{print $2}')"
	have="${have%-*}"
elif [[ ${MODE} == user && -f "${STATE_DIR}/VERSION" ]]; then
	have="$(<"${STATE_DIR}/VERSION")"
fi
if [[ -n ${have} && ${FORCE} -eq 0 ]]; then
	if [[ ${have} == "${want}" ]]; then
		say "==> SyncLens ${have} is already installed and up to date."
		exit 0
	fi
	newest="$(printf '%s\n%s\n' "${have}" "${want}" | sort -V | tail -n1)"
	[[ ${newest} == "${want}" ]] || die "${want} is older than the installed ${have}; use --force to downgrade"
fi

# ---- download and verify ---------------------------------------------------

tmp="$(mktemp -d)"
trap 'rm -rf "${tmp}"' EXIT
file="${tmp}/${url##*/}"
say "==> Downloading ${url##*/}"
curl -fL --progress-bar -o "${file}" "${url}" || die "download failed: ${url}"
got="$(sha256sum "${file}" | cut -d' ' -f1)"
[[ ${got} == "${sum}" ]] || die "checksum mismatch, not installing (try again)"
say "==> Checksum OK."

# ---- Arch package ----------------------------------------------------------

install_arch() {
	# Files that exist but belong to no package (a previous `cmake --install`)
	# would make pacman refuse to install; let the package take them over.
	local overwrite=() pkg_files path
	pkg_files="$(pacman -Qlp "${file}")"
	while read -r _ path; do
		[[ ${path} == */ ]] && continue
		if [[ -e ${path} ]] && ! pacman -Qo "${path}" >/dev/null 2>&1; then
			overwrite+=("${path}")
		fi
	done <<<"${pkg_files}"

	local args=(-U "${file}")
	if [[ ${#overwrite[@]} -gt 0 ]]; then
		say "==> Replacing files from a manual install:"
		printf '    %s\n' "${overwrite[@]}"
		args+=(--overwrite "$(
			IFS=,
			echo "${overwrite[*]}"
		)")
	fi
	sudo pacman "${args[@]}"
}

# ---- per-user tarball ------------------------------------------------------

install_user() {
	tar -xzf "${file}" -C "${tmp}"
	local root
	root="$(find "${tmp}" -mindepth 1 -maxdepth 1 -type d -name 'synclens-*' -print -quit)"
	[[ -x "${root}/bin/synclens" ]] || die "the tarball has no bin/synclens"

	# The binary uses the system's Qt: check every library resolves (Qt 6.4+,
	# QtWebEngine, libsecret, glibc) before replacing anything.
	local libs missing
	libs="$(ldd "${root}/bin/synclens" 2>&1 || true)"
	missing="$(grep -E 'not found' <<<"${libs}" || true)"
	if [[ -n ${missing} ]]; then
		say "error: SyncLens needs libraries this system lacks:" >&2
		say "${missing}" >&2
		say "Install Qt 6.4 or later with QtWebEngine and QtSvg, and libsecret, for example:" >&2
		say "  Debian/Ubuntu: sudo apt install libqt6webenginewidgets6 libqt6svg6 libsecret-1-0" >&2
		say "  Fedora:        sudo dnf install qt6-qtwebengine qt6-qtsvg libsecret" >&2
		exit 1
	fi

	local bin="${PREFIX}/bin/synclens"
	mkdir -p "${PREFIX}/bin" "${STATE_DIR}"
	# Atomic replace: a running SyncLens keeps its old binary until restarted.
	install -m755 "${root}/bin/synclens" "${bin}.new"
	mv -f "${bin}.new" "${bin}"
	local list="${FILES_LIST}.new"
	echo "${bin}" >"${list}"

	local share_files f dest
	share_files="$(find "${root}/share" -type f)"
	while read -r f; do
		dest="${DATA_HOME}/${f#"${root}/share/"}"
		install -Dm644 "${f}" "${dest}"
		echo "${dest}" >>"${list}"
	done <<<"${share_files}"

	# Launchers and the user service run without the shell's PATH, so point
	# them at the binary directly.
	sed -i "s#^Exec=.*#Exec=${bin}#" "${DATA_HOME}/applications/synclens.desktop"
	sed -i "s#^ExecStart=.*#ExecStart=${bin}#" "${DATA_HOME}/systemd/user/synclens.service"

	# Remove files the previous version installed but this one does not.
	if [[ -f ${FILES_LIST} ]]; then
		local stale
		stale="$(grep -vxF -f "${list}" "${FILES_LIST}" || true)"
		while read -r f; do
			[[ -n ${f} ]] && rm -f "${f}"
		done <<<"${stale}"
	fi
	mv -f "${list}" "${FILES_LIST}"
	echo "${want}" >"${STATE_DIR}/VERSION"
	refresh_user_caches

	case ":${PATH}:" in
	*":${PREFIX}/bin:"*) ;;
	*) say "==> Add ${PREFIX}/bin to your PATH to run synclens from a terminal." ;;
	esac
	if [[ -n ${have} ]]; then
		say "==> A running SyncLens keeps the old version until you quit and restart it."
	fi
	say "==> Remove it later with: scripts/install.sh --uninstall"
}

if [[ -n ${have} ]]; then
	say "==> Updating SyncLens ${have} -> ${want} (${MODE})"
else
	say "==> Installing SyncLens ${want} (${MODE})"
fi
icon_paths="$(icon_dirs)"
while read -r dir; do
	[[ -d ${dir} ]] || NEW_ICON_DIRS=1
done <<<"${icon_paths}"
if [[ ${MODE} == arch ]]; then install_arch; else install_user; fi
refresh_plasma
say "==> SyncLens ${want} is installed."
