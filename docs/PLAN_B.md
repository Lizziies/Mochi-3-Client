# Großer Plan für Session B (Module)

Dies ist die vollständige Arbeitsliste. Session B macht **alles**, was für Parität mit Flarial und für unseren Vorsprung noch fehlt. Reihenfolge ist verbindlich: erst die Phasen mit dem größten Nutzen für PvP-Spieler.

Grundlagen, die du vor dem Start lesen musst: `CLAUDE.md`, `docs/TEAM.md`, `docs/FLARIAL_REAL.md` (was Flarial wirklich kann, aus dem Quellcode gelesen), `docs/FEATURE_AUDIT.md` (Stufen), `docs/SDK.md`, `docs/PARITY.md` (ehrliche Lage).

## Regeln für alle Phasen

1. **Arbeite auf deiner Branch, mergen von `main` regelmäßig** (`git fetch origin main && git merge origin/main`). Jeder Block endet mit `tools/cross.sh build` ohne Fehler und `tools/cross.sh shots` ohne Absturz.
2. **Englisch im Code, `i18n::tr`, deutsche Tabelle** in `dll/src/modules/Lang_B.cpp` (Vorlage `dll/src/core/Lang.cpp`). Alle bisherigen deutschen Texte in einem Durchlauf umstellen (Phase 0).
3. **Jedes neue Modul** in `Manager.cpp` registrieren, in `dll/src/modules/Tiers.cpp` einstufen (Stufe 1 oder 2), Server-Regel in `servers/servers.json` prüfen.
4. **Fertig heißt:** Das Modul hat dieselben Einstellungen wie das Original (siehe `FLARIAL_REAL.md`), läuft mit den Demo-Daten (Schalter in "Game Support"), hat Englisch und Deutsch und ist grau, wenn Daten fehlen. Im echten Spiel wirkt es erst mit Signaturen, das macht später Felix mit Claude Code am PC.
5. **Legit:** nichts senden, was der normale Client nicht sendet. Keine Spoof-Zahlen. Siehe `CLAUDE.md`.
6. Nach jedem Block `docs/STATUS.md` und `docs/HISTORY.md` ergänzen (nur anhängen), committen, pushen.
7. Kein Code aus Flarial kopieren. Verhalten lesen, selbst schreiben.

## Phase 0: Aufräumen (zuerst, 1 Block)

- `main` mergen, Konflikte lösen.
- Alle deutschen Texte (Beschreibungen, Labels, Choices, Unterkategorien wie "Eigene Werte", Toasts) auf Englisch und in die Tabelle `Lang_B.cpp`.
- Prüfen: `Combo Counter` (480-ms-Regel, 15-s-Reset, "Count to Negatives"), `Reach Counter` (2 Nachkommastellen, 15-s-Reset), `Hit Ping` (Zeit vom Angriff bis zur Server-Bestätigung plus Reach), `Opponent Reach` (nächster Spieler im 10-Block-Radius, gültig bis 5,5 Blöcke), `Instant Hurt Animation` ("nur mit voller Rüstung", "Team ausschließen"), `Potion HUD` (Bottom Up, römische Ziffern, rot bei ≤ 5 s), `Arrow/Totem Counter` ("nur wenn in der Hand"), `Pot Counter` (Splash-Tränke, Slots 0 bis 35). Angleichen.

## Phase 1: Server-Module (höchster Nutzen für Spieler)

### 1.1 Hive Utils (ein Modul, alles aus `FLARIAL_REAL.md`)

Auto-Requeue (`/q` oder `/hub`), Solo-Modus, Team-Ausscheiden, Requeue-Taste. **Map Avoider** pro Spiel und Variante mit eigener Liste. **Rollen-Requeue:** Murder Mystery (Murderer, Sheriff, Innocent), Hide and Seek (Hider, Seeker), Death Run (Death, Runner, Todes-Limit 1 bis 100). **Custom-Server-Code kopieren** (optional mit `/cs`). **Chat aufräumen:** Promo `[!]`, Unlock-Hinweis, Join-Meldungen, Nachrichten von Nicht-Rang-Spielern und Hive+, "No Teaming". **Auto-Accept** für Freunde und Party. **Auto Map Vote** mit Hinweis und Ansage (`@here vote for {map}!`).

