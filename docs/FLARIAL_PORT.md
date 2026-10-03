# Flarial-Modulbasis, 2026-10-03

Auf Wunsch des Nutzers liegen alle 117 Flarial-Modulordner unter `vendor/flarial/src/Client/Module/Modules`.
Events, Hooks, SDK und Signaturdefinitionen wurden ebenfalls übernommen. Original-Lizenz (AGPL-3.0)
und der feste Upstream-Commit stehen in `vendor/flarial`. Monchis bestehende UI, Launcher und
zusätzliche Module bleiben erhalten; das Worktree `Desktop/monchi-ui` wurde nicht verändert.

Die importierten Dateien sind die Quellbasis, kein bereits komplett integrierter zweiter Client.
Flarial verwendet ein anderes SDK, eigene Events, Rendering und Konfiguration. Jeder Port muss
an Monchis Schnittstellen angeschlossen werden. Ein vorhandener Menüschalter beweist keine Spielwirkung.

## Erster angeschlossener Bereich

| Modul | Bisher | Neue Anbindung | Prüfung |
|---|---|---|---|
| Freelook | Versionsbit in der entt-Registry verändert; Wirkung unbestätigt | Flarials nativer Kamera-/Spieler-Update-Hook mit drei Argumenten und Body-/Head-Yaw-Patches, über Monchis Einstellungen gesteuert; Kamera-Winkel vor Rückgabe wiederhergestellt | Lokale Hook-/Patch-Lifecycle-Tests bestanden; Signaturen und sichtbare Wirkung im Spiel noch offen |
| Third Person Nametag | Eigenes Poppins-Overlay, Formatierung entfernt | Flarials native Self-Tag-Sperre; keine Text-/Farbersetzung. Original-Overlay bleibt optional | Lokale Patch-Lifecycle-Tests bestanden; Signatur und sichtbare Wirkung im Spiel noch offen |

Freelook nimmt den nativen Hook, wenn `CameraUpdatePlayer`, `CameraYaw` und `CameraHeadYaw` gefunden werden, sonst den eigenen Weg über das Versionsbit im UpdatePlayerFromCameraComponent-Pool. Dieser Rückfall wurde am 3. Oktober im Spiel geprüft (Kamera dreht, Spieler bleibt, Sicht nach dem Loslassen pixelgleich). Der native Pfad ist ungeprüft. Der eigene Nametag braucht `OwnNametagGate`.
Die alten Pseudo-Freigaben ersetzen diese Anbindung nicht. Fehlende Signaturen sperren das Modul.
Die eingefügten Muster stammen aus Flarials Signaturdefinitionen; sie sind noch keine bestätigten
Monchi-Signaturen für 1.26.52.3.
Der 1.26-Head-Yaw-Store ist fünf Bytes lang; der Port ersetzt und restauriert die vollständige
Instruktion, nicht nur vier Bytes. Bei einem unpassenden zweiten Patch wird der erste zurückgenommen.

## Bestand und weitere Ports

Der vorherige Stand registriert 153 Module. Am 3. Oktober ergab ein Runtime-Audit in einer Welt
mit deaktivierten Demo-Daten 153 verfügbare Schalter. Das ist eine Verfügbarkeitsprüfung, kein
Funktionsnachweis. Der ältere Commit hatte 26 nicht angebundene Module entfernt; „0 grau“
beschreibt deshalb nicht den vollständigen gewünschten Funktionsumfang.

`tools/compare_modules.py` vergleicht alle registrierten Klassen mit der Flarial-Quellbasis und
erfasst native Effekt-Anfragen, Game-Aufrufe, Events, Signaturen und Optionsnamen. Nicht direkt
zugeordnete Ordner sind keine bestätigten fehlenden Funktionen: Gruppierung und Namen unterscheiden sich.

Die entfernten Render-Funktionen müssen wieder aufgenommen und einzeln angebunden werden:
Nametag Modifier, Item Physics, Hand-/Swing-Animationen, Block Hit, Left Hand, Smooth Sneak,
Minimal View Bobbing, Time/Weather/Environment Changer, Fog/Water/Hurt/Glint Color, Low Fire,
Particle Multiplier, GUI Scale, Movable Hotbar/Title/Bossbar, Light Overlay, Subtitles,
Skin Stealer und die serverabhängigen Timing-/Hurt-Funktionen. Bestehende Server-Regeln gelten weiter.

Nicht jeden Flarial-Ordner ungeprüft aktivieren: `RawInputBuffer` ist im heruntergeladenen
Stand ein Platzhalter; `NoHurtCam` nutzt teilweise Signaturen, die für 1.26 bereits als veraltet
markiert sind. Quellcode kopiert bedeutet nicht, dass jeder Hook auf dieser Version funktioniert.

## Weitere bereits korrigierte Stellen

