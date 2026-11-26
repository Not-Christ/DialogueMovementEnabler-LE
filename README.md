# Dialogue Movement Enabler LE

Backport of Vermunds' [Dialogue Movement Enabler SE](https://github.com/Vermunds/DialogueMovementEnabler-SE) (v2.4.0) to **The Elder Scrolls V: Skyrim — Legendary Edition (1.9.32.0 / SKSE 1.7.3)** using **CommonLibLE** with LE 1.9.32.0 UI/Input/Dialogue layouts cross-verified against `libSkyrim` and `skyrim-classic-re`.

## Features Ported to Skyrim LE
- **Movement & Controls in Dialogue**: Remaps configured keyboard/mouse and controller inputs (`MenuControls::ReceiveEvent` vtable hook) while `Dialogue Menu` is open and not in menu mode (`MenuManager::IsInMenuMode` / SkyrimSouls LE compatible).
- **Auto-Close Dialogue Menu**: Tracks bounding-box distance (`GetBoundLeftFrontBottom` / `GetBoundRightBackTop`) to the current `MenuTopicManager` speaker (`talkingHandle` / `lastSpeaker`) on `DialogueMenu::ProcessMessage` (`kShow`) and checks distance every frame in `DialogueMenu::NextFrame`, closing via `UIManager::AddMessage("Dialogue Menu", kHide, nullptr)`.
- **Camera Unlock, Free Look & Turn-to-Speaker Control**:
  - `PlayerControls::ReceiveEvent(MenuOpenCloseEvent*) -> IsGamepadEnabled` (`FreeLook` cursor-edge-turn bypass)
  - `Actor::SetDialogueWithPlayer -> MenuTopicManager::StartDialogue` (ForceGreet detection)
  - `MenuTopicManager::StartDialogue -> PlayerCharacter::InitDialogueLookAt` (disable turn-to-speaker)
  - `PlayerControls -> PlayerCharacter::GetDialogueLookAngle` (unlock camera rotation)
  - All 4 call-site hooks use runtime `rel32` `CALL` validation inside confirmed `TESV.exe 1.9.32.0` functions (with optional hex overrides in `[HOOKS]` and safe skip-on-mismatch logging).

## Configuration Adapted for Skyrim LE
Because `ModConfigUI` is SE-only and does not exist on Skyrim LE, configuration has been adapted for LE in two complementary ways:

1. **INI Configuration with Automatic Live Hot-Reload (`Data\SKSE\Plugins\DialogueMovementEnabler.ini`)**
   - Uses the exact same sections and keys as SE (`[GENERAL]`, `[CONTROLS_KEYBOARD]`, `[CONTROLS_CONTROLLER]`, `[AUTOCLOSE]`), plus `bHotReloadINI = true` and an optional `[HOOKS]` override section.
   - Auto-generates the documented INI file on first launch if missing.
   - With `bHotReloadINI = true`, editing the INI (or updating it via MCM) takes effect immediately on the next dialogue open without restarting Skyrim.

2. **Optional SkyUI MCM (`scripts/source/DME_MCM.psc` + `interface/translations/DialogueMovementEnabler_ENGLISH.txt`)**
   - Replicates both pages of the SE `ModConfigUI` menu (**Settings** and **Controls** with Keyboard & Mouse + Controller toggles, Auto-Close Distance & Tolerance sliders, and Restore Defaults).
   - Writes directly to `Data/SKSE/Plugins/DialogueMovementEnabler.ini` via PapyrusUtil LE (`MiscUtil.WriteToFile`), which the DLL automatically hot-reloads.
   - To use the MCM in-game: compile `DME_MCM.psc` with the Skyrim LE Creation Kit (with SkyUI SDK + PapyrusUtil LE scripts) and attach it to a Start-Game-Enabled quest in a small `.esp`.

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
2. (Optional, for SkyUI MCM) Copy `interface\translations\DialogueMovementEnabler_ENGLISH.txt` to `Data\Interface\Translations\` and compile/package `scripts\source\DME_MCM.psc`.
3. Check `Documents\My Games\Skyrim\SKSE\DialogueMovementEnabler.log` on first run to confirm all hooks installed cleanly.

## License
GPL-3.0-or-later WITH Modding Exception (same as upstream `DialogueMovementEnabler-SE`). See `LICENSE` and `EXCEPTIONS.md`.
