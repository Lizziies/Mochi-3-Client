# Status

Stand: 2026-10-02 abends. Wird nach jedem Arbeitsschritt aktualisiert. Die Arbeitsliste für zuhause steht in `docs/HOME_TODO.md`.

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
| Signaturen | 1.26.52: LocalPlayer, Gamma, FOV, Item-FOV, Perspektive, View Bobbing, Hide Hand, Wolken, Hurt Cam | `sigs/1.26.52.json`, 96 von 179 Modulen frei (vorher 75) |
| Test im echten Minecraft | läuft | DX12 + RTSS, Strg+L, Koordinaten/Tempo/Blickwinkel live geprüft |
| Lua-Scripting | nicht gebaut | |
| Version-Switcher | über LeviLauncher | eigener Downloader nicht geplant für Release 1 |

## Nächste Schritte

Alles Weitere geht nur am PC mit dem echten Spiel und steht in `docs/HOME_TODO.md`: bauen mit MSVC, Mochi Online veröffentlichen und Owner setzen, Test im Spiel, Signaturen bis kein Modul mehr grau ist, GUI-Scale-Hook, Latenz messen gegen Flarial, Cosmetics im Spiel, zum Schluss die Exe.

## Signaturen und freigeschaltete Module (1.26.52, Stand 2026-10-03)

Ohne Spieldaten: 75 Module. Pro Datenquelle dazugekommen:

| Quelle | Weg | Module frei | Summe |
|---|---|---|---|
| LocalPlayer + HitResult (Position, Blick, Ziel-Typ, Treffer) | Signatur + player+0x1e8 | +28 | 103 |
| Spieloptionen (Gamma, FOV, Perspektive, Bobbing, Hand, Wolken, Fadenkreuz) | Getter-Muster + Options-Umleitung | (in den 103 enthalten) | |
| Inventory (Hotbar, Rüstung, Offhand, Slot) | Speichersuche pro Welt | +6 | 109 |
| PlayerStats (Leben, Hunger, XP) | Attribut-Array | +3 | 112 |
| MoveState (Tempo, Sprinten, Boden, Schleichen) | aus Bewegung abgeleitet | +3 | 115 |
| WorldTime | player+0x90, +0x340 | +1 | 116 |
| ChatEvents | ClientInstance+0x650, Vektor +0x150 | +4 | 120 |
| HurtEvents, TotemEvents | aus Leben und Totem-Zahl abgeleitet | +4 | 124 |
| UseState, ItemUseEvents (Bogen, Werfen) | Rechtsklick + gehaltenes Item | +3 | 127 |
| Session Stats ohne AttackEntity | Treffer aus dem HitResult | +1 | 128 |

AttackEntity: nicht gesucht, die Treffer kommen schon aus dem HitResult; ein Hook bringt nur den Schadenswert, den das Spiel dort ohnehin nicht kennt.
Noch grau (51): Target-Infos (Name/Leben des Ziels: Target HUD, Waila, Break Progress, TNT Timer, Skin Stealer, Damage Indicator, Kill Effects), Tabliste/Scoreboard (Tab List, Player Notifier, Scoreboard, Hive Utils, Hive Stats), Effekte (Potion HUD), Entity-Liste (Opponent Reach, Entity Counter, Health Above Head, Arrow Trail, Hit Ping), Render-Hooks (Freelook, Cinematic, Bobbing-Stärke, Smooth Sneak, Block Outline, Time/Weather/Environment, Fog/Water Color, Animations, Hitbox, Hurt/Glint Color, Low Fire, Particles, Snap Look, Block Hit, Crystal Optimizer, Kill Cleanup, Item Use Delay, Faster Inventory, Insta Hurt, GUI Scale, Item Physics, Nametags, Light Overlay, Subtitles, Movable Hotbar/Title/Bossbar, Left Hand).

Werkzeuge: `tools/reload.ps1` baut, entlädt mit Strg+L und injiziert neu, ohne das Spiel neu zu starten. Der Probe-Befehl ist der Dev-Explorer (`explore <skript>`, Lua in `tools/explore/`).

## 2026-10-02 nachmittags, PC, Branch claude/pc-test

