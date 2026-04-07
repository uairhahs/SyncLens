# SyncLens

> An `Unofficial` Qt desktop wrapper for the Resilio Sync WebUI on Linux

## Overview

**SyncLens** is a desktop Linux application that provides a native-feeling Qt wrapper around the locally hosted Resilio Sync WebUI rather than reimplementing the sync engine or WebUI itself. Resilio Sync on Linux is documented as a daemon-oriented application managed through a browser-based WebUI, and the WebUI is configurable for local access, which makes a wrapper architecture a practical fit.

The project goal is to improve Linux desktop integration while keeping implementation risk, maintenance cost, and legal exposure lower than a full alternative client. The wrapper should present the existing local interface inside a Qt shell and add native operating-system features such as tray integration, window lifecycle handling, service detection, and quick actions.

## Product Goals

The product should solve the gap between “daemon plus browser tab” and a conventional Linux desktop application. Resilio’s Linux documentation and ArchWiki guidance indicate that Linux users commonly interact with Sync through the WebUI rather than a native desktop interface, which makes desktop integration the main value proposition.

Primary goals:

- Embed the local Resilio Sync WebUI in a Qt application window.
- Provide tray-based access and lifecycle controls expected from a Linux desktop sync app.
- Detect whether the local daemon and WebUI are reachable before presenting the app as healthy.
- Preserve compatibility by avoiding DOM rewriting or replacement of core Resilio functionality, which reduces breakage when upstream WebUI changes.

## Non-Goals

This project should not be positioned as an alternative sync engine, reverse-engineered client, or replacement administration UI. The Linux client already exposes a working WebUI, so the wrapper should remain a shell around that interface rather than fork functionality already delivered by Resilio.

Out of scope for the initial release:

- Reimplementing folder management, transfer logic, or peer discovery.
- Heavily restyling or injecting scripts into the WebUI DOM.
- Remote multi-host fleet management.
- Cross-platform parity beyond Linux.
- Bundling or redistributing Resilio Sync itself unless licensing terms are reviewed separately.

## Legal and Branding Constraints

Resilio’s public terms state that use of Resilio trademarks, logos, and confusingly similar marks requires permission, and the API terms also prohibit implying sponsorship or endorsement by Resilio. Because of that, the wrapper should use **SyncLens** as its product name and use “Resilio Sync” only in descriptive compatibility text, such as “Unofficial Qt wrapper for the Resilio Sync WebUI.”

Required constraints:

- Do not use Resilio logos, icons, or official product artwork.
- Do not market the app as official, endorsed, or affiliated.
- Keep an explicit disclaimer in the About dialog, README, and project site.
- Avoid naming that makes “Resilio” the primary product brand.

## Target Users

The primary audience is Linux desktop users who already run Resilio Sync locally and want a better-integrated application experience than a browser tab. This includes users on GNOME, KDE Plasma, and other desktop environments where tray access, autostart behavior, and a dedicated app window are expected workflow features.

Secondary users include self-hosters and workstation users who operate the local daemon as a system or user service and want quick visibility into whether the WebUI is available. ArchWiki documents both service-based and user-oriented installation flows, which supports this user profile.

## Functional Requirements

### Core Window

The main window should host the local Resilio Sync WebUI using Qt WebEngine and default to the local WebUI endpoint. Resilio documents WebUI configuration for local access, and Linux guidance references browser-based access rather than a native interface.

Requirements:

- Open the local WebUI in an embedded `QWebEngineView`.
- Default target URL: configured local WebUI endpoint, commonly `http://127.0.0.1:8888/gui/` unless changed by the user.
- Provide browser fallback if embedded rendering fails.
- Preserve cookies/session only within the app runtime unless persistent profile behavior is intentionally configured.

### Service and Health Detection

The wrapper should determine whether the daemon is installed, whether the process is running, and whether the WebUI is reachable. ArchWiki and Linux install docs describe service-based installation and daemon operation on Linux, so the health model should reflect that deployment style.

Requirements:

- Detect configured WebUI URL reachability.
- Differentiate between “daemon not installed,” “daemon installed but stopped,” and “daemon running but WebUI unavailable.”
- Offer user-friendly remediation actions, such as open config, retry, or launch browser.
- Optionally probe systemd user and system services where available.

### Tray Integration

Tray support is a primary feature because it is one of the clearest advantages of a native wrapper over a browser tab. The app should remain easily accessible without occupying taskbar or workspace attention all day.

Requirements:

- System tray icon with menu.
- Menu actions: Open SyncLens, Open in browser, Refresh status, Start service, Stop service, Restart service, Quit.
- Optional single-click or double-click behavior configurable later.
- Clear status states: healthy, connecting, stopped, error.

### Launch and Session Behavior

The wrapper should behave like a normal desktop sync app and support “start hidden” and autostart options. Linux users commonly expect sync tools to live in the tray and surface only when needed.

Requirements:

- Launch visible or minimized-to-tray.
- Optional autostart entry using XDG autostart on supported desktops.
- Restore last window geometry when practical.
- Do not rely on `localStorage`-style web assumptions for native app state; store app preferences using Qt mechanisms.

### Notifications

Native notifications should be limited to wrapper-relevant events rather than duplicate all events already handled by the WebUI.

Requirements:

- Notify on daemon unavailable at launch.
- Notify when connection to the local WebUI is restored after failure.
- Notify on start/stop/restart command results when those actions are initiated by the wrapper.
- Allow notifications to be disabled.

## Technical Specification

### Stack

