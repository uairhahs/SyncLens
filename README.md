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
nix run github:uairhahs/SyncLens/v0.1.1
```

On NixOS, add it as a flake input and include the package in your configuration:

```nix
# flake.nix
inputs.synclens = {
  url = "github:uairhahs/SyncLens/v0.1.1";
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
| System tray         | One-click access, status indicator, open in browser, quit                     |
| Health monitor      | Detects when Resilio is stopped or unreachable and shows a clear offline page |
| Credential storage  | Passwords stored in GNOME Keyring or KWallet via libsecret (when available)   |
| Session persistence | Stays signed in across restarts using a persistent browser profile            |
| Window geometry     | Remembers size and position between sessions                                  |
| Start minimized     | Launch straight to tray, configurable in Settings                             |
| Launch on login     | XDG autostart entry written automatically; optional systemd user service      |

---

## Done

- [X] Make a logo and app icon to replace the default Qt one and give it a distinct design language
- [X] Add support for session persistence across multiple Sync instances with separate profiles and tray icons
- [X] Add a "Quit SyncLens" option to the tray menu that gracefully shuts down the application
- [X] Run as a service so it can be enabled at login and can run in the background
- [X] Implement a "Open WebUI in browser" option that launches the default browser to the WebUI URL
- [X] Add a "Refresh" option to the tray menu to manually check the connection status
- [X] Set up arch pkgbuild and deb/rpm spec files for easier distribution

## Todo

- [ ] Implement a dark mode toggle that applies a custom stylesheet to the WebUI
- [ ] Implement a "Reconnect" option that restarts the SyncLens process if the WebUI becomes unresponsive
- [ ] Implement a "Check for updates" feature that notifies users when a new version of SyncLens is available on GitHub
- [ ] Add support for custom icons and themes to better integrate with different desktop environments

## Tray icon states

| Colour | Centre | Meaning                                            |
| ------ | ------ | -------------------------------------------------- |
| Green  | Dot    | Resilio Sync is running and the WebUI is reachable |
| Amber  | Ring   | Connecting or status unknown                       |
| Grey   | Pause  | Daemon is stopped                                  |
| Red    | Cross  | WebUI is not responding or an error occurred       |

Icon sources are in `resources/icons/src/`; run `resources/icons/generate_icons.py` to regenerate them and the PNGs.

---

## Settings

Open **File > Settings** to configure:

- **Name** — label shown in the window title and tray tooltip for this instance
- **WebUI URL** — change if Resilio is bound to a non-default address or port
- **Start minimized to tray** — launch without showing the main window
- **Launch on login** — writes `~/.config/autostart/synclens.desktop` so your desktop environment starts SyncLens automatically; uncheck to remove it
- **Desktop notifications** — toggle wrapper notifications on or off

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