Technik: Chat-Zeilen über `ChatEvents` (SDK), Befehle über das vorhandene Eingabe-Senden (`inject`/Chat-Senden, wie Auto GG). Jede Funktion einzeln schaltbar. Nur auf Hive aktiv (Server-Erkennung aus `server/Rules`).

### 1.2 Zeqa Utils

Auto-Requeue in Ranked/Unranked-Duell-Queues nach dem Match, Chat aufräumen (Promo, Join, Leave, Kill-Streak), Freundschafts- und Duell-Anfragen automatisch annehmen.

### 1.3 Hive Stats (Overlay)

Statistiken pro Spieler aus der öffentlichen Hive-API (WinHTTP, Cache, Rate-Limit beachten): FKDR (BedWars), K/D, Win-Rate, Level, Siege, Niederlagen, Kills, Final Kills, Tode, Spiele, erstes Spiel, je Spiel ein Primär- und Sekundärwert. Farbschwellen pro Wert (einstellbar), Team-Farben aus den Nametags, Spieler-Hervorhebungen mit Farbe, auch in Lobbys, Bestenliste (All-Time/Monatlich, Zeilen 1 bis 100, Refresh 5 bis 300 s, eigenes Fenster), Anker an neun Positionen. Nur anzeigen, was die Hive-API öffentlich liefert.

### 1.4 Weitere Server-Hilfen

Auto GG je Server prüfen (Zeqa, Galaxite, Mineville, Hive, CubeCraft, Lifeboat: Ende-Erkennung pro Server). NetherGames, Mineville, Galaxite: erst Regeln recherchieren, dann nur Chat-Hilfen.

## Phase 2: Crystal- und PvP-Latenz ("Crystal Speed")

Ziel: Crystal-PvP und Nahkampf fühlen sich schneller an, **ohne** auf den Server zu warten und **ohne** Pakete zu senden, die der normale Client nicht sendet. Alles ist rein clientseitig und optisch/Eingabe-seitig.

1. **Crystal Optimizer** (existiert als Gerüst in `combat/Pvp.hpp`): kein Drehen und Wippen, kein Sockel, optional sofort ausblenden. Ausbauen: der getroffene Crystal wird **lokal sofort als zerstört behandelt** (nicht mehr gezeichnet und nicht mehr vom Fadenkreuz-Test getroffen), damit der nächste Schlag oder das nächste Platzieren nicht an einem "Geister-Crystal" hängen bleibt, bis die Server-Antwort kommt. Option "Bestätigung nicht abwarten".
2. **Instant Hit** (existiert): Klick-Weg messen und verkürzen (siehe `INPUT.md`).
3. **Instant Hurt Animation**, **Block Hit**, **Swing Speed/Angle**, **Normal Hit Crit**: sofortiges Treffer-Feedback.
4. **Geister-Entities nach Kill/Tod sofort entfernen** (Spieler, die laut Chat gestorben sind, nicht mehr zeichnen oder anvisieren).
5. Alles in einem Menü-Abschnitt "Crystal PvP" und "Hit Feedback". Alle mit Warnhinweis (`markRisky`), Server-Regeln in `servers/servers.json`.

Verifizierung am PC später: gemessen mit dem Latenz-Overlay (Klick bis Bild). Ohne Messung keine Werbung mit Zahlen.

## Phase 3: Tiefe bei den Kern-Modulen (Parität, siehe `FLARIAL_REAL.md`)

In dieser Reihenfolge, jeweils mit **allen** dort genannten Einstellungen:

