# Status

Stand: 2026-10-01 abends. Wird nach jedem Arbeitsschritt aktualisiert.

## Überblick

| Bereich | Stand | Anmerkung |
|---|---|---|
| DLL-Kern, Renderer (DX11/DX12), Crash-Guard, Config | gebaut, unter Wine getestet | DX12 und echtes Minecraft noch nicht getestet |
| Client-Menü | fertig im Look | 4 Seiten, Kategorie-Pillen, Animationen, Englisch/Deutsch |
| Module | 110 registriert von 135 Ziel | 42 laufen ohne Signaturen (Overlay), 68 warten auf Signaturen |
| Spiel-Schnittstelle (SDK) | gebaut von Session B | Demo-Daten laufen, Live-Provider wartet auf Signaturen |
| Launcher | lauffähig | Fenster, Injector, Updater, Auto-Inject, Selbst-Update, Versionsverwaltung über LeviLauncher |
| Release-Pipeline | geschrieben, noch nie gelaufen | GitHub Action baut DLL und Launcher mit MinGW, Prüfsummen |
| Sprachen | fertig für Launcher und Client | B's neue Module noch teils deutsch, Übersetzungs-Durchlauf offen |
| Signaturen | 0 | nur am PC mit Minecraft + Ghidra möglich |
| Test im echten Minecraft | offen | Anleitung in `docs/PC_TEST.md` |
| Lua-Scripting | nicht gebaut | |
| Version-Switcher | über LeviLauncher | eigener Downloader nicht geplant für Release 1 |

## Nächste Schritte

1. Test zuhause nach `docs/PC_TEST.md` (kein Terminal nötig), Log schicken.
2. Signaturen am PC finden (Claude Code), `sigs/<version>.json` committen.
3. Deutsche Texte der neuen Module übersetzen (Durchlauf, wenn Session B fertig ist).
4. Pre-Release `v0.1.0-alpha.1` erzeugen, damit die exe zum Download bereitliegt.

---

## Frühere Einträge

# Status

Stand: 2026-10-01. Diese Datei wird nach jedem Arbeitsschritt aktualisiert.

## Gesamtfortschritt: ca. 15 %

Das hier ist der Anfang, nicht das Ende. Die Basis steht, der große Rest (Module, Signaturen, Launcher, Version-Switcher, Feinschliff) kommt noch.

| Bereich | Stand | Anmerkung |
|---|---|---|
| DLL-Kern (Boot, Hooks, Crash-Guard, Config, Log) | ca. 85 % | läuft unter Wine im Testhost, Laden/Entladen (Strg+L) ok |
| Renderer (DX11 + DX12 via D3D11On12, ImGui) | ca. 75 % | DX11 live getestet, DX12 nur Code, muss im echten MC geprüft werden |
| Menü/UI (RShift, Sidebar, Karten, Panel, Themes, Profile, Info, HUD-Editor, Toasts) | ca. 70 % | alle Seiten getestet, Onix-Scroll-Feeling fehlt noch |
| Input (Raw Input, CPS, Hotkeys, Latenz-Stack) | ca. 60 % | Grundlagen da, Messung im echten Spiel + Feintuning fehlen |
| Server-Regeln (Erkennung, Warnen/Sperren, servers.json) | ca. 60 % | Erkennung über Netzwerk-Hooks, muss mit echtem Zeqa/Hive getestet werden |
| Signatur-/Versionssystem | ca. 50 % | Scanner, Cache, Vererbung fertig, aber noch KEINE echten Signaturen |
| Module | 138 von 135 (Code vollständig, nicht im Spiel getestet) | Liste und Signatur-Bedarf in `docs/MODULES.md` |
| Spiel-Module (Zoom, Freelook, Fullbright, Hitcolor ...) | Code fertig, 0 % getestet | laufen mit Demo-Daten, echte Signaturen und Live-Leser siehe `docs/SDK.md` |
| Launcher + Injector | ca. 10 % | nur Skelett, nicht lauffähig |
| Version-Switcher | 0 % | |
| Auto-Update | 20 % | DLL kann Signaturen nachladen, Launcher-Update fehlt |
| Lua-Scripting, Server-Profile, Feinschliff | 0 % | |

