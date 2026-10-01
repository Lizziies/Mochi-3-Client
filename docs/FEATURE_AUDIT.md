# Feature-Audit: Was Flarial und Onix haben, nach Wichtigkeit

Ziel: Alle Features der anderen Clients erfassen, nach Wichtigkeit für PvP-Spieler sortieren und mit unserem Stand abgleichen. Wir haben viele Dinge gebaut, die keiner braucht, und gleichzeitig Standard-Module vergessen. Diese Datei ist die Grundlage, um das zu korrigieren.

## Quellen

- **Flarial:** vollständige Modul-Registrierung aus dem Quellcode (`flarialmc/dll-oss`, `Manager.cpp`, 129 Einträge, ✔ gelesen). Dazu aus dem Changelog und aus Nutzer-Screenshots: Crystal Optimizer, Quality of Life (inkl. Pack Changer), Shader Loader, MC GUI Scale, Nametag. Die Zahl "140+" der Webseite enthält Android-Module.
- **Onix:** eingebaute Module laut Webseite/FAQ (über Suchergebnisse, ~) und die 100 Community-Skripte aus `OnixClient-Scripts` (✔ gelesen). Onix selbst ist closed source, die eingebaute Liste ist daher unvollständig.
- **Latite:** Liste aus `docs/CLIENTS.md`.
- **Wichtigkeit** ist meine Einschätzung für Bedrock-PvP auf Hive, Zeqa, CubeCraft, Lifeboat, Galaxite. Sie gehört mit echten Spielern abgeglichen (bitte Felix und Freunde korrigieren).

## Stufen

| Stufe | Bedeutung | Soll im Menü |
|---|---|---|
| **1 Kern** | Fast jeder PvP-Spieler nutzt es täglich. Muss perfekt laufen | immer sichtbar |
| **2 Erwartet** | Spieler anderer Clients erwarten es. Fehlt es, wirkt der Client unfertig | immer sichtbar |
| **3 Nützlich** | Nische oder Komfort | unter "Mehr" |
| **4 Extras** | Spaß oder kein PvP-Nutzen | unter "Extras", standardmäßig versteckt |

**Stand:** ✔ haben wir · ◐ teilweise · ✖ fehlt · ➕ nur bei uns

---

## Stufe 1: Kern (das muss da sein)

| Feature | Flarial | Onix | Wir | Anmerkung |
|---|---|---|---|---|
| CPS Counter | ✔ | ✔ | ✔ | |
| FPS Counter | ✔ | ✔ | ✔ | |
| Ping Counter | ✔ | ✔ | ✔ | |
| Keystrokes | ✔ | ✔ | ✔ | Tastenanimation, Layout |
| Armor HUD (mit Haltbarkeit) | ✔ | ✔ | ✔ | |
| Potion HUD | ✔ | ✔ | ✔ | |
| Coordinates | ✔ | ✔ | ✔ | |
| Toggle Sprint / Toggle Sneak | ✔ | ✔ | ✔ | |
| Reach Counter | ✔ | ✔ | ✔ | |
| Combo Counter | ✔ | ✔ | ✔ | auf Hive wichtig |
| Zoom | ✔ | ✔ | ✔ | smooth, Scroll |
| Fullbright | ✔ | ✔ | ✔ | |
| Freelook | ✔ | ✔ | ✔ | auf Hive gesperrt |
| FOV Changer | ✔ | ✔ | ✔ | |
| Hurt Color | ✔ | ✔ | ✔ | |
| No Hurt Cam | ✔ | (Latite ✔) | ✔ | |
| Custom Crosshair | ✔ | ✔ | ✔ | |
| Block Outline | ✔ | ✔ | ✔ | |
| **Animations (1.8-Look, Swing)** und **Block Hit** | ✔ (Animations, BlockHit, SwingAnimations) | ◐ | ✔ (gesperrt) | "Animations" und eigenes Modul "Block Hit" sind da, brauchen aber die Hand-Signatur |
| Render Options / FPS-Boost | ✔ | ✔ | ✔ | |
| Auto GG | ✔ | ✔ | ✔ | |
| Scoreboard verschieben/aufräumen | ✔ (Movable, Clear) | ✔ | ✔ | |
| GUI Scale | ✔ | (Latite ✔) | ✔ | |
| Raw Input / Instant Input | ✔ (Raw Input Buffer, laut früherer Recherche nur Stub) | – | ◐ | Wir haben Instant Input. **Raw Input Buffer** als eigener Name fehlt. Unser Alleinstellungsmerkmal |
| **Hive Utils (Auto-Requeue)** | ✔ | ✔ (Skript HiveAutoQueue) | ✖ | **Wichtig für Hive-Spieler, fehlt komplett. Zuständig: Session B** |
| HUD-Editor, Config-Profile, Suche | ✔ | ✔ | ✔ | |
| Server-Regeln | – | – | ➕ | unser Plus |

## Stufe 2: Erwartet

