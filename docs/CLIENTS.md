# Bedrock-Clients im Vergleich

Stand: Oktober 2026. Das ist die Wettbewerbs-Referenz für Monchi: was die anderen Clients können, wo sie schwach sind, was wir anders machen. Der Ausbauplan dazu steht in `docs/FEATURES.md`.

## So sicher sind die Angaben

| Kennzeichen | Bedeutung |
|---|---|
| ✔ | direkt gesehen (Ordnerliste im Flarial-Repo, Suchergebnisse, Docs) |
| ~ | aus Suchergebnissen/Wissen, nicht einzeln geprüft |
| ? | unsicher, vor dem Einbau am PC prüfen |

Von dieser Session aus waren flarial.xyz, onixclient.com, latite.net und docs.onixclient.com gesperrt. Flarial-Module stammen aus der Ordnerliste von `flarialmc/dll-oss` (`src/Client/Module/Modules`, alphabetisch gelesen bis "Sprint", ✔). Der Rest der Ordnerliste kam von einem Zusammenfassungs-Tool und enthielt erfundene Namen, deshalb ist er verworfen. Claude Code am PC (mit GitHub-Zugriff) soll die Liste einmal komplett prüfen und diese Datei korrigieren.

## Überblick

| | Flarial | Onix | Latite | Monchi (Ziel) |
|---|---|---|---|---|
| Preis | gratis | Patreon, ab ca. 4 $ (Scripting nur bezahlt) | gratis | gratis |
| Lizenz | AGPL-3.0, Code offen (`dll-oss`) | closed source | Open Source | Open Source, eigene Lizenz |
| Plattform | Windows + Android | Windows | Windows | Windows |
| Module | 140+ (Seite: 104 Windows, 52 Android) | 90+ eingebaut, 125+ von der Community (Lua) | 30+ | 135 Start, Ziel 200+ |
| Scripting | ? (Marketplace/Skripte, nicht geprüft) | Lua 5.4, nur bezahlt | JS/TS-Plugins, gratis | Lua gratis (Phase 9), später JS prüfen |
| Theme/UI | eigenes ClickGUI | Theme Editor | Akzentfarbe, Suche/Filter | volle Theme-Engine |
| Auto-Update | Launcher | Launcher | Launcher | Launcher + sigs.json ohne Release |
| Versions-Wechsel | nur aktuelle Version | ältere Builds, eigener Loader | AppX-Mod, begrenzt | Switcher im Launcher |
| Nach MC-Updates | DLL neu bauen, Tage Wartezeit | Wartezeit | Wartezeit | Signaturen als Commit |

Weitere Clients, die man kennt:

| Client | Typ | Anmerkung |
|---|---|---|
| Luconia | legit, in Entwicklung | nur alte Versionen (1.19.5x–1.19.7x), wahrscheinlich eingeschlafen ~ |
| Bedrock+ | Mod-Manager/Launcher/Client-Hybrid | angekündigt ~ |
| Project Lumen, Wiser | angekündigt | ~ |
| Astral | Resource-Pack-Optimierung | kein DLL-Client |
| Lunar / Badlion | Java | gibt es nicht für Bedrock |
| Horion, Nuke, Sunset und ähnliche | Cheat-Clients | werden bei Monchi nicht berücksichtigt, siehe Hard Rules in CLAUDE.md |

Lokaler Stand der Vergleichsliste: `Nyraxis/MCBE_Clients` auf GitHub führt nur ältere Versionen, die Szene ist schnelllebig.

## Flarial

Kostenlos, AGPL-3.0, aktiv entwickelt, Windows und Android. Stärkster Gegner bei Modulanzahl. Code ist öffentlich, wird aber bei Monchi nur gelesen, nie kopiert.

### Module laut Repo (✔, alphabetisch bis "Sprint")

