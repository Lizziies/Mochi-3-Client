# Module

Quellen: Flarial-Repo (flarialmc/dll, 120 Modul-Ordner, Stand Okt 2026), Onix-FAQ und onixclient.com. Dazu eigene Module, die keiner der beiden hat.

**Tier**
- `overlay` — nur ImGui + eigener Input-Hook. Braucht keine Spielsignaturen und läuft auf jeder Version. Phase 3.
- `game` — liest oder ändert Spielwerte über Signaturen/Hooks. Phase 4/6.
- `server-rules` — auf unbekannten Servern standardmäßig AUS. Auf bekannten Servern entscheidet `docs/SERVERS.md`, ob erlaubt oder gesperrt.

Zusätzlich sperrt das Server-Regel-System Module pro Server (z. B. Freelook auf The Hive). Jedes Modul bekommt Tags (`cosmetic`, `hud-self`, `info-others`, `camera`, `input`, `timing`, `chat`), siehe SERVERS.md.

**Prio** (für game-Module): 1 = vor v1.0, 2 = kurz nach v1.0, 3 = später.

Nicht gebaut: nur echte Cheats mit Kampfvorteil (siehe CLAUDE.md, Hard rules).

## Core / GUI

| Modul | Tier | Prio | Notiz |
|---|---|---|---|
| ClickGUI | overlay | – | RShift, siehe UI.md |
| HUD-Editor | overlay | – | alle HUD-Elemente verschiebbar/skalierbar, Snap-Raster |
| Theme-Editor | overlay | – | Presets + eigene Themes, Export als Code |
| Modulsuche | overlay | – | Fuzzy-Suche mit Tags |
| Notifications | overlay | – | Toasts |
| Config-Profile | overlay | – | mehrere Configs, Import/Export |
| Server-Regeln | game | 1 | sperrt verbotene Module automatisch pro Server, siehe SERVERS.md |
| Server-Profile | game | 2 | neu: Modul-Set pro Server-IP |
| Lua-Scripting | game | 3 | bei Onix kostenpflichtig, bei uns gratis |

## HUD

| Modul | Tier | Prio | Notiz |
|---|---|---|---|
| FPS | overlay | – | inkl. 1%-Low, Frametime-Graph (neu) |
| CPS | overlay | – | links/rechts getrennt |
| Keystrokes | overlay | – | WASD, Space, Maus, Animationen |
| Mouse Strokes | overlay | – | |
| Clock | overlay | – | lokal; Ingame-Zeit = game, Prio 2 |
| Stopwatch | overlay | – | |
| Memory | overlay | – | RAM des Prozesses |
| Session Timer | overlay | – | neu |
| Latenz-Overlay | overlay | – | neu, siehe INPUT.md |
| Ping Counter | game | 1 | |
| Coordinates | game | 1 | inkl. Nether-Umrechnung |
| Movable Coordinates | game | 2 | |
| Force Coordinates | game | 2 | |
| Direction HUD | game | 1 | Kompass |
| Pitch Display | game | 3 | |
| Speed Display | game | 2 | |
| Reach Counter | game | 1 | Anzeige der eigenen letzten Hit-Distanz |
| Opponent Reach | game | 2 | Anzeige |
| Combo Counter | game | 1 | |
| Hit Ping | game | 2 | |
| Armor HUD | game | 1 | mit Haltbarkeit (Onix) |
| Potion HUD | game | 1 | |
| Arrow Counter | game | 2 | |
| Pot Counter | game | 1 | |
| Totem Counter | game | 2 | |
| Item Counter | game | 2 | |
| Durability Warning | game | 2 | |
| Low Health Indicator | game | 1 | |
| Better Hunger Bar | game | 2 | Sättigung wie AppleSkin |
| Experience Info | game | 3 | |
| Entity Counter | game | 3 | |
| Waila | game | 2 | was man anschaut |
| TNT Timer | game | 2 | |
| Day Counter | game | 3 | |
| IP Display | game | 1 | |
| Pack Display | game | 3 | aktives Resource Pack (Onix) |
| Tab List | game | 1 | Java-Style |
| Java Debug Menu (F3) | game | 2 | |
| Subtitles | game | 3 | Audio-Untertitel |

