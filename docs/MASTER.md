# Mochi Client: Gesamtübersicht

Diese Datei ist der Einstieg. Wer neu in das Projekt kommt (Claude Code am PC, eine andere Session, ein Mensch), liest zuerst sie, dann `CLAUDE.md` und die Detail-Docs.

## Ziel

Der beste Minecraft-Bedrock-PvP-Client, den es gibt. Kostenlos, später auf GitHub veröffentlicht. Er soll Flarial, Onix, Latite und alle anderen in jedem Punkt übertreffen: Input-Latenz, UI, Animationen, Anzahl und Tiefe der Module, Updates, Stabilität.

- Onix (bezahlt, 90+ eingebaute Module plus 125 Community-Module per Lua) soll in Funktionen und Gefühl nachgebaut werden, aber gratis. Name, Logo und Design von Onix nicht übernehmen.
- Flarial (gratis, 140+ Module, AGPL-3.0): ansehen, verstehen, aber keinen Code kopieren. Sonst müsste Mochi auch AGPL sein.
- Latite: gratis, JS-Plugins, Vergleichswert.
- Maßstab für jedes Feature: "Ist das besser als bei den anderen?" Nur "funktioniert" reicht nicht.

## Was Felix will (aus dem Chat, alles gilt)

- Injizierbarer Client (Launcher-exe + DLL) für Bedrock auf Windows, wie Flarial.
- Eine fertige exe, später auf GitHub veröffentlichen.
- Der Code soll nicht nach KI aussehen: kaum Kommentare, keine Beschreibungsblöcke, kurze natürliche Namen, normale Ordnerstruktur. Siehe Code-Stil in `CLAUDE.md`.
- Onix-Layout: Modulliste links, Einstellungen rechts. Weiches Scrollen mit Trägheit, Karten die beim Scrollen leicht einblenden, Hover-Lift, sanfte Panel-Übergänge, federnde Toggles, Slider mit Wert-Bubble, Tooltips mit Animation.
- Girly Look: Pink, Herz-Logo, alles per Farbe anpassbar.
- Mehr Unterkategorien, nicht nur "CPS" unter PvP. PvP muss viele Module haben (Kampf-Info, Combo, Reach-Anzeige, Hitbox, Armor/Potion HUD, Hurt Color usw.).
- Beste Eingabe: "fast CPVP", Low-Latency-Modus, null Verzögerung zwischen Klick und Bild. Soll sich von anderen Clients absetzen, auch wenn die so etwas ähnlich haben. Siehe `INPUT.md`.
- Ein Ziel von ca. 135 Modulen, alle mit vielen Optionen, nicht nur ein Schalter.
- Version-Switcher im Launcher: jede Minecraft-Version installieren und umschalten. Siehe `VERSIONING.md`.
- Auto-Update: Der Launcher prüft GitHub-Releases, lädt neue DLL und neue Launcher-Version selbst. Felix lädt nur ein Release hoch.
- Pro Server werden verbotene Module automatisch gesperrt. Siehe `SERVERS.md`.
- Clientname ist vorläufig "Mochi", vor dem Release umbenennen (global suchen/ersetzen).
- Alles Wichtige gehört in die Projektdateien, damit nichts verloren geht.

## Arbeitsteilung

- Hier in der Cloud-Session (kein Windows, kein Minecraft): Code schreiben, Docs pflegen, Module bauen. Kompilieren und Testen ist hier nicht möglich (nur `clang`/`cmake`, kein MinGW, kein Wine).
- Claude Code am PC von Felix (ab ca. 8 h nach Sessionstart): bauen mit MSVC, injizieren, im echten Spiel testen, Log lesen, Signaturen finden. Dort steht die Test-Checkliste unten.
- Felix: Ergebnisse, Logs und Screenshots liefern. Das Spiel kann keine Session sehen.

## Technik in Kürze

