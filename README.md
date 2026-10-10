# Stapik Planner

A desktop weekly activity planner for Linux, written in C++20 using GTK4/gtkmm. Styled after the retro old-school aesthetic, matching [Stapik Calendar](https://github.com/Stapik-Group/stapik-calendar).

![Screenshot](screenshots/screenshot_1.png)

## Features

- **Weekly schedule grid** - seven day columns (Monday–Sunday), each with a configurable number of activity slots
- **Activity catalog** - add, edit and delete reusable activities, each with a name and a difficulty level (Light 10%, Normal 20%, Medium 30%, Hard 50%)
- **Daily load tracking** - a load bar per day shows total workload as a percentage; once a day reaches 100%, its remaining empty slots are locked to prevent overloading
- **Cloud sync** - save and load data via [Stapik Cloud](https://github.com/Stapik-Group/stapik-cloud), with optimistic-concurrency conflict resolution and full version history
- **Multilingual UI** - Polish, English and German interface with instant switching
- **Five themes** - Classic, Classic Pink, Modern, Dark and Neoclassic, switchable live from the menu; the themes and the common parts of the look are shared with the other Stapik applications
- **Auto-save** - schedule and catalog data saved locally after every change
- **Undo / redo** - every change to the plan, the activities and the slot count can be undone and redone from the **Edit** menu (Ctrl+Z, Ctrl+Shift+Z), including reducing the slot count, which brings the dropped slots back. The history lasts for the session and is cleared when a newer version from the cloud replaces the plan
- **Retro aesthetic** — classic look with raised buttons, blue header bar and grey cells by default, consistent with Stapik Calendar

## Dependencies

- `gtkmm-4.0`
- `libcurl`
- [`stapik-common`](https://github.com/Stapik-Group/stapik-common) v1.3.2+ (fetched automatically via CMake FetchContent)
- `nlohmann/json` (fetched automatically via CMake FetchContent, transitively provided by `stapik-common`)

On Ubuntu/Debian:
```bash
sudo apt install libgtkmm-4.0-dev libcurl4-openssl-dev
```

Building a `.deb` package additionally requires `dpkg-dev` (used to auto-detect runtime dependencies):
```bash
sudo apt install dpkg-dev
```

## Building

```bash
git clone https://github.com/Stapik-Group/stapik-planner
cd stapik-planner
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

To build and run the unit tests as well:

```bash
cmake -B cmake-build-tests -DCMAKE_BUILD_TYPE=Debug -DSTAPIK_PLANNER_BUILD_TESTS=ON
cmake --build cmake-build-tests
ctest --test-dir cmake-build-tests --output-on-failure
```

The `resources/` folder is copied next to the executable in the build directory; files that were removed from the sources stay there until the build directory is cleaned.

## Installation

### Option 1 — Download prebuilt `.deb` (recommended)

Download the latest `.deb` package from the [Releases page](https://github.com/Stapik-Group/stapik-planner/releases), then install it:

```bash
sudo dpkg -i stapikplanner_*.deb
sudo apt install -f   # resolves any missing runtime dependencies
```

### Option 2 — build `.deb` from source

```bash
cd cmake-build-release
cpack -G DEB
sudo dpkg -i stapikplanner_*.deb
sudo apt install -f
```

Either option installs the app to `/opt/stapikplanner/`, with a launcher at `/usr/bin/stapikplanner`, and it appears in the desktop environment's application menu. Upgrading from a version that installed to `/usr/lib/stapikplanner/` removes the old files.

### Option 3 — per-user install (no sudo required)

The launcher contains the install location, so both prefixes are given when configuring:

```bash
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$HOME/.local/opt/stapikplanner" \
      -DSTAPIK_APP_SYSTEM_PREFIX="$HOME/.local"
cmake --build cmake-build-release
cmake --install cmake-build-release
```

Installs the app to `~/.local/opt/stapikplanner/`, with a launcher at `~/.local/bin/stapikplanner`. Make sure `~/.local/bin` is in your `PATH`.

## Uninstalling

### If installed via `.deb`

```bash
sudo dpkg -r stapikplanner
```

### If installed per-user

```bash
rm -rf ~/.local/opt/stapikplanner
rm ~/.local/bin/stapikplanner
rm ~/.local/share/applications/pl.stapik.planner.desktop
rm ~/.local/share/icons/hicolor/256x256/apps/stapikplanner.png
```

Either way, your schedule data and cloud config remain at `~/.local/share/stapikplanner/`, and your language and theme preferences at `~/.config/stapikplanner/` — see [Data Storage](#data-storage) below if you want to remove those too.

## Cloud Sync

The app supports synchronization via [Stapik Cloud](https://github.com/Stapik-Group/stapik-cloud) — the same server and protocol used by Stapik Calendar. Go to **File → Connect**, enter the server URL and API key. If a connection is already configured, the app reconnects automatically on startup.

Once connected, the app compares the local file and the cloud copy using a `lastUpdate` timestamp and keeps whichever one is newer, overwriting the other **as a whole document**. There is no field-level or entry-level merging — if both copies changed since the last sync, the older one is fully replaced.

Data is saved locally after every change. The upload to the cloud runs in the background a moment after the change, so the window never waits for the network, and a status line at the bottom of the window shows whether the planner is up to date, syncing, offline or in conflict. Writes are checked for conflicts: if another device saved a newer version in the meantime, the write is rejected, the server's copy is fetched, and the app retries once against that copy before falling back to accepting the server's version; in that case a message tells you that your latest changes were replaced. If the cloud is unreachable, the change stays saved locally and the app keeps retrying in the background with growing pauses; you can also trigger a sync manually from **File → Sync**. When you close the window, pending changes are pushed first, waiting a few seconds at most.

**Caution for multi-device use:** since conflict resolution is whole-document, editing the schedule offline on two different machines before either one reconnects can still cause one set of changes to be discarded. Stapik Cloud keeps a version history of every write, so a discarded document isn't gone permanently, but the app itself doesn't yet expose a way to browse or restore old versions. If you use the app on more than one device, make sure to sync (or at least go online) after each editing session to avoid overwriting your own changes.

The app talks to the Stapik Cloud `/documents/{slotKey}` endpoint (slot key `planner.json`), authenticated via an `x-api-key` header. See the [Stapik Cloud API reference](https://github.com/Stapik-Group/stapik-cloud) for details.

A connection saved by a much older version that used an incompatible cloud protocol is no longer cleared automatically. If the status line shows an error after upgrading from such a version, reconnect once via **File → Connect**.

## Themes

Switch between five themes from **Settings → Theme**: Classic (the original retro look), Classic Pink, Modern (flat, minimal), Dark and Neoclassic. The choice applies instantly and is remembered between launches. The themes come from stapik-common; the planner adds its own widgets on top of them in `resources/themes/<theme>.css`, one file per theme.

## Slots per Day

The number of activity slots per day (3–6) can be changed from **Settings → Slots**. Increasing it adds empty slots to every day; decreasing it truncates each day's plan to the new limit, discarding any activities scheduled beyond it — the app warns you when this happens.

## Data Storage

Schedule, catalog and settings are stored locally at `~/.local/share/stapikplanner/planner.json` (`$XDG_DATA_HOME/stapikplanner/planner.json` when `XDG_DATA_HOME` is set). The file is versioned with a `schemaVersion` and carries the `lastUpdate` timestamp used for cloud sync; files written by earlier versions are migrated when they are loaded. Every save keeps the previous file as `planner.json.bak`. A file that cannot be read is never silently overwritten: an unreadable one is moved aside as `planner.json.corrupt-<time>`, and one written by a newer version of the app as `planner.json.newer-version-<time>`. Cloud config at `~/.local/share/stapikplanner/config.json`. Language and theme preferences at `~/.config/stapikplanner/settings.json` (`$XDG_CONFIG_HOME/stapikplanner/settings.json`); the `locale.txt` and `theme.txt` files of earlier versions are imported on the first launch.

## Themes

![Screenshot](screenshots/screenshot_2.png)
![Screenshot](screenshots/screenshot_3.png)

## TODO

- [x] Weekly schedule grid with configurable slots per day
- [x] Activity catalog (add / edit / delete)
- [x] Daily load tracking with 100% cap
- [x] Cloud sync with conflict resolution
- [x] Multilingual UI (PL/EN/DE)
- [x] `.deb` package for easier distribution
- [x] In-app UI for changing the number of slots per day
- [x] Multiple themes (Classic / Classic Pink / Modern / Dark / Neoclassic)
- [x] Undo / redo
- [ ] Copy a day's plan to another day
- [ ] Weekly load summary / overview
- [ ] Flatpak package