## Movable Vanilla-HUD

| Modul | Tier | Prio |
|---|---|---|
| Movable HUD (alles) | game | 2 |
| Movable Chat | game | 2 |
| Movable Scoreboard | game | 1 |
| Movable Bossbar | game | 2 |
| Movable Title | game | 2 |
| Movable Hotbar | game | 2 |
| Movable Paperdoll | game | 2 |
| GUI Scale | game | 2 |
| Clear Scoreboard | game | 3 |
| Clear Chat (Hintergrund) | game | 2 |
| Compact Chat | game | 2 |

## Visuell

| Modul | Tier | Prio | Notiz |
|---|---|---|---|
| Zoom | game | 1 | smooth, Scroll-Stufen |
| FOV Changer | game | 1 | |
| Java Dynamic FOV | game | 2 | |
| Fullbright | game | 1 | |
| Freelook | game | 1 | |
| Snap Look | game | 2 | |
| Auto Perspective | game | 3 | |
| Cinematic Camera | game | 3 | |
| Motion Blur | overlay | – | Post-Effekt im Present-Hook |
| Depth of Field | game | 3 | |
| Saturation / Hue | overlay | – | Shader auf Backbuffer |
| Deepfry | overlay | – | Spaß |
| Upside Down | game | 3 | Spaß |
| Custom Crosshair | overlay | – | Editor, eigene Pixel-Crosshairs |
| Crosshair (Verhalten) | game | 2 | |
| Block Outline | game | 1 | Farbe, Dicke, Füllung |
| Break Progress | game | 2 | |
| Hitbox | game | 1 | nur sichtbare Entities |
| Hurt Color | game | 1 | |
| Glint Color | game | 2 | |
| Fog Color | game | 3 | |
| Time Changer | game | 2 | clientseitig |
| Weather Changer | game | 3 | clientseitig |
| Environment Changer | game | 3 | Onix: Himmel/Fog zusammen |
| Chunk Border | game | 2 | |
| Light Overlay | game | 3 | Onix |
| Item Physics | game | 2 | |
| Particle Multiplier | game | 3 | |
| Swing Animations | game | 2 | |
| Block Hit (visuell) | game | 2 | |
| View Model | game | 2 | |
| No Hurt Cam | game | 1 | |
| Minimal View Bobbing | game | 2 | |
| Java View Bobbing | game | 3 | |
| Nametag (eigener in F5) | game | 2 | |
| Nick (clientseitig) | game | 3 | |
| Skin Stealer | game | 2 | Skin eines anderen Spielers übernehmen, gilt bis zum Verlassen des Servers |
| Waypoints | game | 2 | mit Beam + Distanz |
| Render Options | game | 1 | Partikel/Himmel/Block-Entities aus, siehe INPUT.md |
| MaterialBin Loader | game | 3 | Shader aus Resource Packs |
| Hotbar Animations | game | 3 | |

## Steuerung & Komfort

| Modul | Tier | Prio | Notiz |
|---|---|---|---|
| Toggle Sprint | game | 1 | |
| Toggle Sneak | game | 1 | |
| Modern Keybind Handling | game | 1 | Bewegung nach Inventar/Chat korrekt fortsetzen |
| Java Inventory Hotkeys | game | 2 | |
| Inventory Lock | game | 2 | |
| Disable Mouse Wheel | overlay | – | |
| Sens Multiplier | game | 2 | |
| Bow Sensitivity | game | 3 | |
| Command Hotkey | game | 2 | |
| Text Hotkey | game | 2 | |
| Auto GG | game | 2 | Hive, Zeqa, CubeCraft, Lifeboat, Galaxite |
| Death Logger | game | 3 | |
| Message Logger | game | 3 | |
| Player Notifier | game | 3 | |
| Mumble Link | game | 3 | |
| Raw Input Buffer | game | 1 | siehe INPUT.md |
| CPS Limiter | overlay | – | begrenzt nur nach unten, ok |
| Null Movement | game | 2 | **server-rules** (Snap-Tap-ähnlich) |
| Item Use Delay Fix | game | 3 | **server-rules** |
| Faster Inventory | game | 3 | **server-rules** |
| Insta Hurt Animation | game | 3 | **server-rules** |