1. **Hitbox:** 2D-Modus, Dicke (fest oder entfernungsabhängig), Deckkraft, Augenlinie (Farbe), Blickrichtungslinie (Länge, Farbe), sich selbst zeigen, Java-Umschaltung per Taste, Reichweite 30 Blöcke.
2. **Keystrokes:** Glow (an/aus getrennt), Rand, Hintergrund-Schatten, Blur, Tastenabstand, Leertasten-Breite/-Höhe, Highlight-Tempo, eigene Texte für W A S D und LMB/RMB, CPS-Text, Text-Versätze.
3. **Gemeinsame HUD-Optik für alle Text-HUDs** (`HudModule`/`TextHud`): Glow, Rand mit Dicke, Hintergrund-Schatten, Blur, Rotation, Padding X/Y, Text-Ausrichtung, Text-Schatten mit Versatz. Das gilt dann für jedes Modul, ohne es einzeln zu bauen.
4. **Modulspezifische Platzhalter** für das Feld "Format": CPS `{lmb}` `{rmb}`, Coordinates `{D}` `{X}` `{Y}` `{Z}`, Reach/Combo/Ping `{value}`.
5. **Coordinates:** vertikaler Modus mit Geschwindigkeit, Dimensionsformate, Koordinaten der anderen Dimension, Taste zum Kopieren.
6. **Tab List:** Spielerköpfe, Plattform-Icons, Weltname, Server-Ping-Symbol, Spieler-Hervorhebungen.
7. **Direction HUD** als Kompassleiste mit Grad-Anzeige und Waypoints.
8. **Debug Menu (F3)** mit allen Blöcken inkl. Frametime-Graph.
9. **Zoom:** Hand ausblenden, Module ausblenden, Cinematic-Balken, "immer animieren". **Cinematic Camera:** Balken. **Auto Perspective:** Schwimmen, Emote. **View Model:** Item-FOV, dritte Person. **Particle Multiplier:** "Normal Hit Crit". **Swing Animations:** Swing Angle, Flux Swing. **Render Options:** Entities, Terrain, Item in Hand, HUD. **Custom Crosshair:** PNG-Import.
10. **Time/Clock:** Spielzeit, Datum-Optionen. **Experience Info:** vier Modi. **Better Hunger Bar:** Vorhersage. **Item Counter:** Textur-Modus, beliebige Items. **Block Break Indicator:** Balken/Text.

## Phase 4: Fehlende Module (komplett neu)

Inventory Lock (nur Werkzeuge, Doppelklick-Droppen 300 ms), Modern Keybind Handling (Bewegung nach Inventar/Pause/Chat), Java Inventory Hotkeys, Item Physics (Speed, Y-Versatz, Rotation behalten, weich), TNT Timer (Format, Nachkommastellen 0 bis 3, Farbschwellen), Nametag Modifier (Text- und Hintergrundfarbe), Nick (Name, fett, verschleiert, Farbe), Skin Stealer (lokal speichern), Subtitles, Movable Title / Bossbar / Hotbar / Day Counter / Coordinates, Light Overlay, Hotbar-Auswahl-Animation, Message Logger (Zeitstempel, "clean"-Datei), Pack Changer (Quality of Life), Chat-Erwähnungs-Ton (`@here`).

## Phase 5: Plattform-Funktionen

1. **Discord Rich Presence** (Named Pipe `\\.\pipe\discord-ipc-0`): Server, Modus, Spielzeit. Abschaltbar.
2. **Lua-Scripting (5.4), gratis:** Quellen des Lua-Interpreters per `git clone` von GitHub (nicht kopieren, einbinden), Skript-Ordner, Hot-Reload, API für Module, HUD-Elemente, Events (Treffer, Chat, Tick). Ein Beispielskript. Dokumentation in `docs/SCRIPTING.md`.
3. **Config teilen:** Profile exportieren und importieren als Code (Grundlage vorhanden).
4. **Marketplace-Vorbereitung:** Skript-Liste von GitHub laden (noch ohne eigenen Server).

## Phase 6: Onix-Funktionen

Aus den Onix-Skripten (öffentlich, `OnixClient-Scripts`) nur das PvP-Relevante: Armor-Anzeige an der Hotbar, Target HUD (vorhanden), Fall-Trajektorie, Pfeil-Spur, Black Bars, Left Hand, Gamemode-Hotkeys, Inventar-Viewer. Eingebaut von Onix: Third Person Nametag, Light Overlay, Theme Editor (vorhanden), Module Search (vorhanden).

## Phase 7: Politur

- Tiers nach neuen Modulen aktualisieren, Kategorien im Menü prüfen.
- Alle Module einmal in `tools/cross.sh shots` mit den Demo-Daten durchklicken, Fehler beheben.
- `docs/MODULES.md` und `docs/FEATURES.md` aktualisieren.

## Berichte

Nach jedem Block eine kurze Zeile in `docs/STATUS.md`: Was ist fertig, was nicht, Modulzahl "nutzbar mit Demo / insgesamt". Nicht übertreiben: gesperrt heißt gesperrt.

## Nicht bauen

FPS- und Ping-Spoof, Reach/Killaura/Aimassist oder irgendetwas, das mehr kann als der normale Client. Keine Spaßmodule mehr.