- Erste echte Signatur: `LocalPlayer` (globaler Zeiger, Muster in `sigs/1.26.52.json`). Position, Pitch und Yaw liegen zusammen bei player>0x138>0x990 (+0x0 Position auf Augenhöhe, +0xc Pitch, +0x10 Yaw). Die Kopie über den ClientInstance-Pfad (player>0x28>0x258>0x5e0) wird nur alle paar Sekunden aktualisiert, nicht nehmen.
- Schleichen/Sprinten: noch nicht gefunden. Der erste Kandidat player>0x1a0 +0xc war Zufall (das Byte springt periodisch zwischen 3, 4 und 5, unabhängig von der Eingabe), wieder entfernt. Offset-Formate: `<feld>.viaN` für Zeigerpfade, `<feld>.is` für Enum-Bytes.
- HitResult (Fadenkreuz-Ziel): bei [[player+0x28]+0x258]+0x1f0, +0x18 mit Entities, +0x128 nur Blöcke. Aufbau: Start (3 float), Strahl (3 float), Typ (0 Block, 1 Entity, 3 nichts), Seite, Block (3 int), Trefferpunkt (3 float), Entity-Referenz (+0x38), Entity-Hitbox (min/max). Noch nicht im Live-Leser, weil Waila/Break Progress zusätzlich Blockname und Abbaufortschritt brauchen.
- Weltzeit: Weltalter-Tickzähler gefunden (mehrere Kopien), Tageszeit-Getter offen.
- Geschwindigkeit wird aus der Positionsänderung abgeleitet, solange kein velX-Offset gefunden ist.
- Im Spiel geprüft: XYZ stimmt mit der Spielanzeige überein, Tempo zeigt beim Laufen einen Wert, Pitch/Yaw stimmen.
- Offen bei den Spielerfeldern: Leben (die Testwelt hat Cheats aus, braucht eine Überlebenswelt mit Cheats für `/damage`), Hunger, onGround (kein Flag in Tiefe 1 gefunden), Dimension, Level, AttackEntity.
- Spieloptionen: Options-Objekt bei player>0x778>0xb8, Array ab +0x10, Index = Options-ID (gfx_gamma 50, gfx_field_of_view 47, game_thirdperson 3, gfx_viewbobbing 38, gfx_hidehand 422, gfx_toggleclouds 394, gfx_damagebobbing 39). Float-Optionen: +0x10 min, +0x14 max, +0x18 Wert. Jeder Getter ist eine kleine Funktion `mov r8d, <id>; call; ... movss/movzx/mov [rax+...]`, darum ein gemeinsames Muster mit eingesetzter ID.
- Options-Umleitung: Jeder Lesezugriff läuft option → info (+0x8) → +0x238 weiter, solange dort ein Zeiger steht, und nimmt den Wert der letzten Option. fx-Art `Option` (Kind 8) zeigt die Umleitung auf eine Kopie mit eigenem Wert. Wirkt für alle Leser (auch das Welt-FOV, das inline gelesen wird), ohne Code-Patch, und landet nicht in der options.txt (geprüft). Die Einstellungsseite zeigt solange den überschriebenen Wert an.
- Maus-Empfindlichkeit: `ctrl_sensitivity2` (387) ist nicht die Maus, die Maus nutzt `ctrl_sensitivity2_mouse` (eigenes Objekt, noch nicht gefunden). Bis dahin skaliert GameInput, das jetzt richtig rechnet.
- Werkzeuge in `tools/explore/` (nur Dev-Build): watch.lua (Hardware-Watchpoint, zeigt jede Codestelle, die einen Wert liest), optmap.lua (alle Optionen mit ID und Getter), sigcheck.lua (Muster eindeutig?), paths.lua (Zeigerpfade zu Zielwerten), heappos.lua (Heap-Suche plus Bewegungsvergleich), flags.lua (Bytes, die sich beim Halten einer Taste ändern), live.lua (Werte mitschreiben).

## 2026-10-02 abends, Branch claude/onix-ui-input-fixes (Cloud)

- **Akzentfarben:** 8 Farben (Blau, Cyan, Grün, Lila, Pink, Rot, Orange, Grau) unter Global Settings → Appearance. Presets behalten die gewählte Farbe. Die alten Pink-Themes laden als Slate.
- **Launcher im neuen Look:** grau wie das Menü, flache Knöpfe, blaues Pixel-Herz im Logo und als Icon, dieselben 8 Akzentfarben unter Settings (startet mit der Farbe aus dem Client).
- **Owner-Abzeichen:** Mochi Online kennt Rollen (`owner`, `staff`). `vlisya` bekommt per Admin-Aufruf die Rolle, alle Mochi-Nutzer sehen dann `[Owner]` hinter dem Namen in Tab-Liste und Chat. Nur für ihn, ein neu geclaimter Name verliert die Rolle. Der neue Dienst-Code ist gebaut und getestet (19 Tests), aber noch nicht veröffentlicht (zuhause, `HOME_TODO.md` Punkt 2).
- **Einstellungen wirken sofort:** alle Module auf Einstellungen durchgesehen, die nichts taten oder erst nach Neustart wirkten, und repariert. Unter anderem: Zoom-Regler live, FPS mit Intervall und Anzeige FPS/Frametime, Ping-Farbe, View Model, Durability Warning, Upside Down, Keystrokes-Glow (doppelte Ids), Pomodoro, Eye Break, Scripts, Toggles nach Alt-Tab, Chat-Tag ohne Farbcodes, Debug Menu.
- **Nichts lädt neu:** Config speichert im Hintergrund und nur bei Änderung, neue Swapchain auf demselben Gerät wird übernommen statt neu aufgebaut, ein Serverwechsel unter 8 s beendet die Sitzung nicht, HUD bleibt in Spiel-Menüs stehen.
- **Eingabe:** Fensternachrichten werden auf dem Render-Thread abgearbeitet, der Eingabe-Thread wartet auf nichts mehr. Mausbewegung ohne offenes Menü geht gar nicht durch ImGui. Shader kompilieren im Hintergrund, keine Ruckler beim ersten Öffnen.
- **Schwache PCs:** Auto-Profil "Low" schaltet Blur, Glow und Extras ab, Frame Limiter bleibt an. Gemessen im Testfenster (Software-Rendering): ohne Client 241 FPS, mit Client 221 FPS.
- **GUI Scale:** Stufen 1 bis 6 wie bei Flarial, Knöpfe 1 / 1.5 / 2 / 2.5 / 3 / 4, "nur ganze Stufen". Der Hook ins Spiel fehlt noch (zuhause, Punkt 5).
- **HUD:** neue Module stellen sich beim ersten Mal nicht mehr über andere (rücken darunter). Neues Modul **Item Tracker** (+3 Iron Ingot / −1 Ender Pearl, einige Sekunden).
- **CI:** Tour mit 1080p-Screenshots von Menü, Seiten, HUD-Editor, Launcher und Akzentwahl. Die gebauten Dateien liegen mit den Ergebnissen auf `ci-results/claude-onix-ui-input-fixes` (`bin/`).
- **Absturz behoben:** Der Shader für Blur und Effekte kompilierte im Hintergrund auf einem Thread mit dem Standard-Stack der Exe; im Testfenster stürzte das in den ersten Sekunden nach dem Start manchmal ab (Seitenfehler im Compiler). Hintergrund-Threads haben jetzt 8 MB Stack-Reserve, Shader-Kompilierungen laufen nie gleichzeitig. Die CI startet den Client fünfmal frisch und zählt Abstürze (`fresh.txt`, danach 0 von 5). Abstürze, die in unserem Code beginnen, schreiben jetzt die Aufrufer ins Log (`fault … called from mochi+0x…`, auflösbar mit `objdump -t`).
- Nicht im echten Spiel geprüft. Was zuhause zu prüfen ist: `docs/PC_TEST.md`, Abschnitt "Live-Einstellungen und Owner".