## Server-Utilities

| Modul | Tier | Prio |
|---|---|---|
| Hive Utils (Auto-Requeue, Stats) | game | 2 |
| Hive Statistics | game | 3 |
| Zeqa Utils | game | 3 |
| CubeCraft Utils | game | 3 | neu |

## Spaß

| Modul | Tier |
|---|---|
| Doom | overlay |
| Snake | overlay |
| Flappy Bird | overlay |
| DVD Screen | overlay |
| 20-20-20 Augen-Pause | overlay |

## Neu, gibt's bei keinem

| Modul | Tier | Prio | Was |
|---|---|---|---|
| Latenz-Overlay | overlay | – | Klick → Frame-Latenz geschätzt, Frametime-Graph |
| Frame Limiter (präzise) | overlay | – | latenzarmer Limiter statt VSync, siehe INPUT.md |
| Server-Profile | game | 2 | |
| Sig-Status | overlay | – | zeigt, welche Module auf dieser Version laufen |
| Screenshot+ | overlay | – | Screenshot ohne HUD/mit HUD per Hotkey |
| Replay-Clip | overlay | 3 | letzten 15 s Frames puffern, als GIF/MP4 speichern |
| Kill Effects (clientseitig) | game | 3 | Herzchen-Partikel bei Kill, passend zum Theme |
| Cosmetics (nur lokal) | game | 3 | Ohren/Cape nur für dich sichtbar |

## Umsetzungsstand (Session B)

Alle 135 Module sind in `dll/src/modules/Manager.cpp` registriert und mit MinGW syntaxgeprüft, im Spiel getestet ist noch keins. "Braucht" nennt die Signaturen, ohne die das Modul grau bleibt. Namen mit `fx.` sind Effekt-Kanäle, andere sind Daten-Signaturen, siehe `docs/SDK.md`. Mit Demo-Daten (Modul "Sig Status") laufen alle.