| Feature | Flarial | Onix | Wir | Anmerkung |
|---|---|---|---|---|
| Clock / Time | ✔ | ✔ | ✔ | |
| Direction HUD | ✔ | ✔ | ✔ | |
| Speed Display | ✔ | ✔ | ✔ | |
| IP / Server Display | ✔ | ✔ | ✔ | |
| Paperdoll (verschiebbar) | ✔ | ✔ | ✔ | |
| Tab List (Java-Style) | ✔ | ✔ | ✔ | |
| Hitbox | ✔ | ✔ | ✔ | nur sichtbare Entities |
| Motion Blur | ✔ | ✔ | ✔ | |
| Waypoints | ✔ | ✔ | ✔ | |
| Debug Menu (F3) | ✔ | ✔ | ✔ | |
| Mouse Strokes | ✔ | – | ✔ | |
| Arrow / Pot / Totem Counter | ✔ | ✔ (Totem) | ✔ | |
| Armor/Item Counter | ✔ | ✔ | ✔ | |
| Opponent Reach, Hit Ping | ✔ | – | ✔ | auf Hive nur Hinweis |
| Low Health Indicator | ✔ | – | ✔ | |
| Better Hunger Bar | ✔ | ✔ (Skript) | ✔ | |
| Waila | ✔ | ✔ | ✔ | |
| Command Hotkey, Text Hotkey | ✔ | ✔ | ✔ | |
| Disable Mouse Wheel | ✔ | – | ✔ | |
| Java Dynamic FOV | ✔ | – | ✔ | |
| Minimal / Java View Bobbing | ✔ | ✔ (Skript) | ◐ | **No View Bobbing** haben wir (besser), Java View Bobbing fehlt |
| Particle Multiplier | ✔ | – | ✔ | |
| Time / Weather / Fog Changer | ✔ | ✔ (Environment) | ✔ | |
| Death Logger / Death Coordinates | ✔ | ✔ (Skript) | ✔ | |
| Player Notifier | ✔ | – | ✔ | |
| Chunk Border | ✔ | ✔ | ✔ | |
| Break Progress (BlockBreakIndicator) | ✔ | – | ✔ | |
| Cinematic Camera | ✔ | ✔ | ✔ | |
| Snap Look | ✔ | (Latite) | ✔ | |
| Auto Perspective | ✔ | – | ✔ | |
| Sens Multiplier / Bow Sensitivity | ✔ | – | ✔ | |
| Skin Stealer | ✔ | ✔ (Skript) | ✖ | Fehlt |
| **Nametag Modifier / Nick** | ✔ | ✔ | ✖ | Fehlt (Nametag eigener, Nick clientseitig) |
| **Item Physics** | ✔ | ✔ | ✖ | Fehlt |
| **Inventory Lock** | ✔ | ✔ (Skripte) | ✖ | Fehlt |
| **Java Inventory Hotkeys** | ✔ | ✔ (inventory-tweaks) | ✖ | Fehlt |
| **Modern Keybind Handling** | ✔ | – | ✖ | Fehlt (Bewegung nach Chat/Inventar korrekt) |
| **Movable Chat / Title / Bossbar / Hotbar / Day Counter / Coordinates** | ✔ | ✔ | ◐ | Wir haben nur Scoreboard. Sehr einfache, gefragte Module |
| **Clear Chat / Compact Chat** | ✔ | ✔ | ◐ | In "Chat Plus"? prüfen |
| **TNT Timer** | ✔ | ✔ | ✖ | Fehlt |
| **Light Overlay** | – | ✔ | ✖ | Fehlt |
| **Pack Changer / Quality of Life** | ✔ | (Pack Display) | ◐ | Pack Display haben wir, Pack-Wechsel fehlt |
| **Discord RPC** | ✔ | ✔ (Skript) | ✖ | Fehlt, beliebt |
| **Scripting / Marketplace** | ✔ | ✔ (Lua, bezahlt) | ✖ | Phase 9, bei Onix Hauptargument |
| Theme / Farben anpassen | ✔ | ✔ | ✔ | |
| **Crystal Optimizer** | ✔ (neu) | – | ✔ (gesperrt, server-rules) | Für Crystal-PvP. Vorher Server-Regeln prüfen (kann als Vorteil gelten), nur mit Warnhinweis |
| Stopwatch, Memory | ✔ | ✔ | ✔ | |
| Experience Info, Durability Warning | ✔ | ✔ | ✔ | |

## Stufe 3: Nützlich (unter "Mehr")

