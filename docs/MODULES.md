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

Alle 171 Module sind in `dll/src/modules/Manager.cpp` registriert, bauen mit MinGW und laufen unter Wine mit den Demo-Daten (Modul "Game Support") ohne Absturz. Im echten Spiel getestet ist noch keins. "Braucht" nennt die Signaturen, ohne die das Modul grau bleibt; bei "einer von" reicht eine. Namen mit `fx.` sind Effekt-Kanäle, andere sind Daten-Signaturen, siehe `docs/SDK.md`. "Stufe" ist die Einordnung in `Tiers.cpp`, "Einst." die Zahl der sichtbaren Einstellungen. Die Tabelle entsteht mit `tools/modules_doc.py` aus einem Modul-Dump.

### HUD (63)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| Better Chat | Chat | Erwartet | 40 | `ChatEvents` | – |
| Death Logger | Chat | Erwartet | 30 | `LocalPlayer` | – |
| Combo Counter | Combat displays | Kern | 34 | `LocalPlayer`, `AttackEntity` | – |
| Cooldown Indicator | Combat displays | Extras | 28 | `LocalPlayer`, `ItemUseEvents` | – |
| Entity Counter | Combat displays | Erwartet | 27 | `Level`, `EntityList` | info-others |
| Hit Counter | Combat displays | Weitere | 30 | `LocalPlayer`, `AttackEntity` | – |
| Hit Info | Combat displays | Extras | 29 | `LocalPlayer`, `AttackEntity` | – |
| Hit Ping | Combat displays | Erwartet | 30 | `LocalPlayer`, `AttackEntity`, `ActorEvent` | info-others |
| Opponent Reach | Combat displays | Erwartet | 31 | `LocalPlayer`, `ActorList` | info-others |
| Reach Counter | Combat displays | Kern | 38 | `LocalPlayer`, `AttackEntity` | – |
| Session Stats | Combat displays | Extras | 31 | `LocalPlayer`, `AttackEntity`, `ChatEvents` | – |
| Target HUD | Combat displays | Erwartet | 37 | `LocalPlayer`, `Target` | info-others |
| Waila | Combat displays | Erwartet | 29 | `LocalPlayer`, `Target` | – |
| Background Load | Diagnostics | Extras | 29 | nichts | – |
| Memory | Diagnostics | Erwartet | 27 | nichts | – |
| Pet | Games | Extras | 30 | nichts | – |
| Movable Bossbar | HUD parts | Erwartet | 26 | `fx.bossbarOffset` | – |
| Movable Hotbar | HUD parts | Erwartet | 26 | `fx.hotbarOffset` | – |
| Movable Title | HUD parts | Erwartet | 26 | `fx.titleOffset` | – |
| Scoreboard | HUD parts | Kern | 30 | `ScoreboardData` | – |
| Subtitles | HUD parts | Erwartet | 38 | `LocalPlayer`, `SoundEvents` | – |
| Tab List | HUD parts | Erwartet | 45 | `TabListData` | info-others |
| Armor HUD | Inventory info | Kern | 38 | `LocalPlayer`, `Inventory` | – |
| Arrow Counter | Inventory info | Erwartet | 31 | `LocalPlayer`, `Inventory` | – |
| Durability Warning | Inventory info | Erwartet | 10 | `LocalPlayer`, `Inventory` | – |
| Hotbar Armor | Inventory info | Erwartet | 14 | `LocalPlayer`, `Inventory` | – |
| Inventory Viewer | Inventory info | Erwartet | 36 | `LocalPlayer`, `Inventory` | – |
| Item Counter | Inventory info | Erwartet | 35 | `LocalPlayer`, `Inventory` | – |
| Pot Counter | Inventory info | Erwartet | 33 | `LocalPlayer`, `Inventory` | – |
| Potion HUD | Inventory info | Kern | 40 | `LocalPlayer`, `Effects` | – |
| Totem Counter | Inventory info | Erwartet | 31 | `LocalPlayer`, `Inventory` | – |
| Toggle Sneak | Movement | Kern | 29 | nichts | input |
| Toggle Sprint | Movement | Kern | 31 | nichts | input |
| Lag Analyzer | Network | Weitere | 33 | nichts | – |
| Latency Meter | Network | Kern | 28 | nichts | – |
| Network Monitor | Network | Kern | 52 | nichts | – |
| Ping Counter | Network | Kern | 31 | nichts | – |
| Better Hunger Bar | Own values | Erwartet | 33 | `LocalPlayer`, `Inventory` | – |
| CPS | Own values | Kern | 27 | nichts | – |
| Clock | Own values | Erwartet | 33 | nichts | – |
| Coordinates | Own values | Kern | 43 | `LocalPlayer` | – |
| Day Counter | Own values | Erwartet | 30 | `Level` | – |
| Debug Menu | Own values | Erwartet | 25 | nichts | – |
| Direction HUD | Own values | Erwartet | 43 | `LocalPlayer` | – |
| Experience Info | Own values | Erwartet | 29 | `LocalPlayer` | – |
| FPS | Own values | Kern | 28 | nichts | – |
| Fall Predictor | Own values | Erwartet | 32 | `LocalPlayer` | – |
| Health Display | Own values | Weitere | 32 | `LocalPlayer` | – |
| Held Item | Own values | Weitere | 32 | `LocalPlayer`, `Inventory` | – |
| IP Display | Own values | Erwartet | 30 | nichts | – |
| Keystrokes | Own values | Kern | 62 | nichts | – |
| Look Angles | Own values | Weitere | 29 | `LocalPlayer` | – |
| Low Health Indicator | Own values | Erwartet | 6 | `LocalPlayer` | – |
| Mouse Strokes | Own values | Erwartet | 28 | nichts | – |
| Pack Display | Own values | Weitere | 26 | `Level`, `PackList` | – |
| Paperdoll | Own values | Erwartet | 33 | `LocalPlayer`, `Inventory` | – |
| Server Display | Own values | Erwartet | 28 | nichts | – |
| Speed Display | Own values | Erwartet | 31 | `LocalPlayer` | – |
| Stats HUD | Own values | Extras | 38 | nichts | – |
| Watermark | Own values | Extras | 31 | nichts | – |
| Pomodoro | Timer | Extras | 38 | nichts | – |
| Session Timer | Timer | Erwartet | 27 | nichts | – |
| Stopwatch | Timer | Erwartet | 28 | nichts | – |

