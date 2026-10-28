# Setting up the environment (Windows)

These steps have to be followed **once per person**. All commands are run in PowerShell, from the root of the repo.

## 1. Clone the repo (with the submodule)

```powershell
git clone --recurse-submodules https://github.com/cxldsakeb/GoW2-nativo-PS2.git
cd GoW2-nativo-PS2
```

If you had already cloned it without submodules: `git submodule update --init --recursive`.

**The submodule points to our fork** (`DaniSBR1/PS2Recomp`, branch `godofwar2`), which carries Waffle565's base. To catch up after a `git pull`:

```powershell
git submodule sync --recursive
git submodule update --init --recursive
```

## 2. C++ compiler and CMake

You need **Visual Studio 2022** (Community or Build Tools) with the *Desktop development with C++* workload, which includes MSVC and CMake:

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

The scripts find the CMake that ships with Visual Studio on their own. If you use another version of VS, pass it with `.\scripts\build-tools.ps1 -Generator "<generator>"`.

## 3. Allow running the scripts

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

## 4. Extract your copy of the game

```powershell
.\scripts\extract-iso.ps1 -IsoPath "path\to\God of War II.iso"
```

- It extracts both DVD layers to `game/disc/` with `tools/iso_extract.py --layout` (needs Python 3), writes the sector map `game/disc/.disc_layout`, copies the IRX to `game/disc/IOP_MOD/` and detects the main ELF (in the USA version it should be `SCUS_974.81`).
- At the end it prints the **ELF's SHA1**. Compare it with the one in [NOTES.md](NOTES.md). If it doesn't match, you have a different region or revision and the project's addresses **won't work for you**.
- It only works with `.iso`. If your copy is `.bin/.cue` or another format, convert it to ISO first.
- The game reads its data from `game/disc/`, not from the ISO. If you had the disc extracted with the old format (without `.disc_layout`), run the script again.

## 5. Build the PS2Recomp tools

```powershell
.\scripts\build-tools.ps1
```

The first time it takes several minutes. It produces `ps2_analyzer.exe` and `ps2_recomp.exe` in `out/build/`.

## 6. Ghidra + Emotion Engine extension

**In GoW II it is needed** for phase 1: it is what gives the function map of an ELF without symbols. It's enough for one person to install it, although it's handy for all of you to have it for investigating.