| Feature | Quelle | Wir |
|---|---|---|
| Faster Inventory, Item Use Delay Fix, Insta Hurt Animation, Null Movement | Flarial | ✔ (alle server-rules) |
| CPS Limiter | Flarial, Onix (Skript ClickLimiter) | ✔ |
| Message Logger / Chatlog | Flarial, Onix | ✔ |
| Mumble Link | Flarial | ✔ |
| Depth of Field, Deepfry, Upside Down | Flarial | ✔ (Spaß, eher Stufe 4) |
| Hue Changer | Flarial | ✔ (Saturation/Hue) |
| Glint Color | Flarial | ✔ |
| Force Coords | Flarial | ✔ (Coordinates) |
| View Model | Flarial | ✔ |
| Subtitles | Flarial, Onix | ✖ |
| Pitch Display | Flarial | ✔ (Look Angles) |
| Shader / MaterialBin Loader | Flarial | ✖ |
| Hive Stats, Zeqa Utils | Flarial | ✖ |
| Screenshot, Replay | Latite, Lunar | ◐ Screenshot+ ja, Replay nein |
| Target HUD | Onix (Skript) | ✔ |
| Armor on hotbar, Hotbar-Armor-HUD, Inventory Viewer | Onix (Skripte) | ✖ |
| Fall Trajectory, Arrow Trail, Breadcrumbs, Beacon Range | Onix (Skripte) | ✖ (Beacon Range, Slime Chunk Finder sind Welt-Hilfen, keine PvP-Module) |
| Black Bars, Camera Effects/Animations | Onix (Skripte) | ✖ |
| Left Hand | Onix (Skript) | ✖ |
| Gamemode Hotkeys | Onix (Skript) | ✖ |
| FPS Limiter | Onix (Skript) | ✔ (Frame Limiter) |
| Cosmetics, Cape Switcher | Onix | ✖ |
| Creative Tools / WorldEdit | Onix | ✖ (kein PvP) |

## Stufe 4: Extras (nicht PvP, verstecken)

Flarial: DVD Screen, Doom, 202020, Twerk, Lewis, PatarHD. Onix: Flappy Bird, Snake, 2048, Pong, Sudoku, Tetris-Klon. Wir haben dazu: Snake, Flappy Heart, DVD Screen, 20-20-20, Block Game, Pomodoro.

---

## Was bei uns überflüssig ist (gibt es bei Flarial/Onix nicht, bringt PvP-Spielern kaum etwas)

Diese Module erzeugen im PvP-Menü Rauschen. Vorschlag: nicht löschen, sondern auf **Stufe 4** setzen (unter "Extras", standardmäßig versteckt).

| Modul | Warum |
|---|---|
| Kill Effects, Hit Effects, Hit Sound, Totem Pop (Effekt) | Spielerei, kein Vorteil, keiner fragt danach |
| Match Summary, Session Stats, Hit Info, Stats HUD | Doppelt zu Combo/Reach/Hit Counter. Höchstens ein einziges "Stats" |
| Damage Indicator, Hit Marker | Java-Mods, gibt es bei Bedrock-Clients nicht. Nur Stufe 3 |
| Bow Charge, Cooldown Indicator | nett, aber selten |
| Pet, Petals, Pomodoro, Block Game, Watermark | Spaß |
| Night Shift, Sharpen, Color Filter, Brightness/Contrast, Screen Tint | Post-Effekte-Sammelsurium. Reicht: Motion Blur + Saturation/Hue + Brightness. Rest Stufe 4 |
| Background Load, Auto Profile | technisch, nicht als Modul sichtbar |

**Behalten, weil sie uns abheben (➕):** Latency Overlay, Latency Blame, Network (WLAN), Instant Input, Frame Limiter, Server Profiles, Streamer Mode, Sig Status.

## Was jetzt als Erstes gebaut werden muss (Reihenfolge)

1. **Hive Utils** (Auto-Requeue) und **Auto GG** prüfen.
2. **Animations ausbauen:** Block Hit, Swing Animations, 1.8-Look.
3. **Chat/HUD verschieben:** Movable Chat, Title, Bossbar, Hotbar, Day Counter, Coordinates; Clear/Compact Chat.
4. **Inventory Lock, Java Inventory Hotkeys, Modern Keybind Handling.**
5. **Nametag Modifier und Nick, Item Physics, TNT Timer, Light Overlay, Skin Stealer.**
6. **Pack Changer / Quality of Life, Subtitles, Discord RPC.**
7. **Scripting (Lua)** als gratis Konter zu Onix.
8. **Crystal Optimizer** erst nach Klärung der Server-Regeln.

## Menü-Umbau daraus

- Jedes Modul bekommt eine Stufe. Das Menü zeigt standardmäßig **nur Stufe 1 und 2**. Ein Schalter "Mehr Module anzeigen" blendet Stufe 3 und 4 ein.
- Kategorien aufräumen: HUD, Kamera & Sicht, Kampf, Komfort, Server, Extras.
- Im Einstellungs-Menü pro Modul feste Gruppen wie bei Flarial: **General, Text/Style, Colors, Misc**, plus **Hold Mode** und **Keybind** und die Knöpfe **Reset all** und **Reset position**.

## Umbenennungen (bekannte Namen wie bei den großen Clients)

| Alt | Neu |
|---|---|
| Instant Input | Instant Hit |
| Latency | Latency Meter |
| Latency Blame | Lag Analyzer |
| Network | Network Monitor |
| Chat Plus | Better Chat |
| Sig Status | Game Support |
| Fun (Kategorie) | Extras |

PvP-Kategorie im Menü enthält jetzt alle Kampf-Module: CPS, Keystrokes, Combo, Reach, Opponent Reach, Hit Ping, Armor/Potion HUD, Pot/Arrow/Totem Counter, Target HUD, Hitbox, Hurt Color, Animations, Block Hit, Crystal Optimizer, Toggle Sprint/Sneak, Instant Hit, Auto GG u. a.
