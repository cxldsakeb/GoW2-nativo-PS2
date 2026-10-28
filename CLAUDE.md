# Context for Claude: native port of God of War II (PS2 → PC)

Claude Code loads this file automatically. It is the **shared thread** between the Claude sessions of the whole team: whatever is not here or in `docs/`, the others' Claude does not know.

## Project

- **Goal:** statically recompile God of War II (PS2, 2007) into a native Windows executable with [PS2Recomp](https://github.com/ran-j/PS2Recomp).
- **Comes from GoW1:** the repo [cxldsakeb/ROMS-nativas-PS2](https://github.com/cxldsakeb/ROMS-nativas-PS2) is already playable at ~60 FPS on top of **Waffle565's runtime** (native GS, VU1 and IOP compiled ahead of time, sound, controller, videos, memory card). GoW II uses an evolution of the same engine: we start from that runtime, and what has to be done here is **what is specific to the game** (function map, recompiler configuration, overrides) plus whatever runtime fixes it needs.
- **`PS2Recomp/` submodule:** our fork [DaniSBR1/PS2Recomp](https://github.com/DaniSBR1/PS2Recomp), **branch `godofwar2`** (branched off `godofwar` at `083a286`, Waffle565's base). It is a different branch from GoW1's so that changes for one game do not break the other. If a fix is valid for both, it is made in one and carried over to the other with `cherry-pick`.
- **Team:** @cxldsakeb (repo owner), @DaniSBR1 and @Alejrr2. They are learning: Claude acts as a guide, so explain the why of things and not just the what.
- **Language** (changed on 2026-10-19, when the repo went public): **documentation in English** (README, CONTRIBUTING, `docs/`, the `README.md` files, this file) **and new code comments in English** (existing Spanish comments are not translated just for the sake of it). **Conversation with the team and commit/PR messages stay in Spanish.** When you add to `docs/NOTES.md` or to "Current status" below, write in English even if the conversation is in Spanish.
- **Repo:** **public** since 2026-10-18 (GPLv3), as a notice of work in progress and so that others can collaborate or make their own fork. That is why rule 1 matters even more: everything that is pushed can be seen by anyone, including in the history.

## Rules that are never broken

1. **Nothing from the game is ever pushed:** no ISO, no ELF, no disc files, no C++ from `generated/`, no VU1 or IOP captures (`out/capturas/`), no images of the game. All of that is local and is in `.gitignore`. **Only exception:** the few example screenshots of the port in `docs/screenshots/`, shown in the README (agreed by cxldsakeb on 2026-10-19); don't add more, or other game images, without asking the team. Nor is it passed to external services (DeepSeek included).
2. **Addresses are only valid for the reference ELF**, identified by the SHA1 in `docs/NOTES.md`. **GoW1's are not valid here**: `referencia/gow1/` is for understanding what each thing does, not for copying addresses.
3. **Every finding goes to `docs/NOTES.md`:** identified functions, blockers and solutions. When you finish a task, update it in the same branch.
4. **Temporary patches** (`ret0`/`ret1`, hacks) carry a `TEMPORAL` comment in the code and a row in NOTES.md.
5. **Changes in `PS2Recomp/`:** first try to fix it with an override (`overrides/`). If the runtime has to change: commit on the `godofwar2` branch **inside** `PS2Recomp/`, push to `DaniSBR1/PS2Recomp`, and then commit the new submodule pointer here. **Never push diagnostic traces to the fork.**
6. **Licenses:** from Waffle565's repo we only use its `runtime` branch (GPL-3.0) and what KIexster redistributes from its `main`. Its `main` has no license (requested in [their issue #2](https://github.com/Waffle565/God-Of-War-Native-Port/issues/2)): you may **read and learn** from its documentation, but not copy its code or its text. Everything adapted cites its source.

## Structure and workflow

```
scripts/extract-iso.ps1 -IsoPath <iso>   ISO → game/disc/ with .disc_layout and IOP_MOD/ (prints the SHA1)
scripts/build-tools.ps1                  builds ps2_analyzer and ps2_recomp (Release, VS 2022)
scripts/ghidra-analyze.ps1 [-Reanalyze]  Ghidra headless (r5900) → config/gow.toml + config/gow_functions.csv
scripts/recompile.ps1 [-SyncOnly]        config/recomp.template.toml + config/funcmap.csv → generated/ → runner; reconfigures CMake
scripts/run.ps1 [-Config] [-NoBuild]     builds ps2EntryRunner and runs it; log in out/logs/
scripts/build-launcher.ps1 [-Run]        builds the launcher (launcher/) into out/launcher-build; -Run opens it
scripts/deepseek.ps1 -Tarea -Archivos    read-only task for DeepSeek (see below)
tools/                                   disc extractor, VU1 and IOP generators, disassemblers (tools/README.md)
referencia/gow1/                         GoW1 configuration and overrides (KIexster/Waffle565), only as a model
```

- **`config/` is empty at the start.** `recompile.ps1` expects `config/recomp.template.toml` (with `@ELF@`, `@MAP@` and `@OUT@`) and `config/funcmap.csv` (`Name,Start,End,Size`). Building them is phase 1 (see `docs/ROADMAP.md`): Ghidra provides the function map, and GoW1's template (`referencia/gow1/config/recomp.template.toml`) serves as the model for the stubs and the `[mmio]` table, **looking up the equivalent addresses in GoW II's ELF**.
- **Ghidra:** 12.1.4 + the ghidra-emotionengine-reloaded extension (`r5900:LE:32:default`) + JDK 21. The project lives in `ghidra/` (ignored). Ghidra does not see functions that are only called through a pointer (virtual methods): after each export, `py -3 tools/scan_missing_functions.py`.
- **Native IOP and VU1** (the big speed gain in GoW1, from 7–8 to ~60 FPS): they are generated from game data captured locally. See section 7b of `docs/SETUP.md`. To capture the VU1 the folder must exist; since fork `afd5c2c` it also captures with the native VU1 on (only the microprograms that are still missing), so new areas can be captured while playing at full speed.
- **When copying overrides by hand, update their timestamp** (`(Get-Item $destino).LastWriteTime = Get-Date`): otherwise MSBuild may not recompile them.
- **Unity build:** several overrides share a compilation unit. Use per-file prefixes (`kIop…`, `g_iop…`) and make sure `PS2_REGISTER_GAME_OVERRIDE` does not fall on the same line as in another override (use `__LINE__`).
- The scripts are in PowerShell 5.1 and are saved in UTF-8 **with BOM**.
- If VS Code has the C/C++ extension, exclude `generated/` and `PS2Recomp/ps2xRuntime/src/runner/` in `.vscode/settings.json` (`C_Cpp.files.exclude`).
- Debugging method (the same as GoW1): run, fix the **first** blocker in the log and repeat. Guide: PS2Recomp wiki, "PS2Recomp Stripped Game Walkthrough".

## How we collaborate

- One branch per task and a PR to `main`. **No review needed** (agreed on 2026-10-15): whoever opens it merges it.
- **PRs and issues from outside the team** (since the repo is public): someone from the team reviews them before merging, checking above all rule 1 (that nothing from the game gets in) and that the addresses are from the reference ELF. The rules for contributors are in `CONTRIBUTING.md`.
- Before starting: `git pull`, `git submodule sync --recursive` and `git submodule update --init --recursive`. Read `docs/NOTES.md` and `docs/ROADMAP.md`.
- Whoever is going to change the fork (`godofwar2`) gives notice beforehand.
- When finishing: update NOTES.md and, if the phase changes, the **Current status** section below.

## Tasks for DeepSeek (OpenCode)

Same as in GoW1: **DeepSeek V4.1 Flash, free on OpenCode**, for high-volume reading work. Claude: when a task fits, propose delegating it, launch it with `scripts/deepseek.ps1` and **always review the result**.

- Installation: `npm install -g opencode-ai`. Usage: `.\scripts\deepseek.ps1 -Tarea tarea.md -Archivos <archivos>`. It works in `%TEMP%\gow-deepseek\`, outside the repo, read-only.
- **Yes:** summarizing, classifying or cross-referencing public material (PS2Recomp, PCSX2, KIexster, Waffle565's documentation, our GoW1 repo).
- **No:** anything derived from the game (rule 1), nor deciding fixes.
- Tasks with an exact output format (tables) so they can be checked; large inputs, in ~70 KB chunks in parallel.

## Current status

> Update this section when closing each milestone, so that everyone's sessions see it.

### Phase 4: speed, 60 FPS in combat with the asynchronous mode (cxldsakeb, 2026-10-28)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`55cadbf`**), `recompile.ps1 -SyncOnly` with `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` set and build the runner (`ps2_memory.h` changed: most of the runtime library rebuilds). If you already had `afd5c2c` built: only `cmake --build` (one file).

- **Update, fork `55cadbf`:** the VIF1/VU1 thread lost ~35 % of its time in the Windows heap, copying every VIF1 `DIRECT` packet; now they go straight to the GS. First area, 10 min of combat: **59.7 FPS on average with the asynchronous mode (minimum 57)**, and **51–53 in the default mode** (was ~46).

- **Slow areas were VU1 microprograms never captured** (Typhon's Cavern: 16 → 35–55 FPS once captured). `GOW_VU1_CAPTURA` now works with the native VU1 on, so **capture while playing** whenever you reach a new area (SETUP.md, 7b).
- Fork: `glGetError` once per frame instead of after every batch (it was ~45 % of the GS thread), reused DMA buffers (the FPS used to drop from 59 to 51 over 10 min), fast VIF1 `UNPACK` and other small costs. Default mode, first area: ~42 → ~46.
- **Asynchronous mode, optional and off by default:** `GOW2_NO_FINISH_WAIT=1` + `GOW_GS_DISPLAY_ASINCRONO=1` lets the EE build the next frame while the VU1 draws the previous one: 50–60 FPS in combat (60 with `55cadbf`), stable over 10 min, no hangs (the hang it had, a lost FINISH, is fixed). Only automatic tests so far: **cxldsakeb, play with both on** and decide whether the launcher turns them on by default.
- **The VIF1/VU1 thread is still the busiest** (~86 %, now real work: native VU1 and `UNPACK`). New tool: `GOW2_HOST_PROFILE=5` (thread profiler). Details in NOTES.md, "Speed: the road to 60 FPS".

### Phase 4: bubbles, steam, wavy lines and every launcher setting (DaniSBR1, 2026-10-22)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`379fa48`**), **`recompile.ps1` with `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` set** (`config/recomp.template.toml` changed; on DaniSBR1's PC it became a full runner build, ~20 min) and `.\scripts\build-launcher.ps1`.

- **Blocker 25, the bubbles and the steam:** in the bath room the bubbles came out stacked in white columns and the steam as one white blob (the "excessive light"). The template stubbed the game's `rand` with the host's, which only goes up to `0x7FFF`; GoW II's goes up to `0x7FFFFFFF` and every particle got the same offset. `rand` is no longer stubbed. **GoW1 has the same stub** (`rand@0x0028C0C8`): cxldsakeb, worth checking there.
- **Blockers 24 and 26 (fork):** the wavy lines under water and in the underworld (sprite edges now on the pixels the GS covers, like PCSX2's `alignSprite`) and the bubbles drawn as square fragments at scale. **Checked by DaniSBR1 playing.**
- **Launcher:** every Graphics setting now reaches the game: 16:9 / 4:3, borderless fullscreen at startup, FXAA (PCSX2's), anisotropic filtering (new row, up to 16x), adjustable sharpening (0 = off) and vsync. The game no longer shows a pink screen at startup, and the Runtime Debugger starts hidden (F1 opens it).
- **New method:** a PCSX2 GS dump of the same room (Shift+F8) read with `tools/gsdump/gsdump_draws.py`, compared draw by draw with the port's local F8 log. Details in NOTES.md ("Repository status", "Blockers", "Launcher").
- **Next, for DaniSBR1:** the filtering at internal scales x2 and x4. Also open: speed to a steady 60, the videos stretched to 16:9, the second cutscene's flicker, the moving pixelated filter.

### Phase 4: the lines at scale 3 (blocker 23) and the launcher comes back (DaniSBR1, 2026-10-21)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`1d9f4b6`**), `cmake --build` (only `gs_hw_backend.cpp` changes) and, for the launcher, `.\scripts\build-launcher.ps1`.

- **Blocker 23:** at internal scale 3, thin bright vertical lines every 32 PS2 pixels in bands at the bottom and top of the screen, outdoors. A bug in blocker 21's fix: `fract(h / 3)` on the GPU (approximate division) made some host pixels read the texel on their left, and the `T8H` haze pass, which reads the target it writes, applied the haze twice at every strip seam. Fixed in the fork with whole-number arithmetic. **Checked by DaniSBR1 playing.** Details in NOTES.md ("Blockers").
- **Launcher:** it now hides while the game runs and **comes back when the game closes**, to try other settings without starting it again.
- **Debugging method that worked** (NOTES.md, "Repository status"): for scenes the pad script can't reach, local keys for whoever is playing there, one that logs the frame's draws and one that leaves out one pass or fix at a time. Never pushed to the fork.

### Launcher (DaniSBR1, 2026-10-20)

**After the `git pull`:** nothing to regenerate. To try it: `.\scripts\build-launcher.ps1 -Run` (builds `out\launcher-build`, a minute or two; it reuses the libraries the runner build already downloaded).

- **`launcher/`:** a borderless window in C++ with raylib + Dear ImGui (the runtime's own libraries), the approved design, English UI. **Save settings** writes `GoW2Launcher.ini` next to the executable; **PLAY** saves, finds the repo and the newest runner build and starts it like `run.ps1` (log in `out\logs\launcher-*.log`), hides while the game runs and comes back when it closes (to try other settings). Details and the `.ini` format in `launcher/README.md`.
- **What reaches the game:** internal resolution (`PS2X_GS_HW_SCALE`), FPS counter (`GOW_SHOW_FPS`) and sharpening on/off (`GOW_SCALER`). Aspect ratio, fullscreen at startup, FXAA and vsync say "coming soon": the runtime has no switch for them yet (next step, in the fork).
- **Remember after every pull:** the `.exe` does not update itself. A runner built before the latest fork or `config/` changes keeps the old bugs (it happened with the 16:9 and blocker 22): follow the "After the `git pull`" line of each section here.
- ~~Open: thin vertical lines every 32 PS2 pixels at internal scale 3~~ solved (blocker 23, see the section above).

### Phase 4: 16:9 (cxldsakeb, 2026-10-19, night)

**After the `git pull`: regenerate** (`recompile.ps1` with `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` set, ~1 min) and `cmake --build` (only 2 files change: the patched function and the override). Without regenerating, the image is 16:9 but crops vertically instead of widening the view.

- The game rendered 4:3 and the runtime stretched it to 16:9. Now it renders real 16:9 with **Hor+** (more view at the sides), using PCSX2's widescreen patch for GoW II: its data part in the controller override (every frame) and its one instruction as the **first recompiler patch** in `[patches]` of `config/recomp.template.toml`. Checked playing. Details in NOTES.md ("Image stretched to 16:9").
- **Next, for DaniSBR1:** the **videos** (title screen background, video cutscenes) are 4:3 and still come out stretched. Proposal in NOTES.md ("Videos still stretched"): 4:3 with black bars while the IPU is decoding, like PCSX2. Also pending: wavy lines under water and in the underworld, bubbles, pool lighting, the pixelated filter, and re-checking the greenish block (probably blocker 22).

### Phase 4: the flicker after the Blade of Olympus (blocker 22) (cxldsakeb, 2026-10-19, night)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`9499406`**) and `cmake --build` (only runtime library files change, a few minutes).

- **Blocker 22:** from the cutscene where Zeus sends the Blade of Olympus on, giant polygons flickered over whole frames (and sometimes the VIF1 lost its place). Cause: the runtime kept its RPC/TLS pools at fixed addresses `0x01F00000`–`0x01F30FFF`, **inside GoW II's heap**, and every SMPD call rewrote a fake server descriptor in the middle of the game's VU1 packets. Fixed in the fork: the pools now live in the private heap below the ELF. **Checked playing 20+ minutes** (bridge included). The full trail and the local diagnostics that found it are in NOTES.md ("Blockers").
- **For DaniSBR1:** the greenish block that follows Kratos in the room with the gate was probably this same bug; check it again. **New:** in the underworld, the same wavy lines as under water.
- From here, according to cxldsakeb, what's left is mostly **speed and graphics improvements**.

### Phase 4: upscaling fixes, the ghost, the right edge and the blue line (DaniSBR1, 2026-10-19)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`c012422`**) and `cmake --build` (only `gs_hw_backend.cpp` changes). If you come from before `6d3379b`, run **`build-tools.ps1` before `recompile.ps1`**: `recompile.ps1` does not rebuild `ps2_recomp.exe`, and with the old one the `movn`/`movz` fix is silently missing.

- **Blockers 19–21, all at internal scale > 1**, in how the GS backend reads a render target drawn at scale k: linear reads took the picture half a host pixel to the right (the bloom drifted left: **Kratos's ghost**); CLAMP mode clamped at the target's edge instead of `TW × TH` (the **bright band on the right edge**); point-sampled reads at texel centres took the next host texel (the depth haze one pixel off: **the blue line ahead of Kratos and the blue outlines on pillars**). Checked by DaniSBR1 playing. Details and the game's post-processing chain in NOTES.md ("Repository status", "Blockers").
- **Open for DaniSBR1:** underwater bubbles drawn as white pillars, far too much light in the pool room, flicker in the second cutscene, a moving pixelated filter, a greenish block that follows Kratos (NOTES.md, "Repository status").

### Screenshots in the README and the 16:9 stretch (cxldsakeb, 2026-10-19)

- **Exception to rule 1, agreed by cxldsakeb:** four example screenshots of the port (title screen, a cutscene and combat in the first area) in `docs/screenshots/`, shown in the README. It is the only exception: no other game images go into the repo.
- Taking them showed a real bug: **GoW II renders 4:3 and the runtime stretches it to 16:9** (everything ~33 % too wide). The screenshots are resized to 4:3. Cause and options in NOTES.md, "Repository status" ("Image stretched to 16:9").

### Documentation in English (cxldsakeb, 2026-10-19)

- **All documentation is now in English** (README, CONTRIBUTING, `docs/`, the `README.md` files and this file), so that people outside the team can follow the project now that it is public. Nothing was summarized: same sections and content, same addresses and commands.
- **New rule (see "Language" above):** docs and new code comments in English; conversation and commit/PR messages stay in Spanish. Your Claude will keep talking to you in Spanish, but it writes what goes into NOTES.md and here in English.
- NOTES.md section titles changed accordingly (e.g. "Estado del repositorio" → "Repository status", "Bloqueos" → "Blockers", "Diagnóstico de bloqueos" → "Diagnosing blockers"); the references in code comments were updated too. "bloqueo N" is now "blocker N".
- Nothing to regenerate after `git pull`: only text and comments changed (one comment in `overrides/gow2_overrides.cpp`, so the next `recompile.ps1 -SyncOnly` + `cmake --build` recompiles just that block, ~20 s).

### Public repo (cxldsakeb, 2026-10-18)

- The repo becomes **public** to let people know the port is under way (people on Twitter were asking about one) and to open it to contributors and forks. Beforehand the whole history was reviewed: only text, nothing from the game, no credentials.
- Public-facing README (status, notice that there is no playable version yet, summary in English), `CONTRIBUTING.md` and `.recomp.json` set to `in-progress`. **Be careful with what gets pushed from now on:** anyone can see it.

### Phase 4: cutscenes (blocker 17), speed measured and goodbye to TEMPORAL (cxldsakeb, 2026-10-17, evening)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`6d3379b`**), **`recompile.ps1`** with `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` set (the translation of `movn`/`movz` changed: `generated/` has to be regenerated) and build the runner.

- **Blocker 17, the cutscenes:** neither the opening one nor the one after the first QTE against the Colossus was showing. Their videos carry **ADPCM** audio and the runtime only delivered it to the game if it was registered as PCM (GoW1's case): the player waited for the first audio packet and gave up on the movie. Fixed in the fork (`ac9dfaf`). Also, the movies ran at ~0.57× because of a busy-wait in libmpeg: override of `0x190A08` with `catchUpToHost`, as in GoW1. **The opening cutscene plays in full, at 30 frames per second and with the audio in sync.** The one after the QTE is still to be confirmed by playing. **Its audio was a beep** (blocker 18): the runtime did not do the scratchpad DMA in interleaved mode, which the game uses to split the two channels; fixed in the fork (`6d3379b`) and **checked by ear**. Saving at a save point is still to be tested.
- **Real speed, finally measured:** `GOW_PERF_DIAG=1` + `GOW_PERF_FRAME_PC=0x14E380` → `guest_flip_hz`. On cxldsakeb's PC: **60 FPS in the menu and ~44–59 in the first area with combat**. **F10 now shows those real FPS** (`[G]`, fork `19c65a5`). What limits it: the game sends each frame and waits for its FINISH before preparing the next one, so the EE (~12 ms) and the VU1 (~12 ms) run in series; `Release` and a bigger GS queue change nothing (NOTES.md, "Repository status").
- Also in the fork: `SetupThread` provides `argc`/`argv` like a console does (**there is no TEMPORAL left**), `movn`/`movz` write only 64 bits (like the R5900) and the memory card keeps its file open between writes (saving should be faster; **still to be tested by saving**).
- Pending image issues, unchanged (NOTES.md, "Repository status"): blue line with Kratos, wavy lines underwater, ghost to the left of Kratos (for DaniSBR1).

### Phase 4: memory card, saves and loads (blocker 16) (cxldsakeb, 2026-10-17)

**After the `git pull`: you have to regenerate** (`recompile.ps1` with `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` set, ~1 min) and build the runner (~14 min on cxldsakeb's PC): `config/funcmap.csv` changed.

- Saving never got to write to the card (and it asked to format on every boot): libmc2's `switch` that dispatches its operations (`0x2B8CC0`) was split into two functions in `funcmap.csv`, and its cases gave `missing-target`. Joined with `tools/find_split_switches.py --fix` (this was the only case). **Checked by playing: it formats, saves (~10–20 s the first time) and loads without asking to format.** Also, the card reopens its 8 MB file on every page written: slow (NOTES.md, "Memory card: performance").
- Pending items for DaniSBR1 in NOTES.md ("Repository status"): blue line with Kratos, wavy lines underwater, cutscenes that get skipped (the opening one and the one after the first QTE against the Colossus), ghost to the left of Kratos.

### Phase 4: the hall now shows in full (blocker 15) and PCSX2 as a reference (cxldsakeb, 2026-10-17)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`eb5657e`**) and `cmake --build` (only `gs_hw_backend.cpp` changes).

- **Blocker 15:** walls, ceilings and NPCs more than a few meters away disappeared behind the sky. The game clears Z by drawing sprites with **a Z format into Z memory** (and with its own `ZBUF` for the lower half), and the backend discarded them. Fixed in the fork; the first area looks as on the PS2 (big windows with the Colossus).
- **Portable PCSX2 as a reference** and `tools/gsdump/` to read its GS dumps (Z per draw, Z buffer simulation). How to set it up in NOTES.md, "PCSX2 for comparison".
- **New pending items** (NOTES.md, "Repository status"): blue line that follows Kratos (for DaniSBR1), the opening betrayal cutscene that does not show up, the ghost to the left of Kratos and the faint edge of the distant horizon.

### Phase 4: native VU1 and IOP, from ~3 to an estimated ~35–40 FPS (cxldsakeb, 2026-10-16, night)

- **Everyone has to capture and generate on their own PC** (SETUP.md, 7b; the data comes from the game and is not pushed): 4 min of play with `GOW_VU1_CAPTURA` + `GOW_VU1_NATIVE_OFF=1` + `PS2X_IOP_DUMP_MODULES`, and then `recompile.ps1 -SyncOnly` with `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` set (~20 min). Without this the game runs at ~3 FPS: the interpreted VU1 takes 97% of the time.
- `tools/iop/iopn_gen.py` had to be fixed (*delay slot* labels in GoW II's LIBSD and 989NOMID). Measurements in NOTES.md, "Native VU1 and IOP in GoW II".
- **Next for speed:** measure real FPS with `GOW_PERF_FRAME_PC` (GoW II's *flip* address is still missing). With the speed now reasonable, we can go back to the graphics.

### Phase 4: the blue outlines come from upscaling (cxldsakeb, 2026-10-16, night)

- **Blue outlines:** they only appeared at scale 2. **The bright blue line is solved (blocker 14, fork `2b20dc4`)**: the bloom weights by the frame's alpha, and a reinterpreted read left it at PS2 resolution. After the `git pull`: `git submodule update --init --recursive` and `cmake --build` (only `gs_hw_backend.cpp` changes, a few minutes). **What remains** is a faint edge only on the distant horizon, which is already there before the bloom (suspicion and method in NOTES.md, "Repository status"). What PCSX2 enables for GoW II is in NOTES.md, "PCSX2 settings for God of War II".
- **Do not trust the F10 counter:** on cxldsakeb's PC it shows 60 FPS and 100% speed, but the game runs at ~2 FPS. Speed gets looked at after the graphics.
- **Memory card:** MC2_D, SIO2D and SIO2MAN are identical to GoW1's, so their IOP replacements are valid.

### Faster builds (DaniSBR1, 2026-10-16)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`a50613b`**). Nothing else is needed: CMake reconfigures itself and enabling `/MP` does not recompile anything.

- **`/MP`** (fork): the runner and the runtime are compiled with several `cl.exe` at once; before, one at a time. The number comes from each PC's RAM and cores (at configure time: `PS2X: MSVC parallel compilation /MPn`; it is changed with `-DPS2X_MSVC_MP_PROCESSES=N`). Full rebuild on DaniSBR1's PC: **40 min 24 s → 21 min 13 s**.
- **`recompile.ps1`** copies to the runner only the files whose content changed: a `-SyncOnly` with no changes no longer recompiles anything, and a changed override recompiles a single chunk (~20 s).
- Measurements and details in NOTES.md, "Faster builds". With this you no longer need to copy the override by hand to iterate quickly: `recompile.ps1 -SyncOnly` + `cmake --build` does the same.

### Phase 4: sky fixed, F12/F10 and speed measured (DaniSBR1, 2026-10-16, afternoon)

**After the `git pull`:** `git submodule update --init --recursive` (submodule at **`3b71410`**), `cmake -S PS2Recomp -B out/build` (new raylib options) and **recompile the whole runner** (~45 min: `gs_frontend.h` changed, and everything includes it).

**Done:** blocker 13 (a blue polygon over the sky: the shadows' Z16 copy read the wrong depth target, fork `d0b6162`). **F12** = borderless fullscreen / window; **F10** = FPS counter like PCSX2's (`GOW_SHOW_FPS=1` shows it at startup). With it the first area was measured on DaniSBR1's PC: **~58–60 real FPS and 100% speed** (`FPS … [B]`: GoW II copies each frame to a single display buffer). raylib's built-in F12 screenshot was disabled, since it left `screenshotNNN.png` files (images of the game) in the repo root.

**Next:** the thin blue outlines against the sky at scale 2 (compare the same scene at `PS2X_GS_HW_SCALE=1`), Kratos's "ghost", and checking combat, HUD and save/load by playing. Details in NOTES.md, "Repository status".

### Phase 4: the scenery is no longer green (DaniSBR1, 2026-10-16)

**For the next person's Claude: read "Repository status" in `docs/NOTES.md` first.** After the `git pull`: `git submodule update --init --recursive` (the submodule points to **`8a4abab`**) and recompile the runner (only `gs_hw_backend.cpp` changes: `cmake --build`, a few minutes, if `generated/` is already up to date).

**Done, blocker 12:** the GS backend discarded the column copies the game uses to restore the green channel, because **they end at a fraction of a pixel** (`y = 895.4375`) and `tryShuffle` required integer endpoints. The partial mask that was suspected was not the cause (`0x3FFF` covers whole bytes). Fixed in the fork: the scenery looks right and Kratos's shadow is drawn (checked with a screenshot). Phases 2 and 3 are closed in the ROADMAP.

**Next:** (1) a translucent blue polygon in the sky; (2) the shadows' Z16 → framebuffer copy, which the backend still discards (`shuffleWhy=13`); (3) speed (native VU1 and IOP). Method for the GS: a local counter of the backend's rejection reasons (not pushed to the fork), described in NOTES.md. DeepSeek helps read PCSX2 and the backend (public material), never captures or logs from the game.

### Phase 2, initialization passed: the game loads its resources and draws (cxldsakeb, 2026-10-16)

**For the next person's Claude: read "Repository status" in `docs/NOTES.md` first.** After the `git pull`: `git submodule update --init --recursive` (the submodule points to `9ccffe1`) and recompile the runner (if `generated/` is already up to date, `cmake --build`, a few minutes).

**Done:** blockers 8 and 9, both in the runtime.
- **8:** the EE's `Count` register did not advance, and the resource loader (`0x18CF70`, with a per-frame time budget) never finished.
- **9:** in GoW II **no** native IOP replacement was applied (LIBSD, memory card, SIO2). The game names its modules `NAME.IRX;1` and its LIBSD is a different build, with different offsets. Without them, uploading a sound bank waited for an SPU2 DMA that does not exist.

Now the game loads (`R_PermA`, `SBP_general`…), enters its main loop and the GS draws stably. The SMPD protocol and the loader are in NOTES.md ("Resource loader and SMPD protocol"). New tools in the override: `GOW2_CALL_PROFILE` (most-called functions) and `GOW2_PEEK` (variables in memory).

**Afterwards (blocker 10):** the "no controller connected" warning came up. It was fixed with the `libpad2` override (adapted from GoW1, GoW II addresses verified). Without a controller the keyboard works: `Enter` = Start, `X` = cross, arrows, `WASD`… (the full map is in NOTES.md, "Repository status").

**Phase 3: gameplay is reached, with sound** (cxldsakeb, 2026-10-16). The scenery comes out **green** (blocker 12, open). The game counts the characters' shadows in the green channel and restores it with 16-bit bit-masked copies that the GS backend (`tryShuffle`) does not do yet; the full diagnosis, with captures that confirm it, is in NOTES.md. **Next task: implement those copies in `gs_hw_backend.cpp` (fork).** A workaround from the game side (nulling out `0x16EDB8`) leaves the screen black: discarded.

**Before:** the full title screen shows up (at a few FPS), the menu responds and, when starting a new game, the first video plays. That required blocker 11: the runtime's generic `sceIpuInit` stub carries another game's addresses, and it was replaced with an override using GoW II's. **Next:** confirm the video looks right and reach gameplay. Without a keyboard, `GOW2_PAD_SCRIPT=15:start,20-60/3:x` (NOTES.md, "Diagnosing blockers"). Keep an eye on the sound, which now goes through GoW II's LIBSD replacements.

### Phase 2: blockers 6 and 7 solved in the runtime; the game requests resources from SMPD (cxldsakeb, 2026-10-16)

**For the next person's Claude: read "Repository status" in `docs/NOTES.md` first.** After the `git pull`: `git submodule update --init --recursive` (the submodule points to `df2a082` on `godofwar2`) and recompile the runner (if `generated/` is already up to date with phase 2's `funcmap.csv`, `cmake --build` is enough, ~2 min; if not, `recompile.ps1` and ~25–50 min).

**Done:** Daniel's `godofwar2-ioman` branch works (SMPD reads the whole TOC) and is merged into `godofwar2`. Blocker 7: SMPD split its paths with `strtok` (the runtime returned 0, `files = NULL`) and used `GetSystemTimeLow`, which did not exist. Fixed in the fork; also, unhandled IOP imports now always leave the `[IOP] unhandled import` warning. **New method:** `tools/irx_imports.py` cross-referenced with the catalog of what the runtime implements (NOTES.md, "IOP imports used by GoW II"): it predicted blocker 7 before running.

**~~Next: blocker 8~~ done, see above.** The EE requests resources from SMPD (`R_LglScA…`, `R_PermA…`), SMPD reads 384 KB of `PART1.PAK` and both end up waiting; thread 1, in the loop at `0x18CF70`. Details and useful traces (`PS2X_IOP_TRACE`, `PS2X_TRACE_RPC=123`) in NOTES.md.

### Phase 2 under way: it boots, loads the IRXs and starts reading the disc (DaniSBR1, 2026-10-16)

**For the next person's Claude: read "Repository status" in `docs/NOTES.md` first.** Summary:

**Done (on `main`):** the game boots, loads its 8 IRXs into the IOP interpreter (`SIO2MAN`, `DBCMAN`, `SIO2D`, `MC2_D`, `DS2U_D`, `LIBSD`, `989NOMID`, `SMPD`), binds to their RPC servers and starts reading the disc. Five blockers solved, all in `overrides/gow2_overrides.cpp` (registered for `SCUS_974.81`):
1. `sceCdReadDvdDualInfo` (`0x2B6BF0`), as in GoW1.
2. **TEMPORAL** `0x1875E0`: the runtime's `SetupThread` ignores the argument block and `main` receives `argc = 0`; the IRX paths came out empty. The real fix: have the runtime fill in the arguments (proposal in NOTES.md).
3. `setPrivateGuestHeap(0xA0000, 0xFF000)`: the runtime's internal heap was stomping on the game's main heap (as in GoW1).
4. Resume points: `ps2_recomp` does not register them in the `entry_points` functions (it calls them `entry_XXXX`). **The scanner's 733 functions now go in `config/funcmap.csv`** (`tools/make_funcmap.py --extra`), not in `entry_points`.
5. `sceSifLoadStartModuleBuffer` (`0x2D4850`): the IOP module that ships inside the ELF (as in GoW1).

**~~Next step (pending testing)~~ done, see above:** blocker 6. SMPD reads `cdrom0:\GODOFWAR.TOC` with `ioman` (`open`/`read`/`lseek`/`close`) and the runtime's IOP did not implement it: it loads an empty index and the game hangs at `0x27D7E8` (empty state stack). **The implementation is on the fork's `godofwar2-ioman` branch** (`b14b951`, not merged into `godofwar2`): it compiles, but it was never tested in the game. To test it:
1. `git pull`, `git submodule update --init --recursive` and then `git -C PS2Recomp fetch` + `git -C PS2Recomp switch godofwar2-ioman`.
2. `.\scripts\recompile.ps1` (you have to regenerate: `funcmap.csv` changed) and `.\scripts\run.ps1 -Config RelWithDebInfo` (~50 min the first time).
3. In the log, no `unhandled import ioman`. With `GOW2_THREAD_WATCH=5`, thread 1 should no longer stay at `pc=0x0027d7e8`.
4. If it works: merge `godofwar2-ioman` into the fork's `godofwar2`, push, and commit the new submodule pointer here (rule 5). If not: the next blocker, with the same method.

To investigate: `GOW2_THREAD_WATCH=5` and `GOW2_BOOT_TRACE=1` (NOTES.md, "Diagnosing blockers", which also explains how to test an override in ~1 min without recompiling everything). DeepSeek is only good for reading public material (the runtime, `referencia/gow1/`): NOTES.md, "DeepSeek in phase 2".

### Phase 1 closed: first configuration that recompiles and builds (DaniSBR1 and cxldsakeb, 2026-10-15)

**Done:** DaniSBR1's phase 0 (same SHA1). Ghidra → 5,874 functions; `tools/make_funcmap.py` → `config/funcmap.csv` (functions only: the export also brings 31,986 internal labels that must not go in); scanner → 733 more functions (at first in `entry_points`, as in GoW1; **since phase 2 they go in `funcmap.csv`**, see above). `recompile.ps1` generates 6,607 functions **without errors** and **the runner compiles and links** (0 unresolved symbols). Leftovers from GoW1 fixed: `$ConfigToml` undefined in `_common.ps1` and the `tools/` scripts pointing to GoW1's ELF.

**Pending to close phase 1:**
1. ~~Sony stubs~~ **done: 76 of 79** (the other 3, `exit`, `read` and `sceTtyRead`, are not linked in GoW II). **GoW II uses SDK 3.0.0**: the libraries are shifted as a block relative to GoW1 (+0x3C0E8 libmpeg, +0x3C0F0 ipu/graph/dma/cdvd, +0x3BB68 sif/deci2) and each stub was verified in GoW II's ELF. Method in NOTES.md ("Sony library stubs"). Watch out: in SDK 3.0, `StartThread`, `iWakeupThread` and `InitTLB` are C functions in front of the syscall wrapper, and the stub goes on the C function.
2. ~~`[mmio]` table~~ **done: 240 entries** from `ps2_analyzer`, equivalent to GoW1's table (which was the analyzer's output untouched). The analyzer, with its Sony symbol database, also confirms the 76 stubs at the same addresses. See NOTES.md ("[mmio] table and cross-check with ps2_analyzer").
3. Each `recompile.ps1` regenerates all of `generated/` and the runner takes ~50 min to rebuild entirely: batch changes before regenerating.

~~Still to confirm that the runner compiles with the `[mmio]` table~~ **Phase 1 closed** (cxldsakeb, 2026-10-15): with stubs and `[mmio]`, the `RelWithDebInfo` runner compiles and links without `LNK2001`/`LNK2019`/`LNK4088`. Watch out: the runner is linked with `/FORCE` and an `.exe` always comes out; what counts is that the linker log has none of those errors.

**Next: phase 2** (first boot), already under way (see above). Keep an eye on the 8 `sceSif*` that `ps2_recomp` turns into stubs by name.

### Handover to DaniSBR1: phase 1, function map and first configuration (cxldsakeb, 2026-10-15)

cxldsakeb closed phase 0 on his side and passes phase 1 to Daniel. Daniel's Claude: guide him step by step and explain the why.

**Done (phase 0, cxldsakeb):** repo created (GoW1's scripts and tools, submodule on `godofwar2`, `referencia/gow1/`), disc extracted, **reference ELF in NOTES.md** (`SCUS_974.81` v1.01, SHA1 `E6A62A0F3E781C29513F563F4625B9B3A9103C56`) and `build-tools.ps1` building. Findings: the disc has the same structure as GoW1's (only `SMPD.IRX` changes) and the VU microprograms ship inside the ELF (`.DVP.overlay`).

**Steps for Daniel, in order:**
1. Accept the invitation to the repo and clone it with submodules (`docs/SETUP.md`, steps 1–5). With his ISO, `.\scripts\extract-iso.ps1` and **check that the SHA1 matches**. If it does not match (for example, a v1.00), stop and let us know: the addresses would not be valid.
2. Ghidra (SETUP, step 6) and `.\scripts\ghidra-analyze.ps1` → `config/gow.toml` and `config/gow_functions.csv`. Then, `py -3 tools/scan_missing_functions.py` for the functions Ghidra does not see (in GoW1 there were 634, mostly virtual methods).
3. **`config/funcmap.csv`**: Ghidra's CSV (same format, `Name,Start,End,Size`) plus the scanner's functions.
4. **`config/recomp.template.toml`**, taking `referencia/gow1/config/recomp.template.toml` as the model:
   - `[general]` with `input = "@ELF@"`, `ghidra_output = "@MAP@"`, `output = "@OUT@"` (filled in by `recompile.ps1`);
   - **stubs**: each Sony library function in GoW1's template (`sceMpeg*`, `sceIpu*`, `sceGs*`, `sceCd*`, `sceDma*`, libkernel…) has to be **found in GoW II** and given its address. Hints: nearby strings, the function's instruction pattern in GoW1 (`tools/elf_dis.py`), and who calls it (`tools/elf_xref.py`). If one cannot be found, better to leave it out and note it in NOTES.md than to put in a doubtful address;
   - `[mmio]`: the same, the hardware register accesses, looking for GoW1's equivalents in GoW II. It can come after the first boot if it is hard.
   - **DeepSeek can help** cross-reference lists (for example, GoW1's stub table against a listing of GoW II functions that only contains names and public patterns), but **never with disassembly or ELF data** (rule 1).
5. `.\scripts\recompile.ps1` until it generates C++ **without errors**, and build the runner: that is phase 1's "done" criterion. Note the identified functions and the problems in NOTES.md, and update this section.

What from GoW1 has not been brought over here: its overrides get redone with GoW II addresses only when a blocker calls for it (phase 2).
