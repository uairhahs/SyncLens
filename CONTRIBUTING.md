# Contributing

## Branches and pull requests

`main` is protected. Every change reaches it through a pull request:

- Work on a branch named by the kind of change: `feat/…`, `fix/…`, `chore/…`, `docs/…`.
- A pull request needs one approving review and a passing **Build gate** check.
- Merge with a merge commit or a rebase merge. Squash merges are off, so commit messages are kept as written.
- `main` cannot be force-pushed or deleted.

Commit subjects follow the existing style: `feat:`, `fix:`, `style:`, `docs:`, `chore:`, `packaging:` and `release:`.

## Checks

`.github/workflows/checks.yml` runs on every pull request and on pushes to `main`:

| Job                                            | What it checks                                                                                                                                        |
| ---------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Lint                                           | `trunk check --ci` on the files the change touches (same linters as locally), and that version numbers agree                                          |
| Build (Arch, latest Qt) and package            | `makepkg` builds the Arch package from the checkout, then `namcap` reviews it                                                                         |
| Build (Debian 13, Qt 6.8) and per-user install | Builds the per-user tarball against Qt 6.8, the oldest supported, in a Debian 13 container, and installs, updates and uninstalls it with `install.sh` |
| Build (Nix flake)                              | `nix build .#synclens`                                                                                                                                |
| Build gate                                     | Passes only if every job above passed. This is the one required check on `main`                                                                       |

To run the same checks locally:

```bash
trunk check                 # lint changed files (add --all for everything)
scripts/check-version.sh    # version numbers agree
cmake -B build && cmake --build build
(cd packaging && makepkg -f) # build the Arch package without installing it
scripts/build-tarball.sh     # build the per-user tarball into ./dist
```

## Releasing

1. On a branch, bump the version in `CMakeLists.txt`, `packaging/PKGBUILD` (`pkgver`, and reset `pkgrel=1`), `nix/package.nix` and the `v…` references in `README.md`. `scripts/check-version.sh` confirms they agree.
2. Open a pull request titled `release: X.Y.Z` and merge it. That is the release: on the push to `main`, `.github/workflows/release.yml` sees that `vX.Y.Z` has no tag yet, creates the annotated tag on the merge commit and builds the release from it. Pushes to `main` whose version is already tagged release nothing.
3. Pushing a `vX.Y.Z` tag by hand also starts a release, if you need to tag a different commit.
4. The workflow builds the release assets from the tag and attaches them to the release, creating it with generated notes if it doesn't exist. Edit the notes on the release page or with `gh release edit vX.Y.Z`.

| Asset                                 | Built on                     | Used by                                           |
| ------------------------------------- | ---------------------------- | ------------------------------------------------- |
| `synclens-X.Y.Z-1-x86_64.pkg.tar.zst` | Arch container, `makepkg`    | `install.sh` on Arch, `pacman -U`                 |
| `synclens-X.Y.Z-linux-x86_64.tar.gz`  | Debian 13 container (Qt 6.8) | `install.sh --user`                               |
| `manifest.json`                       | `scripts/write-manifest.sh`  | `install.sh`: version, URL and SHA-256 per target |
| `*.sha256`                            |                              | Checking a manual download                        |

`scripts/install.sh` reads `manifest.json` from GitHub's latest-release download URL, so a release is available to it as soon as the workflow finishes. To rebuild a release's assets, run the Release workflow by hand with the tag; the build scripts come from the workflow's commit, so this also works for tags made before they existed.

The tarball's binary links against the system Qt, so it is built against Qt 6.8, the oldest version supported; Qt keeps binary compatibility with newer 6.x releases.

### Why Qt 6.8 is the minimum

The minimum Qt is the oldest one shipped by a maintained distribution, so the tarball and CI can be built and tested against a real system Qt rather than a separately downloaded one. That is Debian 13, with Qt 6.8, which is also a long-term-support Qt release. Qt 6.4, the previous minimum, is no longer supported by Qt, including its QtWebEngine, the embedded Chromium that renders the WebUI. No maintained distribution ships Qt 6.5 to 6.7. Users of Ubuntu 24.04 or Debian 12 can use the Nix package, which brings its own Qt. When Debian's next stable release becomes the oldest maintained distribution, raise the minimum to its Qt the same way: `CMakeLists.txt`, the container in both workflows, and the references in this file, `README.md` and `scripts/`.

The PKGBUILD builds the checkout it sits in rather than downloading a tarball, so there is no checksum to update after tagging.