## 2026-10-02 nachmittags, Branch claude/onix-ui-input-fixes (Cloud)

- Menü nach Felix' Onix-Screenshots neu gebaut: Suche, Liste und Einstellungs-Panel als drei schwebende Panels mit Blur und dünnem Rand, Mini-Schalter, blau getönte aktive Zeilen, Onix-Einstellungszeilen (Unbind + Tastenfeld, Zeilen-Slider, Change Color). Details in `docs/UI.md` unter "Fünfte Änderung".
- Schrift Poppins statt Barlow (Client und Launcher), Standard-Theme "Slate".
- Tippen im Menü startet die Suche, Scrollbalken nur wenn nötig.
- Im Testfenster (Wine, CI) geprüft, im echten Spiel noch nicht.

## 2026-10-02 mittags, Branch claude/onix-ui-input-fixes (Cloud)

- Menü als lange Liste links (Onix-Richtung), Kacheln und Symbole wieder raus. Details in `docs/UI.md` unter "Vierte Änderung".
- Cosmetics und Einstellungen laden beim Wechseln von Slot oder Tab nicht mehr neu, Cosmetics liest die Dateien nur noch beim ersten Öffnen.
- HUD bleibt in Menüs innerhalb einer Welt (Pause, Inventar, Chat) bis zu 60 s stehen und ist nach dem Zurückkehren sofort da. Vorher verschwand es und kam erst 1,2 s nach dem Schließen wieder. Verlässt man einen Server, geht es sofort aus, nach einer Einzelspielerwelt nach spätestens 60 s.
- Schrift Barlow statt Nunito (inzwischen Poppins), Rahmenfarbe einstellbar.

## 2026-10-02, Branch claude/onix-ui-input-fixes (Cloud)

- **Menü-Taste nur im Spiel:** Rechts-Shift und Modul-Tasten wirken nur noch, wenn das Spiel den Fokus hat und der Mauszeiger versteckt ist (also wirklich gespielt wird). Im Hauptmenü öffnet sich nichts mehr. Netzwerkverkehr zählt nicht mehr allein als "in einer Welt", weil das Hauptmenü selbst sendet (LAN-Suche, Xbox Live, Serverliste); er hält den Zustand nur, wenn er schon beim Spielen da war.
- **Spiel steht still, solange das Menü offen ist:** Die GDK-Version liest Tastatur und Maus über GameInput (das Spiel bringt `GameInputRedist.msi` mit). Die bisherigen Hooks auf Fenster-Nachrichten konnten das nicht stoppen. Neu ist `hook/GameInput.cpp`: hookt `GetKeyCount`, `GetKeyState` und `GetMouseState` der Readings (alle API-Versionen v0 bis v3, Slots aus den öffentlichen Headern), bei offenem Menü sieht das Spiel keine Tasten, keine Klicks und keine Mausbewegung. Beim Schließen gibt es keinen Kamerasprung und Tasten, die beim Schließen noch gedrückt sind (Esc, Rechts-Shift), kommen erst nach dem Loslassen wieder durch.
- Toggle Sprint, Toggle Sneak und Null Movement wirken jetzt auch über GameInput (gehaltene bzw. unterdrückte Tasten). Sens Multiplier skaliert ohne Signatur die Mausbewegung, Disable Mouse Wheel hält das Mausrad an.
- **Zoom ohne Signatur:** vergrößert vorerst das Bild per Post-Shader, mit angepasster Mausempfindlichkeit und Mausrad-Stufen. Mit der FOV-Signatur nimmt Zoom wieder das echte Sichtfeld.
- **Instant Hit entfernt:** war dieselbe Funktion wie Low Latency (Frame-Queue, Tearing, Thread-Priorität). Low Latency hat jetzt die Thread-Priorität und die Klick-bis-Bild-Messung.
- **FPS-Limiter:** wartet jetzt nach Present statt davor. Vorher hielt er ein fertiges Bild zurück und erhöhte damit die Latenz.
- **Neues Menü (Richtung Onix):** Rechts-Shift öffnet direkt das Menü, kein Hub mehr. Links eine Leiste mit Abschnitten (Alle, PvP, HUD, Visual, Nützliches, Leistung, Server, Extras), Cosmetics, Settings und Edit HUD. Rechts Kacheln mit Symbol, Name und Status, Klick schaltet um, Zahnrad oder Rechtsklick öffnet die Einstellungen. 175 Module stecken in 55 Kacheln (`gui/Tiles.cpp`), z. B. Kampf-Infos (Reach, Combo, Hit Ping, Target …), Item-Zähler, Treffer-Feedback, Klare Sicht, Kamera. Gruppen öffnen rechts ein Panel mit allen Teilen, jeder Teil hat Schalter und aufklappbare Einstellungen. Suche und Favoriten zeigen einzelne Module. Neues Standard-Theme "Carbon" (dunkel, ein blauer Akzent), wer noch das alte Standard-Theme hatte, bekommt Carbon.
- **CI:** `.github/workflows/check.yml` baut bei jedem Push auf `claude/**` mit MinGW, startet den Client im Testfenster unter Wine, klickt durchs Menü und legt Screenshots und Logs auf den Branch `ci-results/<branch>`. Damit lässt sich ohne apt-Zugang in der Cloud bauen und prüfen.
- Nicht im echten Spiel geprüft: alles oben. Was zuhause zu prüfen ist, steht in `docs/PC_TEST.md` unter "Branch claude/onix-ui-input-fixes".

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

