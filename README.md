# Gothic 1 Remake — Save Fix Tool

> Fixes save-game bugs in **Gothic 1 Remake** (PC/Steam) — including the Chapter 3
> softlock where the Water Mage guards turn hostile and you can't reach Saturas.

**[Download the latest release](https://github.com/avslick/gothic1-remake-saturas-fix/releases/latest)**

| Tool | What it is |
|---|---|
| **G1R-FixTool-GUI.exe** | Windows app (English/Russian): pick your save, tick the fixes you want, click **Apply**. Backup and self-verification are automatic. |
| **G1R-FixTool-Console.exe** | The same fixes in a console menu. |
| **G1R-SaturasFix.exe** | The original one-purpose tool: drag & drop your `.sav` onto it, get a fixed copy. Unchanged since v1.0 — all old instructions still apply. |
| **fixes.ini** | The list of fixes (ships next to the GUI/console exe). New fixes can be added here without recompiling. |

## The Saturas bug (Chapter 3)

In Chapter 3 the main quest sends you to meet **Saturas** in the New Camp. If you
ever **walked up to the Water Mage guards earlier in the game** (e.g. in Chapter 1)
and got their "one more step and you're dead" warning, a flag stays stuck in your
save. When you return during the quest, the dialogue that's supposed to let you in
flickers for a split second and never fires — instead the guards **and** the Water
Mages turn fully hostile, and the main story is softlocked.

The cause: the guard-warning flag (`GuardPassageWaterMagesWarning_NC`) is left at
`2`, which the game treats as "already warned / trespassing" and blocks the quest
dialogue. The fix resets it to `0` — the guards give you the normal "let me in"
dialogue again.

## Usage (GUI)

1. Close the game.
2. Run **G1R-FixTool-GUI.exe** (keep `fixes.ini` next to it).
3. Pick your save — the default folder is:
   ```
   C:\Users\<your name>\AppData\Local\G1R\Saved\SaveGames
   ```
4. Tick the fixes you want, click **Apply**. A backup of the original is created
   automatically, and the result is re-parsed and verified before it's written.
5. In Steam, temporarily **disable Steam Cloud** for the game
   (Properties → General) so the cloud doesn't restore the old file.
6. Launch the game and load the save.

## Usage (classic drag & drop — G1R-SaturasFix.exe)

1. Close the game.
2. Open your save folder:
   ```
   C:\Users\<your name>\AppData\Local\G1R\Saved\SaveGames
   ```
3. **Make a backup** of the save you want to fix (e.g. `G1R-013.sav`).
4. Drag and drop the `.sav` file onto **`G1R-SaturasFix.exe`**.
   A new `*-fixed.sav` appears next to it (original untouched).
5. Delete/move the original and rename `*-fixed.sav` back to the original name
   (e.g. `G1R-013.sav`).
6. In Steam, temporarily **disable Steam Cloud** for the game
   (Properties → General) so the cloud doesn't restore the old file.
7. Launch the game and load the save — the guards will give dialogue again.

## Notes

- Both tools **verify their own result**. If the save format is unexpected, they
  refuse and change nothing.
- The fixed file is **slightly larger** than the original — that's expected. The
  payload is written back without Oodle re-compression; the game re-compresses
  everything on your next in-game save.
- If the flag is already `0`, or not present in that save, the tool just tells you
  and exits.
- **Windows SmartScreen** may warn about an unknown publisher:
  "More info" → "Run anyway". The full source is included so you can build it
  yourself instead (see below).

## Troubleshooting

- **Nothing happens when you drag the save onto the `.exe`?**
  Right-click the `.sav` file → **"Open with"** → `G1R-SaturasFix` — that works too.
- **Still nothing?** Make sure you're **not** running the `.exe` as administrator.
  Windows blocks drag-and-drop from a normal Explorer window into an elevated app.
- **Terminal:** run it from a terminal with the save path as an argument:
  ```
  G1R-SaturasFix.exe "C:\Users\<you>\AppData\Local\G1R\Saved\SaveGames\G1R-013.sav"
  ```

## Linux (Steam Proton / Steam Deck)

Your saves live inside the game's Proton prefix:

```
~/.local/share/Steam/steamapps/compatdata/1297900/pfx/drive_c/users/steamuser/AppData/Local/G1R/Saved/SaveGames/
```

You don't need to compile anything — Steam's own Wine (Proton) runs the exe:

```bash
cp G1R-002.sav G1R-002.sav.bak   # back up first
WINEPREFIX="$HOME/.local/share/Steam/steamapps/compatdata/1297900/pfx" \
  "$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/files/bin/wine" \
  /path/to/G1R-SaturasFix.exe \
  "C:\users\steamuser\AppData\Local\G1R\Saved\SaveGames\G1R-002.sav"
```

Then rename `*-fixed.sav` back to the original name and disable Steam Cloud before
launching, as above.

- `Proton - Experimental` in the path depends on which Proton version the game
  uses (check Properties → Compatibility); adjust the folder name under
  `steamapps/common/` if you run a different one (e.g. a GE-Proton build).
- Flatpak Steam: the base path is `~/.var/app/com.valvesoftware.Steam/.local/share/Steam/`.
- Steam Deck: same steps, the prefix path is identical.

## Adding new fixes

Fixes are defined in `fixes.ini` — title/description (EN+RU), the save variable
name and the value to set. The GUI and console pick them up on start, no
recompilation needed:

```ini
[fix]
title    = Menu title (Russian)
title_en = Menu title (English)
desc     = Description (Russian)
desc_en  = Description (English)
var      = SaveVariableName
set      = 0
```

Found another save-flag bug? Open an issue with the variable name and the value
that breaks it.

## Building from source

**Visual Studio 2022:** open `G1R-FixTool.sln`, build Release | x64 — produces the
GUI and console tools.

**Linux → Windows cross-build (mingw-w64):** `./build-mingw.sh` builds all three
exes into `dist/` (needs `g++-mingw-w64-x86-64`).

**Just the classic drag & drop patcher** (single console exe):

MSVC (open a *Developer Command Prompt for VS*):
```bat
cl /O2 /EHsc /std:c++17 src\patcher.cpp src\kraken_lib.cpp src\bitknit.cpp src\lzna.cpp /Fe:G1R-SaturasFix.exe
```

MinGW-w64 / g++:
```bash
g++ -O2 -std=c++17 -msse4.1 -fpermissive -w -static src/patcher.cpp src/kraken_lib.cpp src/bitknit.cpp src/lzna.cpp -o G1R-SaturasFix.exe
```

CI: GitHub Actions builds everything on each push to `main`; pushing a `v*` tag
publishes a GitHub Release with the binaries.

## How it works (technical)

- Save files use a custom **`GSAV`** container; the gameplay payload inside is
  **Oodle-compressed**, which is why the flags aren't visible in the raw bytes.
- The tool decompresses the payload with **ooz**, locates the target property
  (e.g. `GuardPassageWaterMagesWarning_NC`), and rewrites its value (`2 → 0`).
- It then rebuilds the container and writes the result. A length/format sanity
  check (full re-parse of the rebuilt file) runs before saving; on any mismatch it
  aborts without writing.

## Code signing policy

Free code signing on Windows provided by [SignPath.io](https://signpath.io),
certificate by [SignPath Foundation](https://signpath.org).

- Committers and reviewers: [avslick](https://github.com/avslick) (project maintainer)
- Approvers: [avslick](https://github.com/avslick)

Releases are built automatically by GitHub Actions from the public source in
this repository and published on the [Releases](https://github.com/avslick/gothic1-remake-saturas-fix/releases)
page.

## Privacy policy

This program will not transfer any information to other networked systems
unless specifically requested by the user or the person installing or
operating it. G1R Fix Tool works completely offline: it only reads and writes
Gothic 1 Remake save files on the local machine and makes no network
connections of any kind.

## Credits

- Oodle Kraken decompression: the **[ooz](https://github.com/powzix/ooz)** project
  by Powzix (clean-room, open source). Oodle itself is © RAD Game Tools / Epic
  Games; this repo does not include Oodle.
- Save format reverse-engineered from real saves.

## Disclaimer

Unofficial tool, **not affiliated with THQ Nordic or Alkimia Interactive**.
Single-player only. **Always back up your save first.** Use at your own risk.

## License

This project is released under the **GNU General Public License v3.0** (see
`LICENSE`). GPL is required because the tools compile in the **ooz** Oodle
decompressor, which is GPL-licensed — so the combined binaries are derivative
works and must be distributed under the GPL, with full source available (it is,
in this repo). Keep the ooz source files and their license headers intact.