1. **Java 21**: `winget install EclipseAdoptium.Temurin.21.JDK`
2. **Ghidra 12.1.4**: download it from https://github.com/NationalSecurityAgency/ghidra/releases and unzip it (for example in `C:\ghidra`).
3. **Emotion Engine extension** (the PS2's processor, with MMI/VU0 support, which Ghidra's regular MIPS doesn't decode properly): download from https://github.com/chaoticgd/ghidra-emotionengine-reloaded/releases the zip **for your exact Ghidra version** (`ghidra_12.1.4_PUBLIC_..._ghidra-emotionengine-reloaded.zip`). In Ghidra: *File → Install Extensions → +*, pick the zip and restart Ghidra.
   The quick way: unzip the zip directly into `C:\ghidra\ghidra_12.1.4_PUBLIC\Ghidra\Extensions\`.
4. Analyze the ELF and export the config **without opening the GUI** (it takes about 1 minute):

   ```powershell
   .\scripts\ghidra-analyze.ps1          # Ghidra in another folder: -GhidraDir "path" or $env:GHIDRA_INSTALL_DIR
   ```

   The script creates the project in `ghidra/` (ignored by Git) and writes `config/gow.toml` + `config/gow_functions.csv`. If the project already exists, it only exports again; `-Reanalyze` starts from scratch.
5. To investigate functions with the decompiler, open the GUI (`C:\ghidra\ghidra_12.1.4_PUBLIC\ghidraRun.bat`) and in it the project `ghidra/GodOfWar.gpr`. If you fix function boundaries, run `ghidra-analyze.ps1` again (with Ghidra closed) to re-export.

## 7. Recompile and run

```powershell
.\scripts\recompile.ps1                        # ELF → C++ (generated/) → PS2Recomp runner
.\scripts\run.ps1 -Config RelWithDebInfo       # builds ps2EntryRunner and starts the game; log in out/logs/
```

- You need `config/recomp.template.toml` and `config/funcmap.csv` first (phase 1 of [ROADMAP.md](ROADMAP.md)).
- The first build of the runner **will take quite a while** (20–40 min).
- If you only changed something in `overrides/`: `.\scripts\recompile.ps1 -SyncOnly` and then `.\scripts\run.ps1`.

## 7b. Native VU1 and IOP (in GoW1, from 7–8 to ~60 FPS)

For when GoW II already boots (phase 4). Without this the VU1 is interpreted and runs very slowly. The data comes from the game, so everyone captures it on their own PC and it is **never uploaded**.

```powershell
# 1. Capture (a ~3 min playthrough up to the first playable area). The folders have to exist.
New-Item -ItemType Directory -Force out\capturas\vu1, out\capturas\iop
$env:GOW_VU1_CAPTURA = "$PWD\out\capturas\vu1"       # VU1 microprograms
$env:GOW_VU1_NATIVE_OFF = '1'                         # optional since fork afd5c2c (see below)
$env:PS2X_IOP_DUMP_MODULES = "$PWD\out\capturas\iop"  # the game's 8 IRX
$env:GOW2_PAD_SCRIPT = '15:start,20-120/3:x'         # gets to the first area on its own (or play with the keyboard)
.\scripts\run.ps1 -Config RelWithDebInfo -NoBuild    # close it after ~4 min: the more you play, the more microprograms
Remove-Item Env:GOW_VU1_CAPTURA, Env:GOW_VU1_NATIVE_OFF, Env:PS2X_IOP_DUMP_MODULES, Env:GOW2_PAD_SCRIPT

# 2. Generate and build with them
$env:GOW_VU1_MICROCODIGO = "$PWD\out\capturas\vu1"
$env:GOW_IOP_MODULOS = "$PWD\out\capturas\iop"
.\scripts\recompile.ps1 -SyncOnly
.\scripts\run.ps1 -Config RelWithDebInfo
```

- With `$env:GOW_VU1_STATS = '1'`, the log says how many VU1 pairs are interpreted: with everything captured, 0.
- In GoW II (cxldsakeb's PC, 2026-10-16): 4 min of capture give ~1,000 VU1 microprograms and the 8 IRX; step 2 takes ~20 min (it generates and builds the VU1). The `GOW_VU1_MICROCODIGO` and `GOW_IOP_MODULOS` variables have to be set on **every** `recompile.ps1`: without them, CMake reconfigures itself without native VU1.
- **New areas run slowly until they are captured** (Typhon's Cavern: 16 FPS, then 35–55). Since fork `afd5c2c`, `GOW_VU1_CAPTURA` also works with the native VU1 on: it saves only the microprograms that are still missing, at full speed. Play through the new areas with only `GOW_VU1_CAPTURA` set (into the same folder), then repeat step 2.
- Memory card: `$env:GOW_MC0 = "$PWD\game\disc\play.mcd"` (it is created automatically).

## 8. Recommended helper tools

- **PCSX2** (with its debugger): runs the real game; set breakpoints on addresses and see what a function returns. It is the fastest way to find out how something that fails in the port *should* behave.
- **ripgrep** (`winget install BurntSushi.ripgrep.MSVC`): useful for searching for addresses in `generated/`, which has thousands of files.
- **Python 3 + `capstone` + `pyelftools`** (`py -3 -m pip install capstone pyelftools`): for the scripts in `tools/` (see `tools/README.md`).
- **OpenCode** (`npm install -g opencode-ai`, needs Node.js): to hand DeepSeek high-volume reading tasks, for free, with `scripts/deepseek.ps1`. Public material only; see "Tasks for DeepSeek" in `CLAUDE.md`.
- **PS2Recomp Discord**: https://discord.gg/JQ8mawxUEf
- **PS2Tek** (hardware documentation): https://psi-rockin.github.io/ps2tek/
