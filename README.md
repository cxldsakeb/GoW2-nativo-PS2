# God of War II: native PC port (PS2 → Windows)

> [!NOTE]
> **Work in progress (WIP).** There is no version to download and play yet. What there is is real, documented progress: the game already reaches the first playable area, with sound, cutscenes and memory card, at ~60 FPS. If you want to follow it, help, or base your own fork on it, you're in the right place.

Static recompilation of **God of War II** (PS2, 2007) into a native Windows executable with [PS2Recomp](https://github.com/ran-j/PS2Recomp). It is not an emulator: the game's code is translated to C++ and compiled as a PC program, together with a runtime that plays the part of the PS2 (GS, VU1, IOP, sound, controller, videos and memory card).

It is the team's second project after [God of War](https://github.com/cxldsakeb/ROMS-nativas-PS2), which is already playable at ~60 FPS on Waffle565's runtime. GoW II uses an evolution of the same engine, so we start from that runtime and what we do here is the game-specific part: the function map, the recompiler configuration, the overrides and the runtime fixes the game requires.

> [!IMPORTANT]
> This repository **contains nothing from the game** (ISO, ELF, IRX, disc data) nor the C++ generated from it, and **it never will**. The only exception are the few screenshots of the port below. Everyone needs **their own legal copy** of God of War II (NTSC-U, `SCUS-97481`, v1.01). Don't ask for or share ISOs in the issues.

## Screenshots

The port running on PC (October 2026, internal resolution x2): the title screen, the opening of the first level with the Colossus of Rhodes, and combat in the first area.

| | |
|---|---|
| ![Title screen](docs/screenshots/1-title.jpg) | ![The Colossus of Rhodes](docs/screenshots/2-colossus.jpg) |
| ![Combat in the first area](docs/screenshots/3-combat.jpg) | ![Combat in the first area](docs/screenshots/4-combat.jpg) |

## Status

**Phase 4 (playable), in progress.** What works today, verified by playing:

- Boots and gets through the logos, the title screen and the menu.
- **Cutscenes** at 30 frames per second, with the audio in sync.
- **First playable area (Rhodes)** with sound, scenery, shadows and combat.
- **Memory card:** formats, saves and loads.
- **Speed:** 60 FPS in the menu and ~44–59 FPS in the first area with combat (measured on a team member's PC), with the VU1 and the IOP compiled to native code.

What's missing: some graphics glitches (a blue line that follows Kratos, wavy lines under water, a "ghost" to his left), checking the rest of the game beyond the first area, and reaching a stable 60 FPS in combat.

| Phase | Status |
|---|---|
| 0. Environment | Done |
| 1. Function map and recompiler configuration | Done: 6,607 functions, 76 Sony library stubs, 240 hardware register accesses |
| 2. First boot | Done: loads the 8 IOP modules, reads the disc and reaches the title screen |
| 3. Menu and videos | Done |
| 4. Playable at full speed | In progress: 18 blockers solved so far; graphics details and combat speed remain |

Details in [docs/ROADMAP.md](docs/ROADMAP.md). Every blocker, with its cause and its fix, is in [docs/NOTES.md](docs/NOTES.md): it is also a good guide to how a PS2 game is brought to PC with PS2Recomp.

## Getting started

You need Windows, Visual Studio 2022 and your copy of the game. Follow [docs/SETUP.md](docs/SETUP.md). In short: extract your ISO with `scripts/extract-iso.ps1` (the ELF's SHA1 must match the one in NOTES.md), build the tools, `scripts/recompile.ps1` and `scripts/run.ps1 -Config RelWithDebInfo` (the first build takes ~20–50 min depending on the PC). To run at full speed you have to generate the native VU1 and IOP with data from your own playthrough (SETUP.md, section 7b).

**Controls:** a PC controller or the keyboard. Keyboard: arrows = D-pad, `Z`/`X`/`C`/`V` = square/cross/circle/triangle, `Enter` = Start, `Q`/`E` = L1/R1, `1`/`3` = L2/R2, `WASD` and `IJKL` = sticks.

**Launcher (optional):** `.\scripts\build-launcher.ps1 -Run` builds and opens a small window to choose the graphics settings (internal resolution, 16:9 or 4:3, fullscreen at startup, FXAA, anisotropic filtering, sharpening, vsync, FPS counter) and start the game with **PLAY**, without recompiling anything. Details in [launcher/README.md](launcher/README.md).

**Window keys:** `F12` = fullscreen / windowed · `F10` = FPS counter (like PCSX2's) · `F1` = debug panel.

## Contributing

All help is welcome: testing on your PC, reporting bugs or sending fixes. Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening an issue or a PR. The runtime lives in our PS2Recomp fork ([DaniSBR1/PS2Recomp](https://github.com/DaniSBR1/PS2Recomp), branch `godofwar2`), which is the `PS2Recomp/` submodule.

And if you'd rather take it your own way: it's GPLv3, so you can make your own fork as long as you keep the same license and credit the source.

The context for Claude Code (and for anyone who joins) is in [CLAUDE.md](CLAUDE.md).

## Credits

- [**PS2Recomp**](https://github.com/ran-j/PS2Recomp) by ran-j and contributors (GPLv3): the recompiler and the base of the runtime.
- [**Waffle565/God-Of-War-Native-Port**](https://github.com/Waffle565/God-Of-War-Native-Port): its `runtime` branch (GPL-3.0) is the base of our runtime; `tools/iso_extract.py`, `tools/iop/` and `tools/vu1/vu1n_gen.py` come from its `main` branch, with the author's permission and through KIexster.
- [**KIexster/god-of-war-recomp**](https://github.com/KIexster/god-of-war-recomp) (GPLv3): the patches that put that base on top of PS2Recomp and, for GoW1, the configuration and overrides that we keep as a reference in `referencia/gow1/`.
- [**PCSX2**](https://github.com/PCSX2/pcsx2) (GPLv3+): hardware reference.
- Team: [@cxldsakeb](https://github.com/cxldsakeb), [@DaniSBR1](https://github.com/DaniSBR1) and [@Alejrr2](https://github.com/Alejrr2).

## License

**GPLv3** ([LICENSE](LICENSE)), same as PS2Recomp. God of War is a trademark of Sony Interactive Entertainment; this project is not affiliated with or endorsed by Sony. No game files, nor anything derived from them, are distributed.