### HUD (55)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Toggle Sneak | Bewegung | nichts |
| Toggle Sprint | Bewegung | nichts |
| Chat Plus | Chat | `ChatEvents` |
| Death Logger | Chat | `LocalPlayer` |
| Background Load | Diagnose | nichts |
| Memory | Diagnose | nichts |
| Better Hunger Bar | Eigene Werte | `LocalPlayer`, `Inventory` |
| CPS | Eigene Werte | nichts |
| Clock | Eigene Werte | nichts |
| Coordinates | Eigene Werte | `LocalPlayer` |
| Day Counter | Eigene Werte | `Level` |
| Debug Menu | Eigene Werte | nichts |
| Direction HUD | Eigene Werte | `LocalPlayer` |
| Experience Info | Eigene Werte | `LocalPlayer` |
| FPS | Eigene Werte | nichts |
| Health Display | Eigene Werte | `LocalPlayer` |
| Held Item | Eigene Werte | `LocalPlayer`, `Inventory` |
| IP Display | Eigene Werte | nichts |
| Keystrokes | Eigene Werte | nichts |
| Look Angles | Eigene Werte | `LocalPlayer` |
| Low Health Indicator | Eigene Werte | `LocalPlayer` |
| Mouse Strokes | Eigene Werte | nichts |
| Pack Display | Eigene Werte | `Level`, `PackList` |
| Server Display | Eigene Werte | nichts |
| Speed Display | Eigene Werte | `LocalPlayer` |
| Stats HUD | Eigene Werte | nichts |
| Watermark | Eigene Werte | nichts |
| Scoreboard | HUD-Teile | `ScoreboardData` |
| Tab List | HUD-Teile | `TabListData` |
| Armor HUD | Inventar-Info | `LocalPlayer`, `Inventory` |
| Arrow Counter | Inventar-Info | `LocalPlayer`, `Inventory` |
| Durability Warning | Inventar-Info | `LocalPlayer`, `Inventory` |
| Item Counter | Inventar-Info | `LocalPlayer`, `Inventory` |
| Pot Counter | Inventar-Info | `LocalPlayer`, `Inventory` |
| Potion HUD | Inventar-Info | `LocalPlayer`, `Effects` |
| Totem Counter | Inventar-Info | `LocalPlayer`, `Inventory` |
| Combo Counter | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Cooldown Indicator | Kampf-Anzeigen | `LocalPlayer`, `ItemUseEvents` |
| Entity Counter | Kampf-Anzeigen | `Level`, `EntityList` |
| Hit Counter | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Hit Info | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Hit Ping | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Opponent Reach | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Reach Counter | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Session Stats | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity`, `ChatEvents` |
| Target HUD | Kampf-Anzeigen | `LocalPlayer`, `Target` |
| Waila | Kampf-Anzeigen | `LocalPlayer`, `Target` |
| Latency | Netzwerk | nichts |
| Latency Blame | Netzwerk | nichts |
| Network | Netzwerk | nichts |
| Ping Counter | Netzwerk | nichts |
| Pet | Spiele | nichts |
| Pomodoro | Timer | nichts |
| Session Timer | Timer | nichts |
| Stopwatch | Timer | nichts |

### Visuell (31)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Custom Crosshair | Crosshair | nichts |
| Auto Perspective | Kamera | `LocalPlayer`, `fx.perspective` |
| FOV Changer | Kamera | `fx.fov` |
| Freelook | Kamera | `LocalPlayer`, `fx.lookCamera`, `fx.lookTurn` |
| Java Dynamic FOV | Kamera | `LocalPlayer`, `fx.fov` |
| Minimal View Bobbing | Kamera | `fx.bobStrength` |
| No Hurt Cam | Kamera | `fx.hurtCam` |
| No View Bobbing | Kamera | `fx.viewBob` |
| Smooth Sneak | Kamera | `fx.sneakCam` |
| Zoom | Kamera | `fx.fov` |
| Animations | Modell | `fx.handMatrix` |
| Hide Hand | Modell | `fx.hideHand` |
| View Model | Modell | `fx.handMatrix` |
| Brightness / Contrast | Post-Effekte | nichts |
| Color Filter | Post-Effekte | nichts |
| Depth of Field | Post-Effekte | nichts |
| Motion Blur | Post-Effekte | nichts |
| Night Shift | Post-Effekte | nichts |
| Saturation / Hue | Post-Effekte | nichts |
| Screen Tint | Post-Effekte | nichts |
| Sharpen | Post-Effekte | nichts |
| Block Outline | Welt | `LocalPlayer`, `Target` |
| Break Progress | Welt | `LocalPlayer`, `Target` |
| Chunk Border | Welt | `LocalPlayer` |
| Environment Changer | Welt | `fx.fog` |
| Fog Color | Welt | `fx.fogColor` |
| Fullbright | Welt | `fx.gamma` |
| Time Changer | Welt | `fx.time` |
| Water Color | Welt | `fx.waterColor` |
| Waypoints | Welt | `LocalPlayer` |
| Weather Changer | Welt | `fx.rain` |

### PvP (21)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Bow Sensitivity | Eingabe | `LocalPlayer`, `fx.sensitivity` |
| CPS Limiter | Eingabe | nichts |
| Faster Inventory (SR) | Eingabe | `fx.inventoryDelay` |
| Insta Hurt Animation (SR) | Eingabe | `fx.hurtAnim` |
| Instant Input (SR) | Eingabe | nichts |
| Item Use Delay Fix (SR) | Eingabe | `fx.useDelay` |
| Null Movement (SR) | Eingabe | nichts |
| Sens Multiplier | Eingabe | `fx.sensitivity` |
| Snap Look | Eingabe | `LocalPlayer`, `fx.lookCamera` |
| Bow Charge | Kampf-Anzeigen | `LocalPlayer`, `UseState` |
| Damage Indicator | Kampf-Anzeigen | `LocalPlayer`, `AttackEntity` |
| Glint Color | Treffer-Visuals | `fx.glintColor` |
| Hit Effects | Treffer-Visuals | `LocalPlayer`, `AttackEntity` |
| Hit Marker | Treffer-Visuals | nichts |
| Hit Sound | Treffer-Visuals | `LocalPlayer`, `AttackEntity` |
| Hitbox | Treffer-Visuals | `fx.hitbox` |
| Hurt Color | Treffer-Visuals | `fx.hurtColor` |
| Kill Effects | Treffer-Visuals | `LocalPlayer`, `KillEvents` |
| Low Fire | Treffer-Visuals | `fx.fire` |
| Particle Multiplier | Treffer-Visuals | `fx.particleScale` |
| Totem Pop | Treffer-Visuals | `LocalPlayer`, `TotemEvents` |

### Komfort (11)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Command Hotkey | – | nichts |
| Text Hotkey | – | nichts |
| Mumble Link | Audio | `LocalPlayer` |
| Screenshot+ | Aufnahme | nichts |
| Auto GG | Chat | `ChatEvents` |
| Message Logger | Chat | `ChatEvents` |
| Player Notifier | Chat | `TabListData` |
| Disable Mouse Wheel | Eingabe | nichts |
| GUI Scale | HUD-Teile | `fx.guiScale` |
| Profile Hotkeys | Profile | nichts |
| Streamer Mode | Profile | nichts |

### Performance (6)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Auto Profile | Diagnose | nichts |
| Sig Status | Diagnose | nichts |
| Frame Limiter | Frame-Timing | nichts |
| Low Latency | Frame-Timing | nichts |
| System Boost | Frame-Timing | nichts |
| Render Options | Grafik | `fx.clouds`, `fx.particles`, `fx.blockEntities`, `fx.shadows`, `fx.sky`, `fx.fog`, `fx.vignette`, `fx.rain` |

### Server (2)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Server Profiles | Profile | nichts |
| Match Summary | Statistik | `LocalPlayer`, `AttackEntity` |

### Spaß (8)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| Deepfry | Post-Effekte | nichts |
| Upside Down | Post-Effekte | nichts |
| 20-20-20 | Spiele | nichts |
| Block Game | Spiele | nichts |
| DVD Screen | Spiele | nichts |
| Flappy Heart | Spiele | nichts |
| Petals | Spiele | nichts |
| Snake | Spiele | nichts |

### Client (1)

| Modul | Unterkategorie | Braucht |
|---|---|---|
| ClickGUI | – | nichts |

### Bewusst nicht gebaut

- Reach, Killaura, Aim Assist, Autoclicker, Velocity, Scaffold, ESP, Fake Lag und Paket-Manipulation (siehe `CLAUDE.md`).
- Skin Stealer, Replay-Clip, Paperdoll, Cape/Cosmetics: brauchen Zugriff auf Skin- und Render-Daten, der sich erst am PC prüfen lässt.
- Java Inventory Hotkeys, Inventory Lock/Sort, Modern Keybind Handling, Movable Hotbar/Bossbar/Title, Item Physics, Light Overlay, Subtitles, Doom, Hive Utils/Statistics, Zeqa/CubeCraft Utils, Reconnect, Server-Liste: Prio 3 oder hängen an Spielstrukturen, kommen nach den ersten echten Signaturen.
- DSCP/QoS-Markierung und Bandbreiten-Hinweis pro Programm im Netzwerk-Modul: brauchen Admin-Rechte beziehungsweise ETW, nur nach ausdrücklicher Zustimmung, später.