Recommended stack:

- **Language:** C++ with Qt 6.
- **Framework:** Qt 6.4 LTS (minimum).
- **Build system:** CMake.
- **Web embedding:** Qt WebEngine.
- **Desktop features:** `QSystemTrayIcon`, `QProcess`, `QNetworkAccessManager`, and optional DBus/systemd integration.

A Qt 6 stack fits the goal of a well-integrated Linux desktop shell while keeping the architecture straightforward around a locally hosted web application.

### Architecture

Suggested internal modules:

| Module | Responsibility |
| --- | --- |
| App Shell | App lifecycle, single-instance behavior, settings, window creation |
| WebView Host | Embedded WebUI, navigation control, fallback handling |
| Health Monitor | Poll local endpoint, determine availability and error states |
| Service Controller | Interact with systemd or local process commands where supported |
| Tray Controller | Tray icon, menus, quick actions, status presentation |
| Settings Manager | URL, startup mode, browser fallback, notification preferences |
| About/Legal UI | Disclaimer, license notices, project metadata |

### Health Model

The wrapper should expose a simple finite state model:

- **Unknown**: startup, no checks completed.
- **Running**: daemon reachable and WebUI responding.
- **Starting**: start requested, waiting for health confirmation.
- **Stopped**: daemon absent or not running.
- **Unreachable**: process may exist but WebUI endpoint is not responding.
- **Error**: wrapper-specific failure, such as misconfiguration or permissions issue.

This model should drive tray icon color or badge, menu text, and notification behavior.

### Configuration

Initial settings should include:

- WebUI URL.
- Start minimized to tray.
- Launch on login.
- Open external browser instead of embedded view.
- Enable service control actions.
- Notification preferences.
- Health check interval (default: 10 seconds).

### Service Control Strategy

Service control should be optional because installation methods vary. Linux documentation and ArchWiki coverage show multiple deployment approaches, so commands cannot be hardcoded without detection logic.

Preferred order:

1. Detect user service.
2. Detect system service.
3. Fallback to custom executable path if configured.
4. If none found, disable service controls and keep read-only monitoring.

## User Experience Requirements

The product should feel like a lightweight desktop companion rather than a second full application layer on top of Resilio Sync. The native shell should be visually quiet and should not compete with the WebUI’s own interaction patterns.

UX requirements:

- Fast startup to tray.
- Minimal chrome around the embedded WebUI.
- Obvious offline/error state before showing a blank page.
- One-click access to browser fallback.
- Clear About dialog with unofficial disclaimer.
- No aggressive theming of embedded content in v1.

## Security and Privacy

The wrapper should assume the local WebUI may expose sensitive sync metadata and should minimize additional data handling. Resilio’s API terms also impose obligations around privacy disclosures if an app collects user information.

Requirements:

- No telemetry by default.
- No external network dependency for core functionality beyond the local WebUI.
- Do not proxy Resilio credentials through third-party services.
- If logs are collected for debugging, keep them local and user-accessible.
- Publish a privacy statement if any diagnostics or crash reports are added later.

## Packaging and Distribution

The first release should target Linux desktop users with packaging that does not bundle Resilio Sync itself. That keeps licensing and maintenance simpler while assuming users install Resilio Sync separately through their preferred method.

Recommended distribution targets:

- AppImage for broad compatibility.
- Native packages later for Arch, Debian/Ubuntu, and Fedora.
- Optional AUR package if the project becomes public and maintained.

## MVP Definition

A v1 MVP should include only the features needed to make the wrapper genuinely useful without creating long-term maintenance burden.

MVP checklist:

- Qt main window with embedded WebUI.
- Configurable local URL.
- Tray icon with Open, Refresh, Open in browser, Quit.
- Health check and status indicator.
- Optional start minimized.
- About dialog with unofficial disclaimer.
- Basic settings dialog.

## Post-MVP Candidates

Possible later enhancements:

- Start/stop/restart service controls.
- Desktop notifications for wrapper-managed events.
- Single-instance enforcement.
- Deep links to common Resilio pages.
- Optional custom titlebar and keyboard shortcuts.
- Better environment-specific integration for GNOME and KDE.
- Read-only dashboard widget for daemon status before loading the WebUI.

## Acceptance Criteria

The project can be considered successful when the following are true:

- A Linux user can install SyncLens and point it at a local Resilio Sync WebUI.
- The app can present the WebUI in a stable embedded window for normal daily use.
- The tray provides quicker access than using a browser tab.
- The app clearly distinguishes itself as unofficial and avoids official branding.
- The wrapper remains functional even if service controls are unavailable on a given installation method.

## Recommended Positioning Text

Suggested short description:

> SyncLens is an unofficial Qt desktop wrapper for the Resilio Sync WebUI on Linux.

Suggested longer description:

> SyncLens provides a native-feeling Linux desktop shell for locally hosted Resilio Sync, adding tray access, health checks, and desktop integration without replacing the underlying daemon or WebUI.

## Implementation Decisions

These decisions should be resolved before implementation begins:

- Language: C++ with Qt 6.4 LTS minimum.
- Build system: CMake.
- Embedded-only mode for v1; no in-app browser fallback. "Open in browser" tray/menu action invokes `xdg-open` with the configured WebUI URL.
- Default health check poll interval: 10 seconds.
- Service control ships in v1.1.
- Packaging target for the first public release is AUR; other Linux package formats to follow if the project is maintained.
- Session persistence is handled by web cookies and local storage within the embedded WebUI, not by the wrapper itself.
