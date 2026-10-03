# Plan

## Ziel

Ein kostenloser Bedrock-PvP-Client, der mehr kann als Flarial (ca. 120 Module, gratis) und Onix (90+ Module, Lua-Scripting, kostenpflichtig per Patreon).

Wo wir besser werden:

| Bereich | Flarial / Onix | Monchi |
|---|---|---|
| Nach MC-Updates | DLL muss neu gebaut werden, Tage Wartezeit | Signaturen liegen als `sigs.json` auf GitHub und werden beim Start nachgeladen. Ein Fix ist ein Commit, kein neues Release |
| Fehlende Signatur | Crash oder Modul kaputt | Modul wird grau, Rest läuft weiter |
| Crashes | Spiel stürzt ab | Crash-Guard fängt ab, deaktiviert das Modul beim nächsten Start, schreibt Log |
| Input | Raw Input Buffer (im Open-Source-Code nur ein Stub) | Kompletter Latenz-Stack, siehe INPUT.md, mit eingebauter Latenz-Messung |
| Scripting | Onix nur bezahlt | Lua gratis |
| Versionen | Externe Tools nötig | Version-Switcher im Launcher |
| UI | Standard | Eigenes Theme-System, Presets, Glas-Blur, Animationen |
| Server-Regeln | Spieler muss selbst wissen, was erlaubt ist | Verbotene Module werden pro Server automatisch gesperrt |
| Server-Profile | — | Modul-Sets pro Server (Hive, Zeqa, CubeCraft …), wechseln automatisch |

## Repo-Struktur

```
monchi/
  dll/                 C++ Client
    src/
      core/            Entry, Guard, Log, Config, Events, Hotkeys
      hook/            MinHook-Wrapper, DX11/DX12-Present, Input, Game-Hooks
      render/          ImGui-Backend, Fonts, Blur, Animationen
      sdk/             Spielklassen (ClientInstance, LocalPlayer, Actor …)
      sig/             Scanner, Versionserkennung, sigs.json-Loader
      gui/             ClickGUI, HUD-Editor, Theme-Editor, Notifications
      modules/         ein Ordner pro Modul
      script/          Lua 5.4 Binding
    lib/               minhook, imgui, libhat, nlohmann_json, lua
  launcher/            C# .NET 8 WPF
  sigs/                sigs.json pro Spielversion (wird vom Client geladen)
  servers/             servers.json mit Regeln pro Server
  tools/               inject.ps1, sigcheck (prüft sigs.json gegen eine exe)
  assets/              Logo, Icons, Fonts
  docs/
  .github/workflows/   Build + Release
```

## Technik