## Was geht jetzt schon (getestet in Wine/Testhost, nicht im echten Minecraft)

- DLL lädt sich, hookt Present/ResizeBuffers, zeichnet Overlay, entlädt sich sauber mit Strg+L.
- Menü mit Rechts-Shift: Öffnen-Animation, Kartenraster, Suche, Kategorien, Einstellungs-Panel pro Modul, Themes (5 Vorlagen, eigene Farben, Export/Import-Code), Profile, Info-Seite, Server-Chip.
- HUD-Editor: Ziehen, Snappen, Größe per Mausrad.
- 20 Module: FPS, CPS, Keystrokes, Mouse Strokes, Clock, Stopwatch, Session Timer, Memory, Latency, Server Display, Crosshair, CPS Limiter, No Scroll, Low Latency, System Boost, Snake, Flappy, DVD Screen, Eye Break, Menü-Modul.
- Erster Start: FPS, CPS, Keystrokes, Server Display an, Willkommens-Toast.

## Was noch nicht geht

- Im echten Minecraft getestet wurde noch nichts (hier gibt es kein Minecraft). Erster echter Test = Felix am PC mit dem Dev-Injector.
- Keine Spiel-Signaturen, deshalb keine Module, die ins Spiel eingreifen.
- Launcher, Injector, Version-Switcher, Auto-Update des Clients.

## Nächste Schritte (Reihenfolge)

1. Felix testet die DLL im echten Spiel (Claude Code am PC baut mit MSVC, injiziert, liest latest.log).
2. Claude Code am PC findet die ersten Signaturen (Zoom, Freelook, Fullbright, Hitcolor, Skin Stealer ...) und schreibt sigs/<version>.json.
3. UI-Feinschliff nach Onix-Vorbild, siehe docs/RESEARCH.md.
4. Module ausbauen: pro Modul mindestens so viele Optionen wie Flarial/Onix plus Animationen.
5. Launcher + Injector + Version-Switcher + Auto-Update.
6. Input messen und optimieren (Latency-HUD liefert die Zahlen).

## Bekannte Fehler / offene Punkte

- Keine offenen UI-Fehler aus dem Wine-Test. Der Seitenwechsel wirkte kaputt, weil die Test-Klicks 1 px daneben lagen, nicht wegen des Codes.
- Einstellungs-Panel war hinter den Karten gezeichnet: behoben, ist jetzt ein eigenes Fenster.
- DX12-Pfad ungetestet.
- Screenshots aus Wine haben schlechte Auflösung, Schärfe/Schrift im echten Spiel prüfen.

## Update Session 2

- Launcher lauffähig: Fenster, Injector, Spielstart, Updater (GitHub Releases mit Prüfsumme), Auto-Inject, Selbst-Update. Als exe gebaut (MinGW) und unter Wine getestet: Injection in einen Fake-Minecraft-Prozess klappt. Noch nicht im echten Minecraft getestet (AUMID und Paketname für den Spielstart müssen am PC geprüft werden).
- Sprachen Englisch/Deutsch in Launcher und Client, Standard Englisch, Auto folgt Windows. Umschalten im Launcher unter Einstellungen und im Client-Menü unter Info.
- Bauen und Testen in der Cloud über `tools/cross.sh`.

## Session B (Module), Block 1: Latenz, Netzwerk, Limiter

- Neu: Network (Ping/Jitter/Verlust, WLAN/LAN-Erkennung, Ampel mit Grund und Tipps, Scan-Spitzen), Ping Counter, Latency Blame, Frame Limiter, Instant Input (SR). Latency-HUD um 1%-Low, Spitze und Overlay-Kosten erweitert.
- `perf/Tuning` sammelt die Wünsche von LowLatency, Frame Limiter und Instant Input und schreibt `dx::tuning()` einmal pro Frame.
- Alles mit MinGW syntaxgeprüft, nicht im Spiel getestet. Stand der Modulzahl steht in der Zeile von Block 2.

## Session B (Module), Block 2: Post-Effekte, Crosshair, Screenshot, Spiele