## Session A, Block: neues Menü (Hub, Liste, Favoriten, Settings, Cosmetics)

- Rechts-Shift öffnet zuerst den Hub (Karten Modules, Favorites, Cosmetics, Settings, Suche, Schnellschalter für Favoriten). Noch einmal Rechts-Shift öffnet das volle Menü, Esc geht eine Ebene zurück.
- Modules: eine lange Liste nach Muster M1 (`docs/ui_proposals/m1_plain_twopane.png`): Abschnitte Server, HUD, PvP, Visual, Comfort, Performance, Extras mit dezenten Pastellfarben, Einstellungen des gewählten Moduls rechts. Oben nur Suche, Favoriten-Filter, "More modules", Edit HUD.
- Favoriten: Stern an jeder Zeile, wird pro Profil gespeichert (`favorite` im Modul-JSON), Favoriten stehen oben in der Liste und als Schnellschalter im Hub.
- Settings-Seite mit Unterseiten General, Chat & watermark, Appearance, Modules (Voreinstellungen Minimal/PvP/Alles aus, Standard-Größe der HUD-Module, HUD-Positionen zurücksetzen), Profiles, About. Animationen lassen sich abschalten (`draw::setMotion`).
- Cosmetics-Seite liest `%LOCALAPPDATA%\Mochi\cosmetics\index.json` (Format in `docs/COSMETICS.md`), Ausrüsten pro Slot, 3D-Vorschau folgt mit dem ersten Set.
- Watermark im Inventar (Pille unten rechts, wenn das Inventar offen ist).
- `gui/Gui.cpp` aufgeteilt in `Gui.cpp`, `GuiCommon.cpp`, `ModulesPage.cpp`, `SettingsPage.cpp`, `CosmeticsPage.cpp`. Mouse Strokes, CPS, Keystrokes, Armor HUD und Potion HUD stehen jetzt unter HUD statt PvP.

## Session A, Block: Blur, Shader Packs, Cosmetics-Vorschau, Eingabe-Priorität

- Hub kompakt (rund 720 px breit), Hintergrund wird beim Öffnen echt unscharf (Shader, Einstellung "Menu background blur"). Neues Modul Blur (Vollbild, immer oder nur in Menüs).
- Shader Packs: eigener Pixel-Shader über dem Spielbild, vier eingebaute Looks, eigene `.hlsl` aus `%LOCALAPPDATA%\Mochi\shaders`. Details und Grenzen in `docs/RENDERDRAGON.md`.
- Cosmetics: Lader (`dll/src/cosmetics/`) liest `item.json` und `tex.png`, 3D-Vorschau mit Drehen, Animationen (flap, sway, bob, wag, twitch, float, spin), Farbmischung über Tints. Unter Wine mit Testdaten geprüft (`tools/testdata/cosmetics`).
- Alle Fenster öffnen animiert: Hub, Menü, Seitenwechsel, Settings-Unterseiten, HUD-Editor (blendet ein, Hilfsleiste schiebt von oben).
- System Boost: Spiel-Priorität (MMCSS "Games") für Render- und Fenster-Thread, abschaltbar.

- **Session B, nach dem Merge von main:** 173 Module registriert (171 aus Session B plus Client Settings und Performance Lock aus main). Der Self-Test unter Wine läuft sauber durch: 171 Module geprüft, 0 Fehler, 0 Aussetzer, 0 unübersetzte Texte (17 fehlende deutsche Einträge ergänzt, vor allem Datumsformate und Hive-Spielnamen). `tools/cross.sh build` und `shots` ohne Fehler im Log. `docs/MODULES.md` neu aus dem Modul-Dump erzeugt. Weiterhin gilt: nichts davon wurde im echten Spiel getestet, und die Hive-/Zeqa-Chattexte und API-Felder sind Annahmen.
- **Session B, Mochi Online (Client und Dienst):** Neues Modul "Mochi Online": rotes Herz, Namensfarbe (einfarbig, Verlauf, Regenbogen, Puls) und Tag anderer Mochi-Nutzer in Tab-Liste und Better Chat, eigener Stil mit allen Einstellungen, Einwilligungs-Hinweis beim Einschalten, Knopf zum Löschen der Daten. Dienst in `server/` (Cloudflare Worker, D1, KV): 18 Tests grün gegen Speicher und gegen die echte SQL-Datenbank. Der Wine-Client meldet sich gegen den lokalen Dev-Server an und zeigt die Nutzer von dort (hello, presence, lookup). Nicht getan: Veröffentlichen (Felix: Cloudflare-Konto, siehe `server/README.md`) und der Xbox-Beweis für Gamertags. Außerdem: Strg+L entlädt nicht mehr, wenn Toggle Sprint Strg per Eingabe-Injektion hält (`hook/Input.cpp`).
- **Session B, Merge mit dem neuen Menü:** main (Hub, lange Liste, Cosmetics-Seite, Blur, Shader Packs) eingearbeitet. Mein Cosmetics-Modul und meine Vorschau sind gelöscht, Lader und Seite aus main gelten. Mochi Online folgt den neuen Namens-Regeln: kein freier Text mehr (Tags entfernt, auch im Dienst), Herz hinter dem Namen mit wählbarer Farbe, ausgerüstete Cosmetics kommen aus `ClientSettings::equipped()`. Das Blur-Feld "Background blur" der HUD-Module bleibt neben dem Menü-Blur aus main bestehen (`post::blur`). Self-Test 176 Module: 0 Fehler, 0 unübersetzte Texte, zwei Aussetzer über 350 ms (Shader Packs beim HLSL-Kompilieren, Hit Sound einmalig unter Wine, einzeln nicht reproduzierbar). Die detaillierten Flügel und Capes liegen als Generator und Testset in `tools/cosmetics_hd/`, ihre Format-Erweiterungen stehen als Wunsch in `docs/HANDOFF.md`.

