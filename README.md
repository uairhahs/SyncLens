# SyncLens

> An unofficial Qt desktop wrapper for the Resilio Sync WebUI on Linux

SyncLens gives Resilio Sync a proper Linux desktop home — system tray, a dedicated app window, connection health indicator, and secure credential storage — without replacing or reimplementing anything Resilio already does.

---

## Requirements

- Resilio Sync installed and running locally (any installation method)
- Qt 6.4 or later with QtWebEngine
- KDE / GNOME or any desktop with a system tray

---

## Installation

### From source

```bash
# 1. Clone
git clone https://github.com/uairhahs/SyncLens.git
cd SyncLens

# 2. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

# 3. Build
cmake --build build --parallel

# 4. Try it without installing (optional)
./build/synclens

# 5. Install
sudo cmake --install build
```

This installs the `synclens` binary, desktop entry, app icon and systemd user service. Omit `-DCMAKE_INSTALL_PREFIX=/usr` to install under `/usr/local` instead. Files installed this way are not tracked by your package manager.

**Optional dependency:** install `libsecret` (`libsecret-1-dev` on Debian/Ubuntu, `libsecret` on Arch) before configuring to enable secure password storage via GNOME Keyring or KWallet. Without it, only your username is remembered between sessions.

### Arch Linux

Build and install a package from the included `PKGBUILD`, run from a clone of this repository:

```bash
cd packaging
makepkg -si
```

### Nix

Run it directly:

```bash
nix run github:uairhahs/SyncLens/v0.1.3
```

On NixOS, add it as a flake input and include the package in your configuration:

```nix
# flake.nix
inputs.synclens = {
  url = "github:uairhahs/SyncLens/v0.1.3";
  inputs.nixpkgs.follows = "nixpkgs";
};

# configuration module (with `inputs` passed via specialArgs)
environment.systemPackages = [
  inputs.synclens.packages.${pkgs.stdenv.hostPlatform.system}.synclens
];
```

The `nix` command needs the `nix-command` and `flakes` experimental features enabled.

---

## First launch

1. SyncLens starts and checks whether Resilio Sync is reachable at `http://127.0.0.1:8888/gui/`.
2. If your WebUI is on a different address, go to **File > Settings** and update the URL.
3. When Resilio requires a password, a native sign-in dialog appears. Check **Remember password** to store it securely in your system keychain — you will not be asked again unless the password changes.

---

## Features

| Feature             | Details                                                                       |
| ------------------- | ----------------------------------------------------------------------------- |
| Embedded WebUI      | Full Resilio Sync interface inside a native window, no browser tab needed     |
| System tray         | One-click access, status indicator, open in browser, reconnect, quit          |
| Health monitor      | Detects when Resilio is stopped or unreachable and shows a clear offline page |
| Credential storage  | Passwords stored in GNOME Keyring or KWallet via libsecret (when available)   |
| Session persistence | Stays signed in across restarts using a persistent browser profile            |
| Window geometry     | Remembers size and position between sessions                                  |
| Start minimized     | Launch straight to tray, configurable in Settings                             |
| Launch on login     | XDG autostart entry written automatically; optional systemd user service      |

---

## Done

- [x] Make a logo and app icon to replace the default Qt one and give it a distinct design language
- [x] Add support for session persistence across multiple Sync instances with separate profiles and tray icons
- [x] Add a "Quit SyncLens" option to the tray menu that gracefully shuts down the application
- [x] Run as a service so it can be enabled at login and can run in the background
- [x] Implement a "Open WebUI in browser" option that launches the default browser to the WebUI URL
- [x] Add a "Refresh" option to the tray menu to manually check the connection status
- [x] Set up arch pkgbuild and deb/rpm spec files for easier distribution
- [x] A "Reconnect" option that starts a fresh WebUI renderer, plus automatic recovery after an outage or renderer crash
- [x] Check for updates: a daily check against GitHub releases, and **Help > Check for Updates**
- [x] Icon themes can restyle the app and tray icons (see [Icon themes](#icon-themes))
- [x] WebUI colour themes, including Catppuccin, plus custom themes and a user stylesheet (see [Themes](#themes))

## Tray icon states

| Colour | Centre | Meaning                                            |
| ------ | ------ | -------------------------------------------------- |
| Green  | Dot    | Resilio Sync is running and the WebUI is reachable |
| Amber  | Ring   | Connecting or status unknown                       |
| Grey   | Pause  | Daemon is stopped                                  |
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

- **Name** — label shown in the window title and tray tooltip for this instance
- **WebUI URL** — change if Resilio is bound to a non-default address or port
- **Start minimized to tray** — launch without showing the main window
- **Launch on login** — writes `~/.config/autostart/synclens.desktop` so your desktop environment starts SyncLens automatically; uncheck to remove it
- **Desktop notifications** — toggle wrapper notifications on or off
- **WebUI theme** — recolour the WebUI (see [Themes](#themes))
- **Check for updates daily** — ask GitHub once a day whether a newer release exists, and notify once per new version. **Help > Check for Updates** checks on demand

**View > Reconnect** (F5), also in the tray menu, reloads the WebUI in a fresh renderer. SyncLens does this on its own when the WebUI comes back after an outage or the renderer crashes.

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

## Disclaimer

SyncLens is an independent, community-developed project. It is **not affiliated with, endorsed by, or sponsored by Resilio, Inc.** "Resilio" and "Resilio Sync" are trademarks of Resilio, Inc. All rights reserved by their respective owners.