- Neu: ein gemeinsamer Post-Shader (`modules/post/PostFx`) für Saturation/Hue, Brightness/Contrast, Screen Tint, Sharpen, Depth of Field, Color Filter, Night Shift, Motion Blur, Deepfry, Upside Down. Läuft als ImGui-Draw-Callback vor dem HUD, nur wenn ein Effekt aktiv ist.
- Custom Crosshair ausgebaut: 10 Formen, Pixel-Editor (15x15) mit Import/Export-Code, Dynamik, Drehung, Regenbogen.
- Screenshot+ (mit/ohne Mochi-HUD, PNG/JPEG über WIC, Zwischenablage), Block Game, Pomodoro.
- Module 38 von 135.

## Session B (Module), Block 3 und 4a: SDK und Kampf-/HUD-Module

- SDK (`dll/src/sdk`, Beschreibung in `docs/SDK.md`): Spielzustand, Ereignisse, Demo-Provider, Live-Provider (feste Offsets), Effekt-Kanäle mit Hooks/Patches, Tasten-Injection. Schalter "Demo-Daten" im Modul "Sig Status".
- Kampf: Reach Counter, Opponent Reach, Combo Counter, Hit Counter, Hit Ping, Session Stats, Hit Info, Entity Counter, Damage Indicator, Hit Marker, Hit Effects, Kill Effects, Hit Sound, Totem Pop, Target HUD, Waila, Bow Charge, Cooldown Indicator, Hitbox, Hurt Color, Glint Color, Low Fire, Particle Multiplier, Sens Multiplier, Bow Sensitivity, Snap Look, Null Movement (SR), Item Use Delay Fix (SR), Faster Inventory (SR), Insta Hurt Animation (SR).
- HUD: Coordinates, Direction HUD, Speed Display, Look Angles, Health Display, Experience Info, Day Counter, IP Display, Pack Display, Held Item, Break Progress, Armor HUD, Potion HUD, Pot/Arrow/Totem/Item Counter, Durability Warning, Low Health Indicator, Better Hunger Bar.
- Module 89 von 135.

## Session B (Module), Block 4b: Kamera und Welt

- Kamera: FOV Changer, Java Dynamic FOV, Zoom (mit Scroll-Stufen), Freelook (Hive gesperrt), No View Bobbing (Kamera/Hand getrennt, nur Sprint/Wasser), Minimal View Bobbing, No Hurt Cam, Smooth Sneak, Auto Perspective, Fullbright.
- Welt: Block Outline (eigene Linien und Füllung über Kameraprojektion), Time/Weather/Environment Changer, Fog Color, Water Color, Chunk Border, Waypoints, Hide Hand, View Model, Animations.
- Module 110 von 135.

## Session B (Module), Block 5: Komfort, Performance, Server, Spaß

- Komfort: Toggle Sprint/Sneak (per Tasten-Injection, ohne Signaturen), Command/Text Hotkey, Profile Hotkeys, Auto GG, Message Logger, Chat Plus, Death Logger, Player Notifier, Scoreboard, Tab List, Mumble Link, GUI Scale, Streamer Mode.
- Performance: Render Options, Auto Profile, Background Load, Stats HUD, Watermark, Debug Menu.
- Server: Server Profiles, Match Summary. Spaß: Pet, Petals.
- Custom Crosshair: Original ausblenden, Ausblenden in Inventar/Dritter Person, Farbe beim Anvisieren.
- servers.json um Snap Look, Waypoints, Hotkey-Module, Damage Indicator, Target HUD ergänzt.
- Module 135 von 135. Das Ziel ist erreicht, der Rest ist Test am PC: Signaturen finden, Live-Leser für Inventar/Effekte/Ziel/Chat, Demo-Daten gegen echtes Spiel vergleichen.

### Was am PC zuerst zu tun ist (Reihenfolge)

