# Match3_UE5

Prototype of a Match-3 game in Unreal Engine 5 (C++) with a 2D board view.

## Features

- Configurable board size (`GridWidth` / `GridHeight`, default `8x8`)
- Four gem colors (`Red`, `Blue`, `Green`, `Yellow`)
- Swap only adjacent gems
- Match detection for `3+` horizontally and vertically
- Cascade resolve loop:
  - clear matches
  - apply gravity
  - refill empty cells
- 2D visuals:
  - flat gem sprites (plane meshes)
  - visible board grid behind gems
- Mouse drag/swipe input
- Auto camera alignment above board on start (Spectator: no mouse-look during play)

## Project Structure

- `Source/Match3/Public/Match3Types.h`
  - `EGemType` enum
- `Source/Match3/Public/Match3Grid.h`
  - core board logic (`AMatch3Grid`)
- `Source/Match3/Public/Match3Gem.h`
  - visual gem actor (`AMatch3Gem`)
- `Source/Match3/Public/Match3PlayerController.h`
  - input + camera logic (`AMatch3PlayerController`)
- `Source/Match3/Public/Match3GameMode.h`
  - game setup + grid auto-spawn (`AMatch3GameMode`)
- `MATCH3_PROJECT_DOCUMENTATION.txt`
  - extended technical documentation (includes **section 9 — session log** before each release-style commit)

## Requirements

- Unreal Engine `5.7.x`
- Visual Studio with C++ toolchain (Windows)

## How to Run

1. Open `Match3.uproject`.
2. If asked, generate project files and build C++ modules.
3. Open the main level (or your test level).
4. In `World Settings`, set `GameMode Override` to:
   - `BP_Match3GameMode` (recommended), or
   - `Match3GameMode`
5. Press `Play`.

## Controls

- Grid input is polled in `AMatch3Grid::Tick` (works with Game+UI cursor).
- **Click** a gem to select; **click an orthogonal neighbor** to swap.
- **Hold LMB** on the selected gem for the wobble/press animation; **release** after dragging at least `InputMinSwipePixels` to swipe-swap.
- Invalid swap (no match) reverts automatically.

## Tuning in Editor

### Board (`AMatch3Grid` → Match3 | Tune)

- **Tune Preset** (`UMatch3TunePreset` data asset) **or** **Local Tune** when preset is empty
- From `FMatch3GameplayTune`: `GridWidth`, `GridHeight`, `CellSize`, grid lines, gem colors, `InputMinSwipePixels`, trace length, selection animation, etc. (see `Match3Config.h`)

### Camera (`AMatch3PlayerController`)

- `CameraHeightMultiplier`
- `CameraHeightOffset`
- `CameraPitch`

Mouse **look** is disabled on the controller so the board stays top-down.

## Git Notes

Repository is configured with Unreal-friendly `.gitignore`:

- ignored: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`
- tracked: `Source/`, `Config/`, `Content/`, project files and docs

## Context Automation (Cursor Hooks)

Project includes local hooks in `.cursor/hooks.json`:

- `sessionStart` -> builds auto context summary
- `afterFileEdit` -> appends edit events into daily context log

Generated context files:

- `.cursor/context/SESSION_CONTEXT_AUTO.txt`
- `.cursor/context/CONTEXT_YYYY-MM-DD.txt`

## Roadmap (Suggested)

- Score system and UI
- Special gems (4/5 match, T/L patterns)
- Hint system and no-move reshuffle
- Better animations and VFX/SFX
- Level goals and progression

