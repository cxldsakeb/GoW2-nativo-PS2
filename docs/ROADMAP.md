# Roadmap

## What to expect

- **We are not starting from scratch.** The runtime already knows how to draw with the GS, run the VU1 and the IOP as native code, play sound, read the controller and the memory card and play videos: that is what took weeks in GoW1. GoW II uses an evolution of the same engine, so much of it should work as is.
- **What is new:** GoW II's ELF has no symbols and its addresses are different. We have to build the function map, the recompiler configuration (Sony library stubs and the `[mmio]` table) and the overrides, and fix the game's own blockers with the usual method: run, fix the **first** blocker and repeat.
- `referencia/gow1/` is the model: which stubs were needed, which register accesses become native calls and which overrides GoW1 needed. For each thing we have to find **its equivalent in GoW II**.

## Phases

Each phase has a verifiable "done" criterion.

### Phase 0: environment (everyone) — Ale still pending

- [ ] Everyone follows [SETUP.md](SETUP.md) up to `build-tools.ps1`. (cxldsakeb and DaniSBR1: done)
- [ ] Everyone extracts their copy with `extract-iso.ps1` and we all have the **same ELF SHA1**, recorded in [NOTES.md](NOTES.md). (cxldsakeb and DaniSBR1: done, `E6A62A0F…`)

**Done when:** all 3 of us build the tools and have the reference ELF.

### Phase 1: function map and first configuration — ✅ done (2026-10-15)

- [x] Ghidra (`ghidra-analyze.ps1`) → `config/gow_functions.csv` and `config/gow.toml`. (DaniSBR1: 5,874 functions)
- [x] Functions that Ghidra doesn't see (`scan_missing_functions.py`). (DaniSBR1: 733; since phase 2 they go in `funcmap.csv`)
- [x] `config/funcmap.csv` (`Name,Start,End,Size`) and `config/recomp.template.toml` with `@ELF@`, `@MAP@` and `@OUT@`, modeled on the ones in `referencia/gow1/config/`. (DaniSBR1: first version; `recompile.ps1` generates C++ without errors)
- [x] Sony library stubs (`sceMpeg*`, `sceIpu*`, `sceGs*`, `sceCd*`, `sceDma*`, libkernel…): locate each one in GoW II (strings and call patterns help; DeepSeek can cross-reference lists). (DaniSBR1: 76 of 79; the other 3 are not linked in GoW II. SDK 3.0.0, see NOTES.md)
- [x] `[mmio]` table: the game's hardware register accesses that the runtime turns into native calls. (DaniSBR1: 240 entries from `ps2_analyzer`, equivalent to GoW1's table; cxldsakeb confirmed that the runner builds and links with it)

**Done when:** `recompile.ps1` generates C++ without errors and the runner builds. **Done** (2026-10-15): with stubs and `[mmio]`, `recompile.ps1` runs without errors and the `RelWithDebInfo` runner links with no unresolved symbols (cxldsakeb).

### Phase 2: first boot — ✅ done (2026-10-16)

**Method** (the one from GoW1): `.\scripts\run.ps1 -Config RelWithDebInfo -NoBuild`, read the **first** error in the log in `out/logs/`, fix it and repeat. Each blocker goes into the "Blockers" table in [NOTES.md](NOTES.md). GoW II's overrides go in `overrides/gow2_overrides.cpp` and are registered with their own `PS2_REGISTER_GAME_OVERRIDE` (`"SCUS_974.81"`, entry `0x00100008`): GoW1's are not activated here, because they are tied to `SCUS_973.99`.

- [x] **Everyone builds their own runner** (`recompile.ps1` + `run.ps1`, ~40–50 min the first time). The `.exe` and `generated/` are not uploaded (rule 1), so they cannot be shared. (cxldsakeb and DaniSBR1)
- [x] **Boots and loads the 8 IRX** in the IOP interpreter (`SIO2MAN`, `DBCMAN`, `SIO2D`, `MC2_D`, `DS2U_D`, `LIBSD`, `989NOMID`, `SMPD`) and connects to their servers. Blockers 1–2: `sceCdReadDvdDualInfo` and the **TEMPORAL** at `0x1875E0` (the runtime passes `argc = 0` to `main`). (DaniSBR1)
- [x] **Gets through initialization and reads the disc.** Blockers 3–5 (DaniSBR1): the runtime's heap was overwriting the game's, the resume points of the scanner's functions (now in `funcmap.csv`) and the IOP module that is embedded in the ELF. Blockers 6–9 (DaniSBR1 and cxldsakeb, in the fork): `ioman` to read `GODOFWAR.TOC`, SMPD's `strtok`/`GetSystemTimeLow`, the EE `Count` register and the native IOP replacements for GoW II's LIBSD.
- [x] **Essential overrides**, with GoW II addresses: `sceIpuInit` (blocker 11) and `libpad2`, the controller (blocker 10; without a controller, the keyboard). `DelayThread` and timer 2 were not needed.
- Solved without an override: `sceSifInitRpc` and `iWakeupThread`. In GoW1 the recompiler discarded them; in GoW II Ghidra detects them.