- HUD-Blur nimmt seinen Hintergrund vor Monchis HUD und Menü auf; die Blur-Fläche folgt der HUD-Ausrichtung.
- Text-HUDs behalten Bedrock-Farben, Fett- und Kursivformatierung. System-Symbole ergänzen die HUD-Schrift;
  Resource-Pack-Zeichen brauchen weiterhin den nativen Minecraft-Renderer.
- Heap-Suche läuft mit niedrigerer Priorität, kurzen Pausen und weniger Wiederholungen.
- Frame-Limiter wartet nur begrenzt auf seinen Timer.
- Signaturen werden im DLL-Build eingebettet; ohne passende Einträge bleiben Funktionen gesperrt.
- Unbekannte Spielversionen verwenden keine älteren oder neueren Offsets als Ersatz.

Monchi wurde aus dem Spiel entladen. Die abschließende Modulliste enthielt nur Flarial.
Seit der Untersagung wurde kein neuer Monchi-Build injiziert. Weitere Sichttests werden erst
in einer später ausdrücklich freigegebenen Monchi-Testsession durchgeführt.


## Entscheidung 2026-10-03: Flarial als Standard, Monchi obendrauf

Alle 117 Flarial-Module werden Monchis Grundbestand. Monchi legt seine Extras darauf: mehr Einstellungen,
eigene UI, HUD-Editor, Animationen und die eigenen Zusatzmodule (Mini-Spiele, Monchi Online, Bildfilter,
Hive-/Zeqa-Werkzeuge usw.). Wo es ein Modul bei beiden gibt, gewinnt die Spielanbindung, die im Spiel
nachweislich wirkt; die Monchi-Einstellungen kommen dazu.

Aufbau:

1. `flarial_core`: statische Bibliothek aus Flarials Spiel-Teil (SDK, Hooks, Events, Utils/Memory, Module-Logik)
   mit den nötigen Abhängigkeiten (EnTT, libhat, safetyhook, fmt, magic_enum, NES-Eventsystem, glm).
   Nicht übernommen: Flarials Menü, Konfig-Speicher, Discord-Anbindung (das Upstream-Repo existiert nicht mehr),
   Lua-Skripting, eigene DX-/D2D-Overlay-Schicht. Das alles hat Monchi bereits.
2. Einstellungs-Brücke: Flarials `setDef`/`getOps` und `addSlider`/`addToggle`/`addColorPicker`/`addDropdown`/
   `addKeybind`/`addTextBox` werden so nachgebaut, dass sie Monchi-Einstellungen anlegen. Jedes Flarial-Modul
   erscheint so in Monchis Menü, gespeichert in Monchis Konfig, und kann um eigene Einstellungen erweitert werden.
3. Zeichen-Brücke: Flarials HUD-Zeichenaufrufe (`FlarialGUI::…`) laufen über Monchis ImGui-Zeichenliste und
   Schriften; Rendering im Spiel (SetupAndRender, MinecraftUIRenderContext, Actor-/Level-Render) bleibt Flarials Hook.
4. Ein Overlay-System: Monchis DX-Hook bleibt der einzige. Flarials eigener kiero/D2D-Pfad wird nicht gebaut.
5. Reihenfolge: zuerst Kern + Brücken bauen, dann die 26 entfernten Module, dann gemeinsame Module abgleichen,
   dann Flarial-only-Module, dann Monchi-Extras wieder darauf setzen. Jede Spielanbindung zählt erst nach
   bestätigter Signatur und sichtbarer Wirkung im Spiel (Regel 7).

## Stand 2026-10-03 abends

- `flarial_core` baut den ganzen Flarial-Client (alle Module, SDK, Hooks, Events, GUI-Engine, ClickGUI) als
  statische Bibliothek, Schalter `-DMONCHI_FLARIAL=ON`. Mit Monchis ImGui 1.93 und MinHook, ohne Lua, Discord-
  Bibliothek, rift und curl (curl-Aufrufe laufen über eine kleine WinHTTP-Schicht in `dll/src/flarial/shim`).
- Angepasste Dateien liegen in `dll/src/flarial` (gleiche Pfade wie upstream, jede mit SPDX-Zeile und Grund).
  CMake baut einen zusammengesetzten Baum im Build-Ordner: upstream, darüber die angepassten Dateien.
- Noch nicht verbunden: Monchi startet den Kern noch nicht. Nächster Schritt ist ein eigener Start statt
  `Client::initialize` (das schickt Telemetrie an Flarial, legt Flarial-Ordner an und lädt Flarials Konfig),
  Flarials DX-Hooks aus der Hook-Liste nehmen und `RenderEvent`/`SetupAndRenderEvent` aus Monchis Present
  speisen, dann die Einstellungs- und Zeichenbrücke.
- Wichtig für Tests: Monchi mit Flarial-Kern darf nicht zusammen mit der installierten Flarial-DLL laufen,
  beide würden dieselben Spielfunktionen hooken.