## Session A, Block: Cosmetics-Set und Format-Erweiterungen

- Neue glatte Körper (`tubes`) für Katzen- und Hundeohren, Schwänze, Bandana, Mütze, Zaubererhut und Sneaker, alle animiert (Ohren zucken, Schwänze wehen und wedeln, Bandana-Enden flattern, Bommel wippt, Hutspitze weht, Füße schwingen). Generator `tools/cosmetics/build.py`, Ausgabe in `cosmetics/`.
- Lader und Vorschau unterstützen jetzt auch die Erweiterungen aus Session B (`texel`, `flat`, `mirror`, `tint2`/`mix`, `sparkle`, `physics` mit Feder und Stoff). Alle Flügel und Capes aus `tools/cosmetics_hd/` laufen im Menü, Bewegung der Vorschau wählbar (Auto, Ruhig, Gehen, Sprinten, Springen).
- Cosmetics-Seite: Karten zoomen auf den Slot (Kopf, Füße, Rücken), Slot-Filter Feet, Farben pro Teil, Slim/Wide-Körper, Dreh- und Animationstempo.
- Offen: Verteilung der Cosmetics an Nutzer (Download beim ersten Start oder Bündel im Launcher, braucht eine Entscheidung), Anzeige im Spiel (Signaturen), Sichtbarkeit für andere (Mochi Online).

## Session A, Block: Versionswahl im Launcher, Leben über dem Kopf

- Launcher, Seite Versions: findet Minecraft-Installationen anderer Launcher (LeviLauncher-Ordner, eigene Ordner über "Add folder"), liest die Version aus der Exe, zeigt Mochi-kompatibel oder nicht und merkt sich die gewählte Version in `launcher.json` (`pinned`). Play startet genau diese Exe direkt und verbindet sich. Meldet, wenn schon eine andere Version läuft. Der Store-Eintrag bleibt als Standard. Gebaut, Oberfläche und Suche unter Wine geprüft, der echte Start einer Exe außerhalb des Stores ist am PC offen (`docs/PC_TEST.md`).
- Kein Überschreiben der Store-Installation: Mochi startet die gewählte Version, ohne Dateien im Store-Ordner anzufassen. Das Laden alter Versionen übernimmt LeviLauncher mit der eigenen Lizenz des Spielers.
- Neues Modul "Health Above Head": Balken und Zahl über anderen Spielern, projiziert über `game::project`. Läuft mit Demo-Daten und wird im Spiel erst aktiv, wenn der Provider `others` liefert. Ausgeliefert aus (Server-Regeln).
- Eigener Name im Spiel bei F5: "Third Person Nametag" von Session B (Hook, braucht die Signatur).
- Offen, nur am PC: Cosmetics im Spiel für andere sichtbar. Ansatz in `docs/COSMETICS.md` unter "Cosmetics im Spiel".
- **Plan für den PC:** `docs/PLAN_HOME.md` beschreibt Phase für Phase, wie Claude Code zu Hause den Client im echten Minecraft verbindet und testet (Start, Signatur-Werkzeuge, vier Signatur-Wellen, Server, Mochi Online, Cosmetics, Messung gegen Flarial und Onix, Versionen und Release).

## 2026-10-01 abends, PC-Test (claude/pc-test)

- Der Client baut mit MSVC, lädt im echten Minecraft 1.26.52.3 und zeichnet über DX12 (Hub, Blur, HUD). Phase 0 ist begonnen, Gate noch nicht erreicht: Strg+L, Fenstergröße, Vollbild, Alt-Tab, 30-Minuten-Lauf und Versionswahl sind ungeprüft.
- 179 Module registriert, 107 gesperrt (`locked:`), `sigs: 0/0`. Das Ziel "locked leer" hängt an den Signaturen (Phase 2).
- Fehler aus dem ersten Spieltest (HUD in Menüs, Eingabe bei offenem Menü, Toast-Seite) sind im Code behoben, Prüfung im Spiel steht aus, siehe `docs/TESTLOG.md`.

## Session A (Cloud), Block: PC-Stand übernommen, Anker-Signaturen

- `claude/pc-test` (Stand 2026-10-02) geholt: baut mit MinGW, Selbsttest 177 Module, 0 Fehler, 0 Aussetzer, 0 unübersetzte Texte.
- Neu: Signaturen über Anker (`dll/src/sig/Image.*`). Eine Zeichenkette der Exe führt per `lea` zur Funktion, über die Slot-Nummer zur vtable, und ein Eintrag kann eine Funktion aus einer vtable lesen. Das entspricht dem Weg, den der Dev-Explorer am PC gefunden hat (ClientInstance, getLocalPlayer). Format in `docs/VERSIONING.md`. Unter Wine geprüft: `selftest: image resolver ok`, und die Ladestrecke mit einer Test-Signaturdatei (`sigs: 3/4`, der falsche Anker wird gemeldet). An der echten Minecraft-Exe ungeprüft.

