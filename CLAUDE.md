# Mochi Client

Free PvP client for Minecraft Bedrock Edition (Windows, GDK builds). Launcher (.exe) + injected DLL.

## Rule #1

This must become the best Minecraft Bedrock PvP client ever made. Best input latency, best UI, best animations, best PvP feel, the most polished and complete modules — better than Flarial, Onix, Latite and every client before them. It should have everything they have and go further, so that anyone who tries it immediately thinks "wow, this is so much better".

Every decision is measured against this. "Works" is not done — done means it beats the best existing version of that feature: smoother, faster, prettier, more configurable. Before building a module, check how Flarial/Onix do it and name what ours does better. Measure input and performance changes, don't guess.

Arbeitstitel "Mochi" — vor dem ersten Release umbenennen (global suchen/ersetzen).

## Read first

- `docs/MASTER.md` — Gesamtübersicht, Wünsche, Arbeitsteilung, Test-Checkliste. Zuerst lesen.
- `docs/PLAN.md` — architecture and phases. Work phase by phase, never skip ahead.
- `docs/MODULES.md` — every module, its tier and phase.
- `docs/INPUT.md` — input latency strategy.
- `docs/VERSIONING.md` — signatures, version detection, updates, downgrades.
- `docs/UI.md` — design spec for ClickGUI, HUD editor and launcher.
- `docs/SERVERS.md` — per-server module rules, auto-blocking on join.
- `docs/STATUS.md` — what works, percent done, next steps. Update after every work step.
- `docs/RESEARCH.md` — other clients, Felix's wishes. Put every new insight here.
- `docs/UPDATES.md` — how client, launcher, signatures and Minecraft updates work, downgrade plan.
- `docs/LAUNCH.md` — what makes Mochi stand out, release checklist, open risks.

## Hard rules

1. Per-server rules from `docs/SERVERS.md` are enforced: a module blocked on a server cannot be turned on there.
2. Legit only. No reach, killaura, aim assist, autoclicker, velocity, scaffold, ESP through walls, or anything that sends packets the vanilla client wouldn't. Modules marked `server-rules` in MODULES.md ship disabled by default with a warning in their description.
3. Do not copy code from Flarial (AGPL-3.0) or decompiled Onix. Reading them to understand an approach is fine; write our own implementation. Signatures are facts about the game binary and may be re-derived, but find them ourselves and verify them.
4. Never distribute Minecraft files. The version switcher downloads only through the user's own Microsoft entitlement.
5. A missing signature must never crash the game. The module goes grey in the GUI with "not available on this version".
6. Every hook body runs inside the crash guard (see PLAN.md, core/guard).
7. A game module only counts as done when it changes the game while it is on: its hook must be installed and verified (log line plus visible effect), not just drawn in the menu. If the hook is missing the module is grey, never silently inactive. Overlay modules are the only ones allowed to be pure drawing.

## Code style

The code must read like a hand-written open-source project.

- C++20, MSVC, CMake, for the client and the launcher (ImGui, DirectX 11, WinHTTP).
- No comments that restate the code. Comment only non-obvious reverse-engineering facts (where a signature points, why an offset is what it is). No banner comments, no section dividers, no emoji, no "// Helper function to ..." lines.
- No doc-comment blocks on every function. Names carry the meaning.
- Short, specific names: `Zoom`, `fovTarget`, `scanSig`, not `ZoomModuleImplementation`, `targetFieldOfViewValue`.
- Files: `PascalCase.hpp/.cpp` for classes, `snake_case` for folders.
- No overly defensive boilerplate, no logging every step, no TODO placeholders committed.
- Keep functions small; prefer early returns.
- Commit messages: short, lowercase, imperative ("add zoom smoothing", "fix dx12 resize crash"). One logical change per commit.
- README in plain, casual tone. No marketing walls of bullet points.

## Languages

English is the default, German is the second language. The client and launcher follow the Windows UI language unless the user picks one (stored in `%LOCALAPPDATA%\Mochi\lang.txt`, shared by both).

- All user-visible text in code is written in English. The English text is the key.
- Show text through `i18n::tr("English text")`, or `i18n::fmt("Hello {}", x)` for format strings. Module names, descriptions and setting labels are stored as English literals and translated where they are drawn (`gui/`).
- Every new string gets a German entry in the German table of its area: `dll/src/core/Lang.cpp` (client), `launcher/src/LangUi.cpp` and `LangApp.cpp` (launcher). Missing entries fall back to English, never to an empty string.
- Do not put translated text into config keys or ids.

## Build

```
tools/cross.sh setup    # once: MinGW, Wine, Xvfb (Linux cloud session)
tools/cross.sh build    # builds dll, launcher and test host
tools/cross.sh shots    # runs the dll in the test host under Wine, takes screenshots

# on Windows
cmake -S dll -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cmake -S launcher -B build-launcher -G "Visual Studio 17 2022" -A x64 -DMOCHI_DLL=%CD%\build\Release\Mochi.dll -DMOCHI_COSMETICS=%CD%\cosmetics
cmake --build build-launcher --config Release
```

With `MOCHI_DLL` set the launcher embeds the client and the cosmetics, so the release is one exe. Without it the launcher looks for `Mochi.dll` next to itself.

## Test loop

1. Build DLL.
2. Start Minecraft, run `tools/inject.ps1` (dev injector) or the launcher.
3. Read `%LOCALAPPDATA%\Mochi\logs\latest.log`.
4. Unload with the eject hotkey (Ctrl+L) before rebuilding — the DLL file is locked while loaded.

Ask the user to verify in-game behaviour; you cannot see the game. Ask for screenshots when a visual changes.