- DLL: C++20, MSVC, CMake. MinHook (Hooks), Dear ImGui (alle UI, DX11- und DX12-Backend), nlohmann/json.
- Rendering: DX12 ist Standard (über D3D11On12), DX11 als Rückfall. Hooks auf `Present` und `ResizeBuffers`.
- Server-Erkennung ohne Spielsignaturen über Netzwerk-Hooks.
- Module melden benötigte Signaturen an. Fehlt eine, wird das Modul grau ("nicht verfügbar auf dieser Version"), das Spiel stürzt nie deshalb ab.
- Jeder Hook läuft im Crash-Guard. Ein Modul mit Fehler wird abgeschaltet, nicht das Spiel.
- Signaturen liegen als `sigs/<version>.json` auf GitHub und werden beim Start nachgeladen. Ein Fix nach einem MC-Update ist ein Commit, kein neues Release.
- Launcher: C# .NET 8, Single-File-Exe, Injection per `LoadLibraryW` + `CreateRemoteThread`.
- Config: `%LOCALAPPDATA%\Mochi\` (`configs/`, `themes/`, `scripts/`, `logs/`, `cache/sigs/`).

Ordner im Repo:

| Ordner | Inhalt |
|---|---|
| `dll/src/core` | Start, Crash-Guard, Log, Config, Events, Pfade, HTTP |
| `dll/src/hook` | DX-Hooks, Input, Netzwerk-Erkennung |
| `dll/src/render` | Zeichnen, Schrift, UI-Helfer |
| `dll/src/gui` | ClickGUI, HUD-Editor, Themes, Toasts |
| `dll/src/modules` | Module, nach Kategorie in Unterordnern |
| `dll/src/sig` | Signatur-Scanner und -Loader |
| `dll/src/server` | Server-Regeln |
| `launcher` | Launcher/Injector (aktuell nur Skelett) |
| `servers` | `servers.json` mit Regeln pro Server |
| `tools/testhost` | Testprogramm, das die DLL ohne Minecraft lädt |
| `docs` | alle Docs |

## Wo steht was

| Datei | Inhalt |
|---|---|
| `CLAUDE.md` | Regeln für jede Session: Ziel, harte Regeln, Code-Stil, Build |
| `docs/PLAN.md` | Architektur und Phasen 0 bis 9 mit fertigen Prompts |
| `docs/MODULES.md` | Alle Module mit Tier, Priorität, Notiz |
| `docs/INPUT.md` | Strategie für niedrige Latenz |
| `docs/VERSIONING.md` | Signaturen, Versionserkennung, Updates, Downgrade |
| `docs/UI.md` | Design von ClickGUI, HUD-Editor, Launcher |
| `docs/SERVERS.md` | Regeln pro Server, Modul-Tags |
| `docs/RESEARCH.md` | Infos zu anderen Clients, Wünsche |
| `docs/STATUS.md` | Fortschritt in Prozent, nächste Schritte |
| `docs/HISTORY.md` | Was in welcher Session passiert ist |

## Harte Grenzen

- Nur legitime Module. Kein Reach, Killaura, Aim Assist, Autoclicker, Velocity, Scaffold, ESP durch Wände, keine Pakete, die der normale Client nicht senden würde. Das schützt Felix vor Bans und das Projekt vor Takedowns.
- Keine Minecraft-Dateien verteilen. Der Version-Switcher lädt nur über die Microsoft-Berechtigung des Nutzers.
- Kein Code aus Flarial oder dekompiliertem Onix.

## Was ehrlich offen ist

- Im echten Minecraft wurde noch nichts getestet, auch DX12 nicht.
- Es gibt keine einzige echte Spiel-Signatur. Alle Spiel-Module (Zoom, Freelook, Fullbright, Hitbox, Hurt Color ...) hängen daran.
- Der Launcher ist nur ein Skelett. Auto-Update und Version-Switcher fehlen.
- Der Version-Switcher ist das schwerste Stück: GDK-Builds sind verschlüsselt und an das Microsoft-Konto gebunden. LeviLauncher macht es, wie genau muss noch nachgesehen werden.
- Ein Client senkt den Ping nicht. Er kann die lokale Kette Maus → Bild kürzen. Werbung nur mit gemessenen Zahlen.

## Test-Checkliste für Felix' PC (Claude Code)

1. VS 2022 Build Tools, CMake, Git, .NET 8 installieren (siehe Phase 0 in `PLAN.md`).
2. DLL bauen: siehe Build-Abschnitt in `CLAUDE.md`. Müssen Compile-Fehler behoben werden, das ist normal, da der Code hier nie mit MSVC gebaut wurde.
3. Minecraft starten, per `tools/inject.ps1` oder Launcher injizieren. `%LOCALAPPDATA%\Mochi\logs\latest.log` lesen.
4. Prüfen: Overlay sichtbar, Rechts-Shift öffnet Menü, Strg+L entlädt ohne Crash. Zuerst DX12, dann DX11 erzwingen.
5. Alle Overlay-Module einzeln an- und ausschalten, HUD-Editor testen.
6. Latenz-Overlay vorher/nachher mit den Optionen aus `INPUT.md` vergleichen.
7. Danach Signaturen finden (Reihenfolge in `STATUS.md`), `sigs/<version>.json` schreiben, `STATUS.md` aktualisieren.