## Session A (Cloud), Block: Modulliste kurz und neutral

- Die Modulliste zeigt zuerst nur die rund 35 wichtigsten Module (Stufe 1), nach HUD, PvP, Visual, Comfort und Performance geordnet. Alles andere steckt in eingeklappten Gruppen darunter: "More HUD", "More PvP", "More Visual", "More Comfort", "Extras" und "Server". Die Suche zeigt weiter alle Module. Wer auf einem Server ist, bekommt die Server-Gruppe oben und aufgeklappt. Der Knopf "More modules" ist weg.
- Neues Standard-Theme "Graphite" (neutrales Dunkelgrau, ein gedämpfter blauer Akzent, ohne Verläufe, Funken und Herzchen). Die rosa Themes bleiben unter Settings, Appearance wählbar. Wer schon ein Theme gespeichert hat, behält es.
- Abschnittsfarben in der Liste sind einheitlich neutral statt bunt.
- Fehler gefunden und behoben: ein veralteter Cache der Signatursuche konnte auf eine Adresse außerhalb des Abbilds zeigen und den Client abstürzen lassen. Adressen werden jetzt geprüft, Vtable-Slots sicher gelesen.

## 2026-10-02 abends, Branch claude/ui-polish (Oberfläche, ohne Spiel getestet)

Gebaut, aber noch nicht im Spiel angesehen. Bitte Screenshots prüfen.

- Dropdown für alle Auswahl-Einstellungen (Hold/Toggle, Formen, Farben usw.): eigenes Popup in `gui/Widgets.cpp` statt ImGui-Selectable. Theme-Farben, Rundung, Schatten, Hover, Haken am gewählten Eintrag, Aufklappen mit Animation, klappt nach oben, wenn unten kein Platz ist, scrollt bei langen Listen, Pfeiltasten, Enter und Esc. Die Pfeil-Anzeige am Auslöser dreht sich. Der Farbwähler hat jetzt ebenfalls Rundung, Rand und Theme-Hintergrund.
- Aufklappen von Modulen in einer Gruppe: Zeit-basiert statt exponentiell, Höhe mit ease-in-out, Inhalt blendet in der zweiten Hälfte ein, Höhenänderungen im offenen Modul (Optionen, die auf- oder zuklappen) laufen weich statt zu springen.
- Custom Crosshair: neue Form "Solid cross" (Standard für neue Configs). Gefülltes Kreuz mit undurchsichtigem Rand in GUI-Pixeln (Armlänge 8, Dicke 1, plus je 1 GUI-Pixel Rand), das das Vanilla-Kreuz verdeckt, solange `fx::available(HideCrosshair)` false ist. Hinweis im Modul, wenn eine andere Form gewählt ist. Die Maße des Vanilla-Kreuzes sind geschätzt (`coverArm`, `coverThick` in `Crosshair.hpp`), im Spiel gegenprüfen. GUI-Skalierung automatisch wie bei den anderen HUD-Modulen, einstellbar.
- Network-Modul: der Ping-Verlauf zeichnete die Balken als dicke Linien und ragte links und rechts aus dem Kasten. Jetzt Rechtecke im Kasten, neueste rechts.
- Pack Display braucht keine Spieldaten mehr: liest `global_resource_packs.json` und die Manifeste der Packs (Namen aus `texts/en_US.lang` bei `pack.name`). Zeigt nur globale Packs, Server-Packs stehen nicht drin, steht in der Beschreibung.

Graue Module durchgesehen, ehrlich geblieben. Ohne Spieldaten läuft sonst keins richtig:

- Mumble Link, Waypoints, Chunk Border, Fall Predictor, Death Logger, Coordinates, Richtung, Tempo, Blickwinkel brauchen nur Position, Kamera und `LocalPlayer`. Das ist schon der Stand der Live-Leser, die waren nur im alten Log grau.
- Session Stats braucht Treffer, Kills und Chat-Ereignisse, die es ohne Spiel nicht gibt. Day Counter braucht die Weltzeit (Systemzeit wäre falsch). Hotbar-Module, Inventar, Effekte, Ziel, Chat, Scoreboard und Tab-Liste haben keine Quelle außerhalb des Spiels.

Achtung, Live-Leser (nicht angefasst, gehört der anderen Sitzung): `supports()` meldet `Player` schon mit Position und Blick. Gesundheit, Hunger, Erfahrung, Luft, Dimension und Boden-Flag fallen auf Standardwerte zurück, solange ihre Offsets fehlen. Health Display, Low Health Indicator, Better Hunger Bar und Experience Info zeigen dann falsche Zahlen (immer 20/20) statt grau zu sein. Entweder die Domain feiner aufteilen oder diese Module an eigene Offsets koppeln. Waypoints rechnen mit `player.dimension`, ohne Offset immer Overworld.

## 2026-10-02 spät, Branch claude/ui-polish-2

Ohne Spiel gebaut (DLL und Launcher ohne Fehler), noch nicht angesehen.

Ehrlichkeits-Prüfung der freigeschalteten Module. Das Live-Lesen liefert Position, Geschwindigkeit, Blickwinkel, FOV, Ansicht, Ziel (Art, Position, Abstand, Block-XYZ) und Hit-Ereignisse mit Reichweite. Alles andere steht im Spielzustand auf Standardwerten. Dafür gibt es Pseudo-Signaturen: ein Modul mit so einer Signatur bleibt grau, bis die Signatur aufgelöst wird; Teilfunktionen werden mit `need::have("Name")` ausgeblendet. Die Namen stehen in `modules/common/Needs.hpp`.