**Done when:** it reaches the first screen (logos). **Done:** full title screen.

### Phase 3: menu and videos — ✅ done (2026-10-16)

- [x] The menu responds (with the controller, the keyboard or `GOW2_PAD_SCRIPT`).
- [x] When starting a new game, the first video plays (blocker 11, `sceIpuInit`). (cxldsakeb)

**Done when:** you reach the menu and the intro plays.

### Phase 4: playable — in progress

The **first playable area (Rhodes) is reached, with sound** (cxldsakeb, 2026-10-16). What's left:

- [x] **Blocker 12, the green scenery.** The game counts the characters' shadows in the framebuffer's green channel and restores it with 8 px column copies through a 16-bit view. Those copies end at a fraction of a pixel (`y = 895.4375`) and the GS backend discarded them. Fixed in the fork (`8a4abab`, DaniSBR1, 2026-10-16): the scenery looks right and **Kratos's shadow is drawn**.
- [x] **Blocker 13, the blue polygon in the sky:** the Z16 → framebuffer copy read the wrong depth target. Fixed in the fork (`d0b6162`, DaniSBR1, 2026-10-16).
- [x] **Upscaling glitches** (blockers 19–21 and 23, DaniSBR1, 2026-10-19 and 21): the "ghost" to Kratos's left, the bright band on the right edge, the blue line that follows Kratos and the thin lines every 32 PS2 pixels at scale 3. Also done: the bright blue line from the bloom and the distant geometry (blockers 14 and 15) and the flicker after the Blade of Olympus (blocker 22).
- [x] **Bubbles, steam and wavy lines** (blockers 24-26, DaniSBR1, 2026-10-22): the wavy lines under water and in the underworld, the bubbles stacked in white columns and the steam in one white blob in the bath room (the stubbed `rand`), and the bubbles drawn as square fragments.
- [ ] Other graphics glitches: **the filtering at internal scales x2 and x4** (DaniSBR1), the faint edge of the distant horizon, the flicker in the second cutscene, the moving pixelated filter and the videos stretched to 16:9. Details in NOTES.md, "Repository status".
- [ ] **Full speed.** The F10 counter is useless (it always shows 60). Measured properly with `GOW_PERF_DIAG=1` and `GOW_PERF_FRAME_PC=0x14E380` on cxldsakeb's PC, with native VU1 and IOP (section 7b of [SETUP.md](SETUP.md)): **60 FPS in the menu and ~44–59 in the first area with combat**. The limit is now the GS thread (and the VU1), not the EE. Details in NOTES.md, "Repository status".
- [x] **Window controls** (DaniSBR1, 2026-10-16, fork `cf4e3f2`/`3b71410`): F12 borderless fullscreen / windowed; F10 FPS counter like PCSX2's.
- [x] **Launcher** (DaniSBR1, 2026-10-20, `launcher/`): a window to adjust settings and start the game with PLAY without recompiling. Every Graphics setting reaches the game (2026-10-22): resolution, 16:9 / 4:3, fullscreen at startup, FXAA, anisotropic filtering, sharpening, vsync and the FPS counter.
- [ ] Gameplay, combat and HUD, checked on screen.
- [x] Saving and loading (memory card): blocker 16 (cxldsakeb, 2026-10-17). Formats, saves and loads.
- [x] **Cutscenes** (blocker 17, cxldsakeb, 2026-10-17): the movies' ADPCM audio didn't reach the game and the player gave up on them; they also ran at ~0.57×. The opening one plays in full; the one after the QTE against the Colossus is still to be confirmed by playing.
- [x] Remove the TEMPORAL at `0x1875E0`: `SetupThread` fills in the arguments in the runtime (fork `5b1a25a`, cxldsakeb, 2026-10-17).

**Done when:** the first level is played at full speed.
