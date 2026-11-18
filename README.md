# SyncLens

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

> An unofficial Qt desktop wrapper for the Resilio Sync WebUI on Linux

SyncLens gives Resilio Sync a proper Linux desktop home (system tray, a dedicated app window, connection health indicator and secure credential storage) without replacing or reimplementing anything Resilio already does.

---

## Requirements

- Resilio Sync running locally or reachable over the network (any installation method)
- Qt 6.8 or later with QtWebEngine (Debian 13, Ubuntu 25.10, Fedora 41 or later, Arch; on older distributions such as Ubuntu 24.04, use the [Nix package](#nix), which brings its own Qt)
- KDE / GNOME or any desktop with a system tray

---

## Installation

### Install script (Arch and other distributions)

Install the latest release, or update to it:

```bash
curl -fsSL https://raw.githubusercontent.com/uairhahs/SyncLens/main/scripts/install.sh | bash
```

Run the same command again whenever SyncLens tells you a new version is out. From a clone, run `scripts/install.sh` instead. The script reads the latest [release](https://github.com/uairhahs/SyncLens/releases)'s `manifest.json`, checks the download against the SHA-256 listed there, and installs it one of two ways:

- **On Arch** it installs the Arch package with `sudo pacman -U`, so pacman tracks the files and dependencies. If SyncLens was previously installed with `cmake --install`, the package takes over those files.
- **On other distributions**, or with `--user` on Arch, it installs into `~/.local` for your user only, with no root. This needs your distribution's Qt 6.8 or later with QtWebEngine and QtSvg, plus libsecret; the script checks they are present before installing. For example, on Debian 13 or Ubuntu 25.10 or later: `sudo apt install libqt6webenginewidgets6 libqt6svg6 libsecret-1-0`.

| Option             | Effect                                        |
| ------------------ | --------------------------------------------- |
| `--version v0.2.6` | Install that release instead of the latest    |
| `--force`          | Reinstall the same version, or downgrade      |
| `--user`           | Per-user install even on Arch                 |
| `--uninstall`      | Remove a per-user install (settings are kept) |

Keep one copy per machine: the script refuses a per-user install while the Arch package is installed, and the other way round.

### Arch package by hand

Download the `.pkg.tar.zst` from the release page and run `sudo pacman -U synclens-*.pkg.tar.zst`, or build the package yourself from a clone (this packages the code you have checked out):

```bash
cd packaging
makepkg -si
```

To update, `git pull` and run `makepkg -si` again. After an upgrade pacman reminds you to restart SyncLens, since a running copy keeps the old version.

### Nix

Run it directly:

```bash
nix run github:uairhahs/SyncLens/v0.2.6
```

On NixOS, add it as a flake input and include the package in your configuration:

```nix
# flake.nix
inputs.synclens = {
  url = "github:uairhahs/SyncLens/v0.2.6";
  inputs.nixpkgs.follows = "nixpkgs";
};

# configuration module (with `inputs` passed via specialArgs)
environment.systemPackages = [
  inputs.synclens.packages.${pkgs.stdenv.hostPlatform.system}.synclens
];
```

The `nix` command needs the `nix-command` and `flakes` experimental features enabled.

### From source

Install the build dependencies. On Debian 13 or Ubuntu 25.10 or later:

```bash
sudo apt install cmake ninja-build pkg-config qt6-base-dev qt6-webengine-dev qt6-svg-dev libgl-dev libsecret-1-dev
```

Then build and install:

```bash
git clone https://github.com/uairhahs/SyncLens.git
cd SyncLens
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
./build/synclens            # optional: try it without installing
sudo cmake --install build
```

This installs the `synclens` binary, desktop entry, app icon and systemd user service. Omit `-DCMAKE_INSTALL_PREFIX=/usr` to install under `/usr/local` instead. Files installed this way are not tracked by your package manager.

- **Update:** `git pull`, then run the `cmake --build` and `sudo cmake --install` lines again.
- **Uninstall:** `sudo xargs rm < build/install_manifest.txt`.

**libsecret is optional** (`libsecret-1-dev` on Debian/Ubuntu, `libsecret` on Arch). Install it before configuring to enable secure password storage via GNOME Keyring or KWallet. Without it, only your username is remembered between sessions.

---

## First launch

1. SyncLens starts and checks whether Resilio Sync is reachable at `http://127.0.0.1:8888/gui/`.
2. If your WebUI is on a different address, go to **File > Settings** and update the URL.
3. When Resilio requires a password, a native sign-in dialog appears. Check **Remember password** to store it securely in your system keychain; you will not be asked again unless the password changes.

---

## Features

- **Embedded WebUI**: the full Resilio Sync interface in a native window, no browser tab needed
- **Multiple instances**: connect to several Resilio Sync servers, local or remote, each with its own isolated browser session (see [Multiple instances](#multiple-instances))
- **One tray icon**: left-click opens the default instance; right-click lists every instance with a coloured status dot, plus Quit
- **Health monitor**: detects when Resilio is stopped or unreachable and shows a clear offline page
- **Reconnect and recovery**: **View > Reconnect** starts a fresh WebUI renderer, and SyncLens does this on its own after an outage or renderer crash
- **Credential storage**: passwords kept per instance in GNOME Keyring or KWallet via libsecret (when available)
- **Session persistence**: stays signed in across restarts using a persistent browser profile per instance
- **WebUI themes**: Catppuccin and other bundled palettes, custom themes and a user stylesheet (see [Themes](#themes))
- **Icon themes**: your icon theme can restyle the app and tray icons (see [Icon themes](#icon-themes))
- **Update checks**: a daily check against GitHub releases, plus **Help > Check for Updates**
- **Start minimized and launch on login**: start straight to the tray, via an XDG autostart entry or the optional systemd user service
- **One copy at a time**: launching SyncLens while it runs, from the menu, autostart or the service, shows the running window instead of starting a second copy
- **Window geometry**: remembers each instance window's size and position
- **Packaging**: Arch `PKGBUILD` and a Nix flake

---

## Tray icon states

| Colour | Centre | Meaning                                            |
| ------ | ------ | -------------------------------------------------- |
| Green  | Dot    | Resilio Sync is running and the WebUI is reachable |
| Amber  | Ring   | Connecting or status unknown                       |
| Blue   | Pause  | Daemon is stopped                                  |
| Red    | Cross  | WebUI is not responding or an error occurred       |

Icons are SVGs in `resources/icons/`; run `uv run resources/icons/generate_icons.py` to regenerate them.

### Icon themes

SyncLens loads its icons from your icon theme first and falls back to its bundled copies, so a theme can restyle them by providing icons with these names:

| Icon name                  | Used for                    |
| -------------------------- | --------------------------- |
| `synclens`                 | App icon and notifications  |
| `synclens-tray-healthy`    | Tray: running               |
| `synclens-tray-connecting` | Tray: connecting or unknown |
| `synclens-tray-stopped`    | Tray: daemon stopped        |
| `synclens-tray-error`      | Tray: unreachable or error  |

The installed tray icons use KDE's `current-color-scheme` stylesheet, so Plasma paints their arrows in the panel's text colour.

---

## Themes

**File > Settings > WebUI theme** recolours the Resilio WebUI. Bundled themes:

- Catppuccin Latte, Frappé, Macchiato and Mocha
- Dracula, Nord, Tokyo Night, Rosé Pine
- Gruvbox Dark and Light, Solarized Dark and Light

Changes apply straight away, without reloading. A theme also chooses light or dark, so it takes over from Resilio's own **Settings > General > Appearance**; pick **Resilio default** to hand that back.

Resilio's stylesheets hard-code every colour, so a theme is a palette rather than CSS: SyncLens rewrites each colour in Resilio's CSS, and in its SVG icons, to the nearest theme colour. Greys are placed on the theme's grey ramp by lightness, and saturated colours take the accent with the nearest hue.

### Custom themes

**Open Themes Folder** in Settings opens `~/.config/SyncLens/themes/`. Each `.json` file there is a theme, listed under its `name`; one with the same file name as a bundled theme replaces it. Edits are picked up while SyncLens runs.

```json
{
  "name": "My Theme",
  "appearance": "dark",
  "primary": "#cba6f7",
  "greys": ["#11111b", "#1e1e2e", "#313244", "#585b70", "#a6adc8", "#cdd6f4"],
  "accents": ["#89b4fa", "#f38ba8", "#a6e3a1", "#f9e2af", "#fab387", "#cba6f7"],
  "colors": { "#e54545": "#f38ba8" }
}
```

| Key          | Meaning                                                                                                      |
| ------------ | ------------------------------------------------------------------------------------------------------------ |
| `appearance` | `dark` or `light`: which of Resilio's two layouts to recolour                                                |
| `greys`      | Two or more neutral colours, in any order. Resilio's black maps to the darkest and its white to the lightest |
| `accents`    | One or more colours. Include a blue, red, green and yellow, since Resilio uses all four                      |
| `primary`    | Optional highlight colour. Resilio highlights in blue; this replaces it, e.g. mauve for Catppuccin           |
| `colors`     | Optional exact replacements for specific Resilio colours, checked first                                      |

### User stylesheet

`~/.config/SyncLens/user.css` is applied on top of the WebUI, with or without a theme, and reloaded when it changes. Resilio's class names are not a stable interface, so rules that target them can break when Resilio updates.

---

## Settings

Open **File > Settings** to configure:

- **Name**: label shown in the window title and tray tooltip for this instance
- **WebUI URL**: change if Resilio is bound to a non-default address or port
- **Start minimized to tray**: launch without showing the main window
- **Launch on login**: writes `~/.config/autostart/synclens.desktop` so your desktop environment starts SyncLens automatically; uncheck to remove it
- **Desktop notifications**: toggle wrapper notifications on or off
- **WebUI theme**: recolour the WebUI (see [Themes](#themes))
- **Check for updates daily**: ask GitHub once a day whether a newer release exists, and notify once per new version. **Help > Check for Updates** checks on demand

**View > Reconnect** (F5) reloads the WebUI in a fresh renderer. SyncLens does this on its own when the WebUI comes back after an outage or the renderer crashes.

## Multiple instances

**File > Manage Instances** adds, edits and removes Resilio Sync connections. Pressing **OK** switches to the highlighted instance. Each instance has its own window and browser session, and they share one tray icon. Only one instance window is open at a time: switching to another closes the current one to the tray.

- Left-click the tray icon to open the default instance. Select an instance and click **Set as Default** to choose which one.
- Right-click for the menu, which lists every instance with a coloured status dot; choose one to switch to it.
- The icon shows the worst state across all instances, and the tooltip lists each one.

Saved passwords are stored per instance URL and removed when you remove the instance.

---

## Running as a systemd user service (optional)

After installing SyncLens, you can manage it as a proper background service:

```bash
# Enable autostart via systemd (alternative to the Settings checkbox)
systemctl --user enable --now synclens.service

# Check status
systemctl --user status synclens.service

# Disable
systemctl --user disable --now synclens.service
```

The service unit is installed to `$PREFIX/lib/systemd/user/synclens.service` and targets `graphical-session.target`, so it starts only after your desktop session is ready.

---

## Contributing

Changes go through pull requests to `main`; see [CONTRIBUTING.md](CONTRIBUTING.md) for the branch rules, checks and release steps.

---

## Disclaimer

SyncLens is an independent, community-developed project. It is **not affiliated with, endorsed by, or sponsored by Resilio, Inc.** "Resilio" and "Resilio Sync" are trademarks of Resilio, Inc. All rights reserved by their respective owners.
