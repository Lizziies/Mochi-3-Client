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
| Module | 137 von 135 (Code vollständig, nicht im Spiel getestet) | Liste und Signatur-Bedarf in `docs/MODULES.md` |
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
