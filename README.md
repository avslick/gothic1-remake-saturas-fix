# Gothic 1 Remake — Saturas Guard Softlock Fix (Chapter 3)

> Resets the `GuardPassageWaterMagesWarning_NC` flag in your save so the Water Mage
> guards give you dialogue again instead of turning hostile — fixing the Chapter 3
> softlock where you can't reach Saturas.

<!-- Optional badges — fill in once the repo exists:
![Platform](https://img.shields.io/badge/platform-Windows-blue)
![License](https://img.shields.io/badge/license-GPLv3-green)
-->

## The bug

In Chapter 3 the main quest sends you to meet **Saturas** in the New Camp. If you
ever **walked up to the Water Mage guards earlier in the game** (e.g. in Chapter 1)
and got their "one more step and you're dead" warning, a flag stays stuck in your
save. When you return during the quest, the dialogue that's supposed to let you in
flickers for a split second and never fires — instead the guards **and** the Water
Mages turn fully hostile, and the main story is softlocked.

This is a known, widespread issue. The cause is that the guard-warning flag
(`GuardPassageWaterMagesWarning_NC`) is left at `2`, which the game treats as
"already warned / trespassing" and blocks the quest dialogue.

## What this tool does

It opens your save, finds `GuardPassageWaterMagesWarning_NC`, and resets it from
`2` to `0`. After that the guards give you the normal "let me in" dialogue and you
can reach Saturas. **Your original file is never modified** — the tool writes a new
`*-fixed.sav` next to it.

## Requirements

- **Windows**
- **PC (Steam) version** of the game.

## Usage

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

- The tool **verifies its own result**. If the save format is unexpected, it
  refuses and changes nothing.
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

## Building from source

The patcher is a single C++ file. Oodle decompression is handled by the
open-source **[ooz](https://github.com/powzix/ooz)** project (a clean-room Oodle
decoder), whose sources are included/vendored in this repo.

**MSVC** (open a *Developer Command Prompt for VS*):
```bat
cl /O2 /EHsc /std:c++17 source-patcher.cpp ooz\*.cpp /Fe:G1R-SaturasFix.exe
```

**MinGW-w64 / g++:**
```bash
g++ -O2 -std=c++17 source-patcher.cpp ooz/*.cpp -o G1R-SaturasFix.exe
```

> Adjust the `ooz\*.cpp` / `ooz/*.cpp` glob to match how the ooz sources are laid
> out in your tree.

## How it works (technical)

- Save files use a custom **`GSAV`** container; the gameplay payload inside is
  **Oodle-compressed**, which is why the flag isn't visible in the raw bytes.
- The tool decompresses the payload with **ooz**, locates the
  `GuardPassageWaterMagesWarning_NC` property, and rewrites its value `2 → 0`.
- It then writes the result as a new `.sav`. A length/format sanity check runs
  before saving; on any mismatch it aborts without writing.

## Credits

- Oodle decompression: the **ooz** project by Powzix (clean-room, open source).
  Oodle itself is © RAD Game Tools / Epic Games; this repo does not include Oodle.

## Disclaimer

Unofficial tool, **not affiliated with THQ Nordic or Alkimia Interactive**.
Single-player only. **Always back up your save first.** Use at your own risk.

## License

This project is released under the **GNU General Public License v3.0** (see
`LICENSE`). GPL is required because the tool compiles in the **ooz** Oodle
decompressor, which is GPL-licensed — so the combined binary is a derivative work
and must be distributed under the GPL, with full source available (it is, in this
repo). Keep the ooz source files and their license headers intact.