202020 (Augenpause), Animations, ArrowCounter, AutoGG, AutoPerspective, BetterHungerBar, BlockBreakIndicator, BlockHit, BlockOutline, BowSensitivity, CPS, CPSLimiter, ChunkBorder, CinematicCamera, ClearChat, ClearScoreboard, ClickGUI, ComboCounter, CommandHotkey, CompactChat, Coordinates, Crosshair, CustomCrosshair, DVD Screen, DeathLogger, DebugMenu, Deepfry, DepthOfField, DirectionHUD, DisableMouseWheel, Doom, DurabilityWarning, EntityCounter, ExperienceInfo, FOVChanger, FPS, FasterInventory, FogColor, ForceCoords, Freelook, Fullbright, GlintColor, GuiScale, HitPing, Hitbox, HiveStat, HiveUtils, HueChanger, HurtColor, IPDisplay, InstantHurtAnimation, InventoryLock, ItemCounter, ItemPhysics, ItemUseDelayFix, JavaDynamicFOV, JavaInventoryHotkeys, JavaViewBobbing, Keystrokes, LowHealthIndicator, MaterialBinLoader, Memory, MessageLogger, MinimalViewBobbing, ModernKeybindHandling, MotionBlur, Mousestrokes, MovableBossbar, MovableChat, MovableCoordinates, MovableDayCounter, MovableHUD, MovableHotbar, MovableScoreboard, MovableTitle, MumbleLink, NametagModifier, Nick, NoHurtCam, NullMovement, OpponentReach, PaperDoll, ParticleMultiplier, PingCounter, PitchDisplay, PlayerNotifier, PotCounter, PotionHUD, RawInputBuffer, ReachCounter, RenderOptions, SensMultiplier, SkinStealer, SnapLook, Sneak, SpeedDisplay, Sprint.

Dahinter folgen nach Wissen/Erwartung (~, vor Einbau prüfen): Snaplook/Timer-artige Module, TabList, Time/Weather Changer, ToggleSneak/Sprint, Waila, Waypoints, Zoom und weitere. Die Gesamtzahl 140+ spricht dafür, dass ca. 40 Einträge hier noch fehlen.

### Besonderheiten

- ClickGUI mit Modul-Kacheln, Einstellungen, eigener Config-Verwaltung, Online-Configs (~).
- Deepfry-Shader, Motion Blur, Hue Changer als Bildschirm-Effekte.
- Hive-spezifische Module (HiveUtils, HiveStat).
- Android-Version mit eigenem Modulsatz (52 Module).
- Raw Input Buffer ist laut früherer Recherche im öffentlichen Code nur ein Stub (Platzhalter), kein echter Eingabepuffer.
- Changelogs mit HUD-Updates (Juni 2026), Fokus auf Stabilität nach MC-Updates.

### Schwächen (Angriffspunkte für Monchi)

- Nach jedem Minecraft-Update bricht der Client oft, bis eine neue DLL gebaut wurde.
- Kein Modul-Ausgrauen bei fehlenden Offsets, bei Fehlern kann das Spiel abstürzen (~).
- Keine Server-Regel-Automatik. Der Spieler muss selbst wissen, was auf Hive/Lifeboat/CubeCraft erlaubt ist.
- Kein Version-Switcher.
- Latenz-Optionen eher kosmetisch (Raw Input Buffer).
- Fehlende Module nach Spielerwünschen, z. B. echtes "No View Bobbing" (es gibt nur Minimal- und Java-View-Bobbing).

## Onix

Ältester großer Bedrock-Client, bezahlt (Patreon-Tiers, Scripting ab einem Supporter-Tier). Closed Source, deshalb nur über Beschreibungen bekannt.

### Eingebaute Module (~, aus Website/FAQ/Suchergebnissen)

Zoom, Fullbright, Freelook (360°), Environment Changer, Java Debug Menu, Toggle Sprint/Sneak, Auto GG, Chunk Borders, Render Options, Third Person Nametag, Block Outline/Overlay, Custom Crosshairs, Hitboxes, Waypoints, FPS Counter, Clock, Keystrokes, Coordinates, Server IP Display, CPS Counter, Direction HUD, Speed Display, Reach Display, Combo Counter, Pack Display, Armor HUD (mit Haltbarkeit), Movable Paperdoll, Flappy Bird, Snake, Potion HUD, Audio Subtitles, Player List (Tab), Hurt Color, Light Overlay, TNT Timer, Creative Tools, Item Physics, Theme Editor, Client-Side Nick, Cape Manager, Chat Logger, Cinematic Camera, Ping Display, Motion Blur (bezahlte V3), Skin Stealer (als Lua-Script).