| Pseudo-Signatur | Felder | Graue Module | Ausgeblendete Teile |
|---|---|---|---|
| `PlayerStats` | Leben, Hunger, Erfahrung, Luft | Health Display, Experience Info, Low Health Indicator, Fall Predictor | Stats HUD (Leben, Hunger), Debug Menu (Spielerblock), Lua `player.health/hunger/level` |
| `MoveState` | Schleichen, Sprinten, Boden, Schwimmen, Gleiten, Fliegen | Java Dynamic FOV, Auto Perspective, Fall Predictor, Hit Info | FOV Changer (Sprint-Bonus), Sens Multiplier (Schleichen, Sprinten), Crit-Optionen bei Hit Counter, Hit Marker, Hit Effects, Hit Sound, Lua `onGround/sprinting/sneaking/swimming` |
| `UseState` | Item benutzen, Blocken, Bogen | Bow Sensitivity, Bow Charge (schon vorher) | Sens Multiplier (Bogen, Blocken) |
| `HurtEvents` | Treffer erhalten, Kill, Tod, Schaden | Combo Counter, Death Logger, Hit Info | Wegpunkt "Tod", Stats HUD (Combo), Debug Menu (Kills, Tode), Lua `combat.combo/kills/deaths/streak` |
| `WorldTime` | Weltzeit, Tag, Regen | Day Counter (Level-Signatur) | Clock (Spielzeit, Tag), Lua `world.time/day/raining` |
| `Dimension` | Dimension | | Coordinates (Dimension, Nether-Umrechnung, {D}), Waypoints (Hinweis, zeigt in jeder Dimension), Lua `player.dimension` |
| `Biome` | Biom | | Coordinates (Biom), Lua `world.biome` |
| `Inventory` | Hotbar, Slot, Rüstung | alle Inventar-Module (schon vorher) | Hide Hand ("nur mit leerer Hand"), Lua `player.slot/held` |
| `TargetInfo` | Name, Spieler, Leben, Abbaufortschritt | Target HUD, Waila, Break Progress (schon vorher) | Crosshair ("nur Spieler"), Debug Menu (Namen), Lua `target.name/isPlayer/health/breakProgress` |
| `PlayerName` | eigener Name | | Mumble Link (Identität leer) |

- `player.name` steht jetzt standardmäßig leer. Vorher war es "Player": Mochi Online hätte sich auf dem Server so angemeldet, und Nick hätte das Wort "Player" im Chat ersetzt. Demo-Daten setzen den Namen weiter selbst.
- Hit-Ereignisse: `crit` hängt an Boden und Sprinten und stimmt deshalb erst mit `MoveState`. Ein "Hit" ist ein Klick auf das anvisierte Entity, keine bestätigte Wirkung.

HUD-Positionen: Standardwerte aller HUD-Module auf Spalten verteilt (links Infozeilen, daneben Kampf und Inventar, dahinter Stats HUD, Pomodoro, Death Logger, Pack Display, Keystrokes; rechts Potion HUD, Scoreboard, Item Tracker, Paperdoll, Pet; Netzwerk-Kästen mittig rechts). Dazu schiebt ein nie platziertes HUD-Modul sich jetzt jedes Bild unter die Module, die vor ihm gezeichnet wurden, statt nach 30 Bildern einzufrieren. Die Verschiebung wird nicht gespeichert, das Modul bleibt auf seinem Standardplatz, sobald dort wieder Platz ist. Wer ein Modul im HUD-Editor zieht, hat es platziert, dann bewegt es sich nie mehr. Die Höhen der Kästen sind geschätzt.

Menü: Öffnen langsamer und weicher als Schließen (zeitbasiert, mit leichtem Überschwingen der Panels, Abdunklung mit Verlauf), Schalter mit zusammengedrücktem Knopf und Schein, Buttons mit Hover-Rand und Druck-Effekt, Zeilen mit weichem Hover und Druck, Listeneinträge rücken beim Hover leicht ein.

Launcher: gebaut ohne Fehler (nur die `sscanf`-Warnung in `Game.cpp`). Farben wie das Slate-Theme des Clients, Karten und Seitenleiste mit Rand und Rundung wie die Menüpanels, Schalter und Buttons wie im Client, Fortschrittsbalken mit Verlauf und weichem Nachziehen.

## 2026-10-03, Branch claude/ui-smooth: Aufklappen ohne Ruckeln

Gemessen mit dem Testhost auf der echten GPU (D3D11, Fenster außerhalb des Bildschirms, 1920x1080, 170 Hz), ohne Minecraft. Jede Messung spielt dasselbe Skript: Menü auf, Gruppe mit mindestens 5 Teilen wählen, Teile nacheinander auf- und zuklappen, danach alle öffnen, ans Ende scrollen und ein Teil mit sichtbarem Kopf zuklappen.

Ablauf zum Nachmessen (kein Spiel, kein eigenes Config-Verzeichnis berührt, `Mochi.root` neben der DLL leitet die Daten um):

```
Mochi.dll und Mochi.root (eine Zeile: Datenordner) in einen Ordner legen
TESTHOST_OFFSCREEN=1 TESTHOST_MANUAL=1 MOCHI_PROFILE=1 TESTHOST_FPS=170 TESTHOST_SIZE=1920x1080 testhost.exe Mochi.dll 37
```

