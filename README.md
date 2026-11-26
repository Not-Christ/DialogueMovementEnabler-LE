# Dialogue Movement Enabler LE

Backport of Vermunds' [Dialogue Movement Enabler SE](https://github.com/Vermunds/DialogueMovementEnabler-SE) (v2.4.0) to **The Elder Scrolls V: Skyrim — Legendary Edition (1.9.32.0 / SKSE 1.7.3)** using **CommonLibLE** with LE 1.9.32.0. Cross-verified against `libSkyrim` and `skyrim-classic-re`.

## Building (Windows Win32 / x86)
Requires **Visual Studio 2022** (with Desktop Development with C++ / Win32 x86 toolset `v143`) and **CommonLibLE** (`../CommonLibLE_MASTER_FULL_PROJECT_AUDITED_V0_9_5_2026-08-04/SDK_SOURCE`).

### Option 1: CMake (Recommended)
```bat
cmake --preset vs2022-win32-release
cmake --build --preset build-release
```

### Option 2: Visual Studio 2022 / MSBuild
Open `DialogueMovementEnabler.sln` in Visual Studio 2022, select **Release | Win32**, and build — or run `build_release_win32.bat` from a **Developer Command Prompt for VS 2022**.

## Installation
1. Copy the built `DialogueMovementEnabler.dll` and `ini/DialogueMovementEnabler.ini` to `Data\SKSE\Plugins\`.
2. Check `Documents\My Games\Skyrim\SKSE\DialogueMovementEnabler.log` on first run to confirm all hooks installed cleanly.

## License
GPL-3.0-or-later WITH Modding Exception (same as upstream `DialogueMovementEnabler-SE`). See `LICENSE` and `EXCEPTIONS.md`.