### Visuell (41)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| Auto Perspective | Camera | Erwartet | 6 | `fx.perspective`, `LocalPlayer` | – |
| Black Bars | Camera | Erwartet | 6 | nichts | – |
| Cinematic Camera | Camera | Erwartet | 4 | `fx.lookDelta` | – |
| FOV Changer | Camera | Kern | 5 | `fx.fov` | – |
| Freelook | Camera | Kern | 5 | `fx.lookCamera`, `fx.lookTurn`, `LocalPlayer` | – |
| Java Dynamic FOV | Camera | Erwartet | 6 | `fx.fov`, `LocalPlayer` | – |
| Minimal View Bobbing | Camera | Erwartet | 3 | `fx.bobStrength` | – |
| No Hurt Cam | Camera | Kern | 2 | `fx.hurtCam` | – |
| No View Bobbing | Camera | Kern | 4 | `fx.viewBob` | – |
| Smooth Sneak | Camera | Weitere | 2 | `fx.sneakCam` | – |
| Zoom | Camera | Kern | 15 | `fx.fov` | – |
| Custom Crosshair | Crosshair | Kern | 37 | nichts | – |
| Hotbar Animation | HUD parts | Erwartet | 12 | `LocalPlayer`, `Inventory` | – |
| Animations | Model | Kern | 7 | einer von `fx.handMatrix`, `fx.swingSpeed` | – |
| Hide Hand | Model | Erwartet | 4 | `fx.hideHand` | – |
| Left Hand | Model | Erwartet | 5 | `fx.handMatrix` | – |
| View Model | Model | Erwartet | 14 | einer von `fx.handMatrix`, `fx.itemFov`, `fx.handMatrixThird` | – |
| Brightness / Contrast | Post effects | Extras | 5 | nichts | – |
| Color Filter | Post effects | Extras | 3 | nichts | – |
| Depth of Field | Post effects | Extras | 2 | nichts | – |
| Motion Blur | Post effects | Erwartet | 6 | nichts | – |
| Night Shift | Post effects | Extras | 6 | nichts | – |
| Saturation / Hue | Post effects | Erwartet | 6 | nichts | – |
| Screen Tint | Post effects | Extras | 7 | nichts | – |
| Sharpen | Post effects | Extras | 2 | nichts | – |
| Arrow Trail | World | Erwartet | 15 | `LocalPlayer`, `ProjectileList` | info-others |
| Block Outline | World | Kern | 10 | `LocalPlayer`, `Target` | – |
| Break Progress | World | Erwartet | 9 | `LocalPlayer`, `Target` | – |
| Chunk Border | World | Erwartet | 10 | `LocalPlayer` | – |
| Environment Changer | World | Erwartet | 5 | `fx.fog` | – |
| Fog Color | World | Weitere | 4 | `fx.fogColor` | – |
| Fullbright | World | Kern | 4 | `fx.gamma` | – |
| Item Physics | World | Erwartet | 5 | `fx.itemPhysics` | – |
| Light Overlay | World | Erwartet | 12 | `LocalPlayer`, `LightLevels` | info-others |
| Nametag Modifier | World | Erwartet | 4 | einer von `fx.nametagText`, `fx.nametagBackground` | – |
| TNT Timer | World | Erwartet | 13 | `Target`, `TargetFuse` | – |
| Third Person Nametag | World | Erwartet | 2 | `fx.selfNametag`, `LocalPlayer` | – |
| Time Changer | World | Erwartet | 4 | `fx.time` | – |
| Water Color | World | Weitere | 4 | `fx.waterColor` | – |
| Waypoints | World | Erwartet | 12 | `LocalPlayer` | – |
| Weather Changer | World | Erwartet | 3 | `fx.rain` | – |