- **DLL**: C++20, MSVC, CMake. MinHook für Hooks, ImGui für alle UI, libhat (MIT) für Pattern-Scanning, nlohmann/json für Configs.
- **Rendering**: Minecraft nutzt standardmäßig DX12. Hook auf `IDXGISwapChain::Present` und `ResizeBuffers`. Für DX12 entweder ImGui-DX12-Backend (Command Queue über `ExecuteCommandLists` abgreifen) oder D3D11On12. Fallback DX11, falls der Spieler DX11 erzwingt.
- **Launcher**: C# .NET 8 WPF, Single-File-Exe. Injection via `LoadLibraryW` + `CreateRemoteThread`. Vor dem Inject ACL auf der DLL für `ALL APPLICATION PACKAGES` setzen (nötig bei UWP-Builds, schadet bei GDK nicht).
- **Configs**: `%LOCALAPPDATA%\Monchi\` mit `configs/`, `themes/`, `scripts/`, `logs/`, `cache/sigs/`.

Aktueller Fortschritt: siehe `docs/STATUS.md`. Recherche und Wünsche: `docs/RESEARCH.md`.

## Phasen

Jede Phase endet mit einem Build, den du im Spiel testest. Erst wenn alles läuft, geht's weiter. Zu jeder Phase steht unten ein Prompt, den du Claude Code geben kannst.

### Phase 0 — Setup (einmalig)

Auf deinem PC installieren: Visual Studio 2022 Build Tools (C++ Desktop + Windows SDK), CMake, Git, .NET 8 SDK. Optional für Signaturen: Ghidra oder IDA Free, Cheat Engine.

> Prompt: *Lies CLAUDE.md und docs/. Prüfe, ob VS Build Tools, CMake, Git und .NET 8 installiert sind, installiere fehlendes mit winget. Lege die Repo-Struktur aus PLAN.md an, binde minhook, imgui, libhat, nlohmann_json als Git-Submodule ein und sorge dafür, dass ein leeres DLL-Projekt und ein leeres WPF-Projekt bauen.*

### Phase 1 — Kern

- DllMain → eigener Thread, Log-Datei, Crash-Guard (SEH um jeden Hook, Vectored Exception Handler als Netz).
- Present-Hook DX12/DX11, ImGui läuft, Testfenster sichtbar.
- Input-Hook (WndProc), Rechts-Shift öffnet/schließt ein leeres Menü, Ctrl+L entlädt sauber.
- Event-Bus, Modul-Basisklasse, HUD-Modul-Basisklasse (Position, Skalierung, Drag), Config speichern/laden.
- Dev-Injector `tools/inject.ps1`.

> Prompt: *Phase 1 aus PLAN.md umsetzen. Ziel: DLL injizieren, ImGui-Overlay im Spiel, RShift toggelt Menü, Ctrl+L entlädt ohne Crash. Ich teste und schicke dir das Log.*

### Phase 2 — UI

ClickGUI, HUD-Editor, Theme-Editor genau nach `docs/UI.md`. Notifications (Toast oben rechts). Modulsuche.

> Prompt: *Phase 2: ClickGUI, HUD-Editor und Theme-Editor nach docs/UI.md. Mit den 5 Theme-Presets. Noch ohne echte Module, nimm 3 Dummy-Module zum Testen.*

### Phase 3 — Overlay-Module (versionsunabhängig)

Alle Module mit Tier `overlay` aus MODULES.md. Die brauchen keine Spielsignaturen und laufen auf jeder Version: FPS, CPS, Keystrokes, Mousestrokes, Uhr, Stopwatch, Memory, Crosshair, Doom, Snake … Input-Daten kommen aus dem eigenen Raw-Input-Hook.

> Prompt: *Phase 3: alle Module mit Tier overlay aus docs/MODULES.md, jeweils mit Einstellungen im ClickGUI.*

### Phase 4 — SDK, Signaturen, Versionserkennung

Das Herzstück für "läuft nach Updates". Details in `VERSIONING.md`.

- Versionserkennung aus der Exe-Ressource.
- `sigs.json`-Format, Loader (lokal → Cache → GitHub raw), Scanner mit Wildcards, Ergebnis-Cache pro Exe-Hash.
- SDK-Grundgerüst: ClientInstance, LocalPlayer, GuiData, Options, Level.
- `tools/sigcheck`: CLI, das alle Sigs gegen eine `Minecraft.Windows.exe` prüft und eine Tabelle ausgibt.
- Server-Erkennung + Server-Regel-System nach `docs/SERVERS.md`, `servers/servers.json` wird wie sigs.json nachgeladen.
- Erste Spiel-Module: Zoom, FOV Changer, Fullbright, Coordinates, Toggle Sprint/Sneak.

> Prompt: *Phase 4 nach docs/VERSIONING.md. Fang mit Versionserkennung und dem Sig-System an, dann das SDK. Signaturen müssen wir zusammen finden: sag mir, welche Funktion du brauchst, ich öffne die exe in Ghidra oder du nutzt Ghidra headless.*

### Phase 5 — Input & Performance

Alles aus `docs/INPUT.md`, inklusive Latenz-Overlay, um vorher/nachher zu messen.

> Prompt: *Phase 5 nach docs/INPUT.md. Baue zuerst das Latenz-Overlay, damit wir jede Änderung messen können.*

### Phase 6 — Restliche Spiel-Module

In Blöcken von 8–10 Modulen aus MODULES.md (Tier `game`), sortiert nach Priorität. Nach jedem Block: testen, Sigs in `sigs/` committen.

> Prompt: *Phase 6, nächster Block: [Modulnamen]. Für jedes Modul: welche Hooks/Sigs nötig, implementieren, in sigs.json eintragen.*

### Phase 7 — Launcher, Auto-Update, Releases

- Launcher-UI nach UI.md: großer Launch-Button, News, Changelog, Einstellungen.
- Auto-Update: GitHub Releases API (`/repos/<user>/<repo>/releases/latest`), vergleicht Version, lädt DLL + neue Launcher-Exe, tauscht sich selbst über Rename-Trick aus, SHA256 prüfen.
- GitHub Action: bei Tag `v*` DLL + Launcher bauen, Release mit Assets und Checksums anlegen.

> Prompt: *Phase 7: Launcher fertig, Auto-Update über GitHub Releases, Release-Workflow. Mein Repo heißt <user>/<repo>.*

### Phase 8 — Version-Switcher

Im Launcher: Liste aller verfügbaren Release-/Preview-Versionen, installieren in getrennte Ordner, umschalten, Auto-Update des Spiels für gepinnte Versionen verhindern. Download nur über die Microsoft-Berechtigung des Users. Vorher prüfen, wie LeviLauncher das macht (Lizenz beachten, nicht kopieren). Details in VERSIONING.md.

### Phase 9 — Lua-Scripting, Server-Profile, Feinschliff

- Lua 5.4: Module und HUD-Elemente per Script, API-Doku, Script-Ordner mit Hot-Reload.
- Server-Profile: Modul-Set wechselt beim Verbinden je nach Server-IP.
- Regeln für Zeqa, NetherGames, Mineville nachrecherchieren und in servers.json eintragen. Anträge auf Freigabe bei CubeCraft und Lifeboat.
- Config-Import/-Export als Code zum Teilen.
- README, Screenshots, Release 1.0.

## Realistisch

- Phase 1–3 schafft Claude Code in wenigen Sessions. Ab Phase 4 bremst das Finden der Signaturen, weil dafür die Spiel-Exe analysiert werden muss.
- Nach jedem großen MC-Update müssen Sigs aktualisiert werden. Mit `sigs.json` und `sigcheck` sieht man sofort, welche kaputt sind, und kann sie ohne neues Release fixen.
- Ziel für v1.0: alle `overlay`-Module, die wichtigsten 40 `game`-Module, Launcher mit Auto-Update. Der Rest kommt danach.