1. Bauen, Compile-Fehler beheben (MSVC kann strenger sein als MinGW), Overlay prüfen. In "Sig Status" Demo-Daten einschalten und alle HUDs ansehen.
2. Post-Effekte testen (ein Shader für alle, `modules/post/PostFx.cpp`): DX12 und DX11. Screenshot+ testen.
3. Signaturen: `LocalPlayer`, `Level`, `AttackEntity` (schaltet Kampf-Module frei), dann `fx.fov`, `fx.gamma`, `fx.viewBob`, `fx.hurtCam`, `fx.hitbox` (Zoom, FOV, Fullbright, No View Bobbing, No Hurt Cam, Hitbox). Alles Weitere steht in `docs/SDK.md`.
4. Toggle Sprint/Sneak, Null Movement und Chat-Hotkeys im Spiel prüfen (SendInput mit Scancode).
5. Netzwerk-Modul mit WLAN gegen die Windows-Anzeige vergleichen.

- Nachtrag: Cinematic Camera (Blickbewegung glätten über `fx.lookDelta`), Paperdoll (2D-Figur mit Rüstung). Target HUD und Waila zeigen Leben von Spielern nur, wenn man es ausdrücklich einschaltet. Module jetzt 137.
- Nachtrag: Hotbar Keys (Tasten für Hotbar-Plätze, per Injection), Zoom mit dunklem Rand, Coordinates kopiert die Position per Taste. Module jetzt 138.
- Phase 0 (Teil 1): alle Texte der Module, des SDK und der Server-Hilfen sind jetzt Englisch im Code, die deutsche Tabelle liegt in `dll/src/modules/Lang_B.cpp` (881 Einträge). Laufzeit-Texte gehen über `i18n::tr` und `i18n::fmt`. `tools/cross.sh build` und `shots` laufen sauber, auf Deutsch (lang.txt im Wine-Profil) sind Karten und Menü übersetzt.
- Phase 0 (Teil 2): Zähler an das Original angeglichen. Combo Counter (480-ms-Regel, Reset nach 15 s, "Count to negatives"), Reach Counter (2 Nachkommastellen, Reset nach 15 s), Hit Ping (misst Angriff bis Server-Bestätigung über neues Ereignis `Confirm`, braucht die Marker-Signatur `ActorEvent`), Opponent Reach (nächster Spieler im 10-Block-Radius, gültig bis 5,5 Blöcke, Team ausschließen, braucht `ActorList`), Insta Hurt Animation ("Exclude team", "Only against full armor"), Potion HUD (Bottom up, römische Ziffern über V, rot bei 5 s oder weniger), Arrow und Totem Counter ("Only when in hand"), Pot Counter (alle Splash-Tränke in den Slots 0 bis 35). Neu im SDK: Bereich `Others` (Spieler in der Nähe), Ziel mit Team und Rüstung. Mit Demo-Daten per Screenshot geprüft: diese Zähler, Potion HUD und die Item-Zähler; die Durchsicht aller 140 Module folgt in Phase 7. Im echten Spiel brauchen Opponent Reach und Hit Ping neue Signaturen (siehe `docs/SDK.md`), ohne sie bleiben die Module grau.
- Phase 1 (Teil 1): **Hive Utils** (Auto-Requeue mit `/q`, `/hub` oder eigenem Befehl, nur Solo, bei Team-Ausscheiden, Requeue-Taste, Map Avoider pro Spiel und Modus, Rollen-Requeue für Murder Mystery, Hide and Seek und Death Run mit Todes-Limit, Custom-Server-Code kopieren, Chat aufräumen, Auto-Accept für Freunde und Partys, Map-Vote-Hilfe mit Ansage) und **Zeqa Utils** (Duell-Requeue Ranked/Unranked, Chat aufräumen, Duell- und Freundesanfragen annehmen). Beide laufen nur auf ihrem Server. Das SDK hat dafür einen Chat-Filter (`game::filterChat`), einen Sende-Verlauf (`inject::sent`) und einen "Demo server" in Game Support (The Hive oder Zeqa mit Skript-Chat). Mit Demo-Daten per Log geprüft (`demo: would send ...`). Die Chat-Texte sind Annahmen und stehen als bearbeitbare Wortlisten in den Einstellungen; echte Hive- und Zeqa-Zeilen müssen am PC mit dem Message Logger gesammelt und die Standardwörter danach angepasst werden. Serverregeln für beide Seiten konnte ich nicht prüfen (Seite von der Cloud-Maschine gesperrt), deshalb steht beides in `warn`. Module jetzt 142. Mit Demo geprüft sind nur Hive Utils und Zeqa Utils, die Durchsicht aller Module folgt in Phase 7.
- Phase 1 (Teil 2): **Hive Stats** (Overlay mit Spielern aus der Tab-Liste, Werte aus der öffentlichen Hive-API über WinHTTP mit Cache und Rate-Limit: Level, K/D, Siegquote, FKDR, Siege, Niederlagen, Kills, Final Kills, Tode, Spiele, Serie, erstes Spiel, je Spiel Haupt- und Nebenwert, Farbschwellen, Team-Farben, Hervorhebung, neun Anker) und **Hive Leaderboard** (Top-Liste gesamt oder monatlich, 1 bis 100 Zeilen, Refresh 5 bis 300 s). `HudModule` hat jetzt einen Pivot (neun Anker). Auto GG erkennt das Spielende pro Server (Hive, Zeqa, CubeCraft, Lifeboat, Galaxite, Mineville, NetherGames). NetherGames und Mineville stehen in `servers.json` (nur `warn`, Regelseiten teils nicht erreichbar). Die Hive-API konnte ich von der Cloud-Maschine nicht aufrufen: Endpunkte und Feldnamen (`/v0/game/all/<spiel>/<name>`, `victories`, `final_kills` usw.) sind aus dem Gedächtnis und müssen am PC gegen eine echte Antwort geprüft werden. Mit Demo-Daten läuft die Anzeige (erfundene Werte, kein Netzwerk). Module jetzt 144.
- Phase 2: **Crystal PvP** und **Hit feedback** als eigene Menü-Abschnitte. Crystal Optimizer behandelt einen getroffenen Crystal lokal sofort als zerstört (Geisterliste im SDK mit den Kanälen `fx.ghostRender` und `fx.ghostPick`, wartet nicht auf den Server, sendet nichts), jede Funktion einzeln und nur sichtbar, wenn ihr Kanal da ist. Neu: **Kill Cleanup** (getötete Spieler sofort ausblenden und aus dem Zielen nehmen, auch laut Chat). Particle Multiplier hat "Critical hit particles on every hit" (`fx.critParticle`). Insta Hurt Animation, Block Hit und Instant Hit sind in den beiden Abschnitten einsortiert. Alle mit Warnhinweis, Server-Regeln: Hive und CubeCraft warnen vor Kill Cleanup, Lifeboat und Galaxite über die Tags. Mit Demo-Daten sieht man die Geisterliste mitzählen (Screenshot geprüft); die echte Wirkung braucht am PC die Signaturen `fx.ghostRender`, `fx.ghostPick`, `fx.critParticle`, die Offsets `actor.typeId` und `type.crystal` sowie die Marker `KillEvents`, `ActorList`. Gemessen wurde nichts, also keine Zahlen versprechen. Module jetzt 145.
- Phase 3 (Teil 1): **Hitbox** mit allen Einstellungen (Boxform 3D oder flach, Deckkraft, Dicke, Reichweite bis 30, eigene Hitbox, Augenlinie, Blickrichtungslinie mit Länge und Farben, Java-Größe mit Taste), alles über den Zeichenweg des Spiels (nie durch Wände, deshalb keine eigene 2D-Überlagerung und keine entfernungsabhängige Dicke), neue Kanäle `fx.hitbox*`. **Gemeinsame HUD-Optik** für jedes HUD-Modul: Padding X/Y, Text-Schattenversatz, Ausrichtung mit Mindestbreite, Rand, Glow, Kastenschatten, echter Hintergrund-Blur (neuer Pass in PostFx, per Screenshot geprüft), Rotation, Positionsklemmung. **Keystrokes** mit Tastenabstand, Leertasten-Breite und -Höhe, Press- und Release-Tempo, eigenen Texten für WASD, LMB, RMB und Leertaste, CPS-Format, Text-Versatz, Glow (ruhend und gedrückt getrennt), Tastenrand und Tastenschatten. Platzhalter: CPS `{lmb}` `{rmb}`, Coordinates `{D}` `{X}` `{Y}` `{Z}`, alle Text-HUDs `{label}` `{value}`. **Coordinates** mit Format, Dimensionsnamen, vertikaler Geschwindigkeit (+/-), Koordinaten der anderen Dimension und Kopierformat. Mit Demo-Daten per Screenshot geprüft, Hitbox wirkt erst mit den Signaturen. Module jetzt 145.
- Phase 3 (Teil 2): **Tab List** (Spielerköpfe aus 8x8-Gesicht oder Initiale, Plattform-Symbole für PC, Handy und Konsole, Kopfzeile mit Weltname, Spielerzahl und Server-Ping mit Farbpunkt, Hervorhebungen, Zeilenabstand, Umschalttaste), **Direction HUD** als Kompassleiste (Pixel pro Grad, Ausblenden am Rand, Zwischenrichtungen, Grad-Anzeige, Waypoints mit Name und Entfernung aus dem Waypoints-Modul), **Debug Menu** (F3) mit Blöcken für Leistung (inklusive 1% Low), Position, Spieler, Welt, Ziel, System (GPU, CPU, RAM), Server (IP, Ping, TPS), Zeit und Mochi sowie Frametime-Graph mit 30- und 60-FPS-Linien, **Zoom** (Hand und Module ausblenden, Kino-Balken, immer animieren), **Cinematic Camera** (Balken), **Auto Perspective** (Schwimmen, Emote), **View Model** (Item-FOV, dritte Person), **Animations** (Schwungwinkel, fließender Schwung), **Render Options** (Entities, Gelände, Item in der Hand, HUD), **Custom Crosshair** (PNG-Import mit Tönung), **Clock** (Echtzeit, Spielzeit oder beides, Datumsformate, Wochentag), **Experience Info** (vier Modi), **Better Hunger Bar** (Sättigungs-Vorschau, Verschwendungswarnung), **Item Counter** (beliebig viele Items, Format, Sortierung, farbige Symbole), **Break Progress** (Balken, Ring oder Text). Nicht möglich: Item-Texturen im Item Counter (keine Spieldateien im Client) und Sound-Zähler im Debug Menu. Mit Demo-Daten per Screenshot geprüft: Tab List, Direction HUD, Debug Menu, Crosshair-PNG, Clock, Experience Info, Hunger Bar, Item Counter; die Spiel-Kanäle (Zoom-Hand, Render Options, View Model, Swing) brauchen Signaturen am PC. Module jetzt 145.
- Phase 4: fehlende Module gebaut. **Inventory Lock** (Werkzeuge und Rüstung per Doppeldruck der Wegwerf-Taste, 300 ms einstellbar, feste Slots), **Modern Keybind Handling** (Bewegungstasten nach Inventar, Chat oder Menü wiederherstellen), **Java Inventory Hotkeys** (liest die Hotbar-Tasten aus `options.txt`), **Item Physics**, **Nametag Modifier**, **TNT Timer** (nur für das TNT am Fadenkreuz, damit nichts durch Wände sichtbar wird), **Nick** (nur in Mochis eigenen Anzeigen, 25 Farben, fett, verschleiert), **Skin Stealer** (speichert den Skin lokal als PNG, ändert nichts im Spiel), **Subtitles** (Richtungspfeile, Lebensdauer bis 5 s, 1 bis 20 Zeilen, neun Anker), **Movable Hotbar, Title und Bossbar** (verschieben die echten Spielelemente über Versatz-Kanäle), **Light Overlay** (Felder mit Spawn-Licht, Radius bis 16), **Hotbar Animation**, **Pack Changer** (aktive Paketliste schreiben), Message Logger mit "clean"-Datei, Better Chat mit Erwähnungs-Ton (`@here`), Coordinates und Day Counter können das Original ausblenden. Mit Demo-Daten geprüft: Subtitles, Light Overlay, TNT Timer, Hotbar Animation. Alles andere braucht Signaturen am PC (neue Kanäle `fx.itemPhysicsData`, `fx.nametag*`, `fx.*Offset`, `fx.hide*`, Marker `SoundEvents`, `LightLevels`, `TargetFuse`, `TargetSkin`). Module jetzt 160.
- Phase 5: **Discord Rich Presence** (Named Pipe `discord-ipc-0` bis 9, Handshake und `SET_ACTIVITY`, gegen einen Fake-Server unter Wine geprüft; Server, Modus, Spielzeit und Texte einstellbar, "nur Minecraft anzeigen" als Datenschutz; braucht eine eigene Discord-Anwendungs-ID, siehe HANDOFF), **Lua Scripts** (Lua 5.4.8 per CMake `FetchContent` von GitHub, ein eigener Zustand je Skript, Sandbox ohne `io`, `package`, `debug`, `load`, 24 MB und 3 Mio. Anweisungen pro Aufruf, Neuladen beim Speichern, Ereignisse für Treffer, Schaden, Kill, Tod, Chat, Sound, Taste, Server und Tick, HUD-Text und -Balken, Speicher für Werte, `docs/SCRIPTING.md`, drei Beispielskripte in `scripts/`; unter Wine geprüft: HUD, Endlosschleife wird gestoppt, `io` ist gesperrt, Syntaxfehler werden gemeldet), **Config Sharing** (Code `MCS1-...` aus den eingeschalteten Modulen und dem Theme, Tasten und Texte nur auf Wunsch, Vorschau und Bestätigung beim Laden) und die **Skript-Liste** (`scripts/index.json` aus dem Repository, Installieren per Knopf). Module jetzt 163.
- Phase 6: **Hotbar Armor** (vier Rüstungsteile neben der Hotbar, Balken, Prozent, Warnung), **Fall Predictor** (Falldistanz, Schaden, "tödlich" mit Absorption; ohne Weltdaten kann er die Landestelle nicht zeichnen, darum nur die Schadensvorhersage), **Inventory Viewer** (Raster mit Mengen, Haltbarkeit, gewähltem Slot), **Arrow Trail** (Spur hinter Pfeilen, Enderperlen und Dreizacken, standardmäßig nur eigene Geschosse), **Black Bars** (schwarze Balken mit Animation), **Left Hand** (Hand gespiegelt nach links über `fx.handMatrix`, muss im Spiel eingestellt werden), **Gamemode Hotkeys** (tippt `/gamemode ...` per Taste), **Third Person Nametag** (`fx.selfNametag`). Theme Editor, Modulsuche, Target HUD und Light Overlay gab es schon. Mit Demo-Daten per Screenshot geprüft: Inventory Viewer, Fall Predictor, Arrow Trail, Black Bars; Hotbar Armor, Left Hand und Third Person Nametag brauchen die Signaturen am PC. Module jetzt 171.
- Phase 7: Politur. Deutsche Reste im Code ersetzt (Armor-Kürzel, GPU-Standardtext), Hive Stats und Hive Leaderboard liegen in der Kategorie Server, `docs/MODULES.md` (Umsetzungsstand) wird jetzt mit `tools/modules_doc.py` aus einem Modul-Dump erzeugt (`MOCHI_DUMP_MODULES`), `docs/FEATURES.md` hat einen Abschnitt zum Stand, `tools/cross.sh tour` schaltet alle Module mit Demo-Daten ein und prüft das Log (keine Fehler), `docs/PARITY.md` hat einen Nachtrag. Stand: 171 Module, 67 ohne Demo und ohne Signaturen nutzbar, 104 warten auf Signaturen, mit Demo-Daten laufen alle 171 unter Wine. Im echten Spiel getestet ist nichts. Die Texte sind Englisch mit deutscher Tabelle (`dll/src/modules/Lang_B.cpp`, 1491 Einträge).
- Phase 1 (Teil 1): **Hive Utils** (Auto-Requeue mit `/q`, `/hub` oder eigenem Befehl, nur Solo, bei Team-Ausscheiden, Requeue-Taste, Map Avoider pro Spiel und Modus, Rollen-Requeue für Murder Mystery, Hide and Seek und Death Run mit Todes-Limit, Custom-Server-Code kopieren, Chat aufräumen, Auto-Accept für Freunde und Partys, Map-Vote-Hilfe mit Ansage) und **Zeqa Utils** (Duell-Requeue Ranked/Unranked, Chat aufräumen, Duell- und Freundesanfragen annehmen). Beide laufen nur auf ihrem Server. Das SDK hat dafür einen Chat-Filter (`game::filterChat`), einen Sende-Verlauf (`inject::sent`) und einen "Demo server" in Game Support (The Hive oder Zeqa mit Skript-Chat). Mit Demo-Daten per Log geprüft (`demo: would send ...`). Die Chat-Texte sind Annahmen und stehen als bearbeitbare Wortlisten in den Einstellungen; echte Hive- und Zeqa-Zeilen müssen am PC mit dem Message Logger gesammelt und die Standardwörter danach angepasst werden. Serverregeln für beide Seiten konnte ich nicht prüfen (Seite von der Cloud-Maschine gesperrt), deshalb steht beides in `warn`. Module jetzt 142, nutzbar mit Demo: 142 (Hive und Zeqa nur im passenden Demo-Server).