### PvP (24)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| Bow Charge | Combat displays | Extras | 9 | `LocalPlayer`, `UseState` | – |
| Damage Indicator | Combat displays | Extras | 11 | `LocalPlayer`, `AttackEntity` | info-others |
| Crystal Optimizer | Crystal PvP | Erwartet | 7 | einer von `fx.crystalSimple`, `fx.crystalNoBase`, `fx.crystalHide`, `fx.ghostRender` | Warnhinweis, info-others, timing |
| Instant Hit | Crystal PvP | Kern | 4 | nichts | Warnhinweis, input, timing |
| Block Hit | Hit feedback | Kern | 3 | `fx.handMatrix` | – |
| Insta Hurt Animation | Hit feedback | Weitere | 3 | `fx.hurtAnim` | Warnhinweis, timing |
| Kill Cleanup | Hit feedback | Erwartet | 6 | `fx.ghostRender`, `KillEvents` | Warnhinweis, info-others, timing |
| Particle Multiplier | Hit feedback | Erwartet | 3 | einer von `fx.particleScale`, `fx.critParticle` | – |
| Glint Color | Hit visuals | Weitere | 4 | `fx.glintColor` | – |
| Hit Effects | Hit visuals | Extras | 13 | `LocalPlayer`, `AttackEntity` | – |
| Hit Marker | Hit visuals | Extras | 11 | nichts | – |
| Hit Sound | Hit visuals | Extras | 5 | `LocalPlayer`, `AttackEntity` | – |
| Hitbox | Hit visuals | Erwartet | 15 | `fx.hitbox` | info-others |
| Hurt Color | Hit visuals | Kern | 5 | `fx.hurtColor` | – |
| Kill Effects | Hit visuals | Extras | 5 | `LocalPlayer`, `KillEvents` | – |
| Low Fire | Hit visuals | Weitere | 2 | `fx.fire` | – |
| Totem Pop | Hit visuals | Extras | 7 | `LocalPlayer`, `TotemEvents` | – |
| Bow Sensitivity | Input | Erwartet | 3 | `fx.sensitivity`, `LocalPlayer` | input |
| CPS Limiter | Input | Weitere | 3 | nichts | input |
| Faster Inventory | Input | Weitere | 2 | `fx.inventoryDelay` | Warnhinweis, timing |
| Item Use Delay Fix | Input | Weitere | 2 | `fx.useDelay` | Warnhinweis, timing |
| Null Movement | Input | Weitere | 8 | nichts | Warnhinweis, input |
| Sens Multiplier | Input | Erwartet | 7 | `fx.sensitivity` | input |
| Snap Look | Input | Erwartet | 5 | `fx.lookCamera`, `LocalPlayer` | – |