`MOCHI_PROFILE` schreibt `logs/profile.csv` (pro Bild: dt, CPU-Zeit des UI-Frames, Zeit für Liste und Details, Zeilen, Draw-Calls, Vertices, Höhe des aufklappenden Teils, Position einer Folgezeile, Scroll) und fährt das Skript selbst (`profileDrive` in `ModulesPage.cpp`). Im Debug-Menü steht die Menüzeit im Mochi-Block. Der Testhost hat jetzt `TESTHOST_FPS` (festes Tempo, `timeBeginPeriod(1)`) und gibt Mittel, Median, p99 und Maximum der Bildabstände aus.

Ursache, gemessen statt geraten:

- Nicht die CPU: ein UI-Frame kostet im Mittel 0,2 ms (Liste 0,09, Details 0,04), 13 Draw-Calls, etwa 9000 Vertices, 5 Fenster. dt ist stabil (5,9 ms, p99 6,3). Die Blur-Pässe der Panels sind ein Backbuffer-Copy und drei Scissor-Pässe, auch das ist nicht das Problem.
- Das Ruckeln war die Bewegung selbst. Ein offener Teil ist bei 1080p rund 1100 px hoch, die Animation lief aber fest 0,3 s (zu 0,22 s beim Zuklappen) mit Ease-in-out. Die Zeilen darunter sprangen dadurch in der Mitte bis zu 67 px (auf) und 90 px (zu) pro Bild. Dazu kommt, dass ImGui Fensterpositionen und den Cursor auf ganze Pixel rundet, der Auslauf also in unregelmäßigen 0/1-Pixel-Schritten lief.
- Kein Layout-Feedback gefunden: die Höhe wird mit dem Inhalt gemessen, aber der Inhalt hängt nicht von der Höhe ab.

Fix:

- Dauer wächst mit der Höhe des Körpers (`0,2 s + Höhe * 0,0004`, höchstens 0,8 s, Zuklappen 0,8x), Kurve ease-in-out über einen Sinus (Spitze nur 1,57x des Mittels statt 3x).
- Der Körper ist kein Kindfenster mehr. Er wird in der Liste in natürlicher Größe gelegt, ein Clip-Rect schneidet ihn bei der animierten Höhe ab, der Cursor rückt um diese Höhe weiter. Nichts wird pro Bild neu erzeugt, und die Höhe des Inhalts steht fest, nur der Ausschnitt wächst.
- Klappt ein Teil mit sichtbarem Kopf zu, hält die Liste ihre Höhe und gibt den Platz danach langsam zurück. So bleibt der geklickte Kopf stehen, statt vom Scroll-Clamp mitgezogen zu werden. Klappt ein Teil außerhalb des Bildes zu, bleibt es bei der alten Mitnahme (die sichtbaren Zeilen stehen still).
- Dropdowns: Dauer nach Höhe der Liste (0,12 s + 0,0003 s/px beim Öffnen).

Zahlen, 170 Hz, maximaler Sprung einer Zeile pro Bild (Median der vier Teile):

| | vorher | nachher |
|---|---|---|
| aufklappen | 63 px | 17 px |
| zuklappen | 87 px | 20 px |
| aufklappen bei 60 Hz | | 72 px (Strecke 1100 px in 0,45 s) |
| Kopf beim Zuklappen am Listenende | wandert 439 px in 0,35 s mit 14 px/Bild | bleibt stehen, danach 1 s Gleiten |
| CPU pro UI-Frame (Mittel) | 0,18 ms | 0,22 bis 0,30 ms (Szenario jetzt mit allen Teilen offen) |

Offen: bei 60 Hz bleiben es etwa 60 px pro Bild, das ist die Strecke von 1100 px, nicht das Ruckeln. Wer das ruhiger will, kann die Dauer weiter strecken oder sehr hohe Teile ohne Höhenanimation einblenden. Die Teile im Spiel selbst sind nicht gemessen, nur im Testhost.

## 2026-10-03, Branch claude/hotbar-overlay

Ohne Spiel gebaut, nicht angesehen. Gemeinsame Geometrie der Vanilla-HUD in `modules/common/VanillaHud.hpp` (`vanilla::hud`): Hotbar unten mittig 182x22 GUI-Pixel, Slotzelle 20x20 ab x=1 (Innenbereich 16x16 ab x=3), Herzen links und Hunger-Keulen rechts je 10x8 px, Unterkante 39 GUI-Pixel über dem Rand. GUI-Skalierung wie Bedrock aus der Fenstergröße, einstellbar. Beide Module haben "Feinkorrektur" (Versatz X/Y in GUI-Pixeln, Größe).

- Hotbar Animation: Rahmen liegt genau auf der Slotzelle von `player.slot`, gleitet weich, Pop nur 0,5 GUI-Pixel pro Seite (Standard 1).
- Better Hunger Bar: Standardmodus legt die Sättigung als goldenen Rand mit Füllung (#FFD24A) auf die echten Keulen (Pixelraster 8x8, Rand = Maske plus 1 Pixel); Essen in der Hand lässt die Vorschau von Hunger und Sättigung halbtransparent auf genau diesen Keulen blinken. Der alte Balken mit Text ist der zweite Modus.

Zu prüfen im Spiel: Die Referenz (Hotbar x 830–1910 bei 2560 Breite) liegt etwa 96 px rechts der Mitte, die Formel setzt sie mittig (734–1826). Falls das im Spiel so ist, mit Versatz X korrigieren. Auch die Höhe der Referenz (70 px) passt nicht zu 22*6 = 132 px, die Keulenform ist eine Annäherung.