## Session A, Block: Client-Einstellungen, HUD-Editor, Self-Test

- **Self-Test** (`MOCHI_SELFTEST=1`, `modules/SelfTest.cpp`): schaltet jedes Modul mit Demo-Daten nacheinander ein, ändert zufällig Einstellungen, simuliert Tasten und Maus, meldet Module, die sich durch einen Fehler selbst abschalten, und listet mit `untranslated [Modul]: Text` alle Texte ohne deutschen Eintrag. Lief vor dem Merge von modules-b mit 138 Modulen ohne Fehler.
- **ClientSettings** (Settings-Seite, Abschnitt "Client tag"): Tag "Mochi <3" hinter dem eigenen Namen im Better Chat und in der Tab-Liste (Text, Farbe, Klammern, Position), Benachrichtigungen an/aus. Nur lokal, es wird nichts gesendet. Das Tag nutzt einen internen Farbcode `§#RRGGBB;`, den `text::colored` und `text::strip` kennen.
- **HUD-Editor**: Einrasten an Bildschirmrändern, Mitte und den Rändern und Mitten aller anderen Elemente (mit Hilfslinien), Pfeiltasten verschieben das gewählte Element (Shift = 10 px), Doppelklick setzt Position und Größe zurück, bei Überlappung wird das kleinere Element gegriffen, Name und Größe stehen am gewählten Element. Shift beim Ziehen schaltet das Einrasten ab.
- Modulpanel: Halten-Modus/Taste-Karten schneiden zu lange Texte ab, die Fußzeile bricht um (auf Deutsch lief "HUD bearbeiten" aus dem Panel). Untergruppe "Eigene Werte" heißt jetzt "Info displays" / "Info-Anzeigen".
- **Echter Fehler durch den Self-Test gefunden und behoben:** Der Netzwerk-Monitor hielt eine Sperre, während er bei nicht auflösbarer Server-Adresse 2 Sekunden wartete. Der Render-Thread blieb daran hängen, das Spiel hätte bei einem Server ohne DNS-Antwort alle zwei Sekunden eingefroren. Der Self-Test meldet jetzt außerdem Frame-Aussetzer über 350 ms (`selftest STALL`), speichert die Konfiguration nicht und kann mit `MOCHI_SELFTEST_ONLY="Modul A,Modul B"` einzelne Module prüfen.
- **Performance Lock** (neues Modul, Kategorie Performance): ein Schalter, der System Boost, Low Latency und Frame Limiter einschaltet und Post-Effekte und Extras pausiert. Der alte Zustand wird gemerkt und beim Ausschalten wiederhergestellt.
- README.md angelegt, CLAUDE.md-Hinweis zum Launcher korrigiert (C++, nicht C#).
- **Session B, nach dem Merge von main:** 173 Module registriert (171 aus Session B plus Client Settings und Performance Lock aus main). Der Self-Test unter Wine läuft sauber durch: 171 Module geprüft, 0 Fehler, 0 Aussetzer, 0 unübersetzte Texte (17 fehlende deutsche Einträge ergänzt, vor allem Datumsformate und Hive-Spielnamen). `tools/cross.sh build` und `shots` ohne Fehler im Log. `docs/MODULES.md` neu aus dem Modul-Dump erzeugt. Weiterhin gilt: nichts davon wurde im echten Spiel getestet, und die Hive-/Zeqa-Chattexte und API-Felder sind Annahmen.