### Komfort (19)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| Mumble Link | Audio | Weitere | 3 | `LocalPlayer` | – |
| Screenshot+ | Capture | Erwartet | 11 | nichts | – |
| Auto GG | Chat | Kern | 10 | `ChatEvents` | chat |
| Command Hotkey | Chat | Erwartet | 14 | nichts | chat |
| Gamemode Hotkeys | Chat | Erwartet | 8 | nichts | chat |
| Message Logger | Chat | Weitere | 6 | `ChatEvents` | – |
| Nick | Chat | Erwartet | 6 | nichts | – |
| Player Notifier | Chat | Erwartet | 4 | `TabListData` | info-others |
| Text Hotkey | Chat | Erwartet | 14 | nichts | chat |
| GUI Scale | HUD parts | Kern | 2 | `fx.guiScale` | – |
| Disable Mouse Wheel | Input | Erwartet | 3 | nichts | input |
| Inventory Lock | Inventory | Erwartet | 7 | `Inventory` | input |
| Java Inventory Hotkeys | Inventory | Erwartet | 1 | nichts | input |
| Hotbar Keys | Movement | Weitere | 10 | nichts | input |
| Modern Keybind Handling | Movement | Erwartet | 9 | nichts | input |
| Pack Changer | Packs | Erwartet | 1 | nichts | – |
| Skin Stealer | Packs | Erwartet | 3 | `Target`, `TargetSkin` | – |
| Profile Hotkeys | Profiles | Weitere | 9 | nichts | – |
| Streamer Mode | Profiles | Erwartet | 4 | nichts | – |

### Performance (6)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| Auto Profile | Diagnostics | Extras | 3 | nichts | – |
| Game Support | Diagnostics | Weitere | 3 | nichts | – |
| Frame Limiter | Frame timing | Kern | 8 | nichts | – |
| Low Latency | Frame timing | Kern | 5 | nichts | – |
| System Boost | Frame timing | Weitere | 4 | nichts | – |
| Render Options | Graphics | Kern | 14 | einer von `fx.clouds`, `fx.particles`, `fx.blockEntities`, `fx.shadows`, `fx.sky`, `fx.fog`, `fx.vignette`, `fx.rain`, `fx.renderEntities`, `fx.renderTerrain`, `fx.hideHand`, `fx.hideHud` | – |

### Server (6)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| Server Profiles | Profiles | Erwartet | 4 | nichts | – |
| Match Summary | Statistics | Extras | 5 | `LocalPlayer`, `AttackEntity` | – |
| Hive Leaderboard | The Hive | Erwartet | 32 | nichts | info-others |
| Hive Stats | The Hive | Erwartet | 59 | `TabListData` | info-others |
| Hive Utils | The Hive | Kern | 67 | `ChatEvents`, `ScoreboardData` | timing, chat |
| Zeqa Utils | Zeqa | Erwartet | 25 | `ChatEvents` | timing, chat |

### Spaß (8)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| 20-20-20 | Games | Extras | 2 | nichts | – |
| Block Game | Games | Extras | 6 | nichts | – |
| DVD Screen | Games | Extras | 3 | nichts | – |
| Flappy Heart | Games | Extras | 1 | nichts | – |
| Petals | Games | Extras | 8 | nichts | – |
| Snake | Games | Extras | 1 | nichts | – |
| Deepfry | Post effects | Extras | 4 | nichts | – |
| Upside Down | Post effects | Extras | 3 | nichts | – |

### Client (4)

| Modul | Unterkategorie | Stufe | Einst. | Braucht | Hinweis |
|---|---|---|---|---|---|
| ClickGUI | – | Weitere | 1 | nichts | – |
| Config Sharing | Platform | Erwartet | 2 | nichts | – |
| Discord Rich Presence | Platform | Erwartet | 10 | nichts | – |
| Lua Scripts | Platform | Erwartet | 3 | nichts | – |

### Bewusst nicht gebaut

- Reach, Killaura, Aim Assist, Autoclicker, Velocity, Scaffold, ESP, Fake Lag, Paket-Manipulation, FPS- und Ping-Spoof (siehe `CLAUDE.md`).
- Eigene 2D-Hitboxen und eigene Boxen über den Bildschirm: sie würden durch Wände zeigen. Hitbox nutzt deshalb nur den Zeichenweg des Spiels.
- Item-Texturen in Zählern und Replay-Clip: brauchen Spieldateien beziehungsweise Render-Zugriff, den der Client nicht hat.
- DSCP/QoS-Markierung und Bandbreiten-Hinweis pro Programm im Netzwerk-Modul: brauchen Admin-Rechte beziehungsweise ETW, nur nach ausdrücklicher Zustimmung, später.