### Besonderheiten

- Lua-Scripting (5.4) mit Downloader, über 125 Community-Module. Skripte liegen auf GitHub (`OnixClient-Scripts`).
- Theme Editor, FPS-/Rendering-Optionen.
- Layout: Modulliste links, Einstellungen rechts, weiches Scrollen. Genau dieses Gefühl will Felix nachbauen.
- Versionen: bezahlte V3 hat Zugriff auf neuere Versionen, die freie ist älter und langsamer aktualisiert ~.

### Schwächen

- Bezahlt, Scripting nur für zahlende Supporter.
- Closed Source, Wartezeit nach Updates.
- Kein offizieller Server-Regel-Schutz.

## Latite

Kostenlos, Open Source, Windows, JS/TS-Plugins mit `.plugin install/load`-Befehlen.

### Module (~, 30+)

Armor HUD, Auto GG, Behind You (Snaplook), Block Game, Block Overlay, Bow Indicator, Break Indicator, CPS Counter, Chunk Borders, Cinematic Camera, Clock, Combo Counter, Command Shortcuts, Custom Coordinates, Environment Changer, FPS Counter, Frame Time Display, Freelook, Fullbright, GUI Scale Changer, Health Warning, Hitboxes, Hurt Color, Item Counter, Java Debug Info, Keystrokes, Motion Blur, Movable Bossbar, Movable Coordinates, Movable Paperdoll, Movable Scoreboard, Nickname, No Hurt Cam, Ping Display, Player List, Position Display, Reach Display, Server Display, Speed Display, Text/Command Hotkey, Third Person Nametag, Toggle Sprint/Sneak, Zoom.

### Besonderheiten und Schwächen

- Sauberes UI mit Suche/Filter, Akzentfarbe, Keybinds, Schrift-Einstellungen.
- Plugin-Ökosystem mit Docs (`latitescripting.github.io`).
- Wenige Module, kaum Server-spezifische Logik, Wartezeit nach Updates.

## Was die anderen noch nicht (gut) haben

Das sind die Lücken, die Monchi füllt. Details und Umsetzung in `docs/FEATURES.md`.

1. Signaturen als Datei auf GitHub: Fixes ohne neues Release.
2. Fehlende Signatur = Modul grau, kein Absturz. Crash-Guard pro Hook.
3. Eingebautes Latenz-Overlay und echter Eingabe-Pfad mit Messung.
4. Präziser Frame-Limiter, Frame-Queue-Reduktion, Reflex-artiger Ablauf (soweit Bedrock es zulässt).
5. WLAN-/Netzwerk-Modul: Verbindungsqualität, Jitter, Paketverlust, Tipps und Optimierungen.
6. Automatische Server-Regeln und Server-Profile.
7. Version-Switcher im Launcher.
8. Lua-Scripting gratis.
9. Echtes "No View Bobbing" plus Optionen für Hurt-Cam, Sneak-Animation, Item-Swing und andere Kamera-Effekte.
10. Theme-Engine mit Presets und Export-Codes.

## Quellen

- Flarial-Repo `flarialmc/dll-oss`, Ordner `src/Client/Module/Modules` (✔ bis "Sprint")
- Flarial: Modules-Seite und Changelog (flarial.xyz, nur über Suchergebnisse gesehen)
- Onix: onixclient.com, Onix FAQ (noobdevrohan.github.io/OnixClientFaq), Scripting-Docs (docs.onixclient.com), `OnixClient-Scripts` auf GitHub
- Latite: `Creative-32/BedrockModSuite`, latite.net
- `Nyraxis/MCBE_Clients` (Clientliste)
