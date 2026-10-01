# Mochi: Feature-Spezifikation

Der volle Ausbauplan. Alles, was Flarial, Onix, Latite und Lunar (das Java-Vorbild "Luna") können, plus das, was keiner hat. Die Wettbewerbsliste steht in `docs/CLIENTS.md`, die kurze Modulübersicht mit Tiers in `docs/MODULES.md`, die Latenz-Technik in `docs/INPUT.md`.

Umsetzungsstand je Modul (Session B): `docs/MODULES.md`, Abschnitt "Umsetzungsstand". Spiel-Schnittstelle: `docs/SDK.md`.

Leitregel (siehe `CLAUDE.md`): Ein Modul ist erst fertig, wenn es das beste existierende Gegenstück übertrifft: mehr Optionen, glatter, schöner, schneller.

**Spalten:** `Prio` 1 = vor v1.0, 2 = kurz danach, 3 = später. `Tier`: `ov` = Overlay (keine Spiel-Signatur nötig), `game` = braucht Signaturen. Tags (`cosmetic`, `hud-self`, `info-others`, `camera`, `input`, `timing`, `chat`) wie in `SERVERS.md`. `SR` = `server-rules`, standardmäßig aus und mit Warnhinweis.

**Legit-Grenze:** Nichts, was einen Kampfvorteil über das hinaus gibt, was der normale Client kann. Kein Reach, Killaura, Aim Assist, Autoclicker, Velocity, Scaffold, ESP, Fake Lag/Lag Switch, Paket-Manipulation. Ausdrücklich nicht gebaut, auch wenn andere Clients es haben.

---

## 1. Latenz & Netzwerk (das Alleinstellungsmerkmal)

### 1.1 Wo die Verzögerung wirklich herkommt

Zwischen Klick und sichtbarem Treffer liegen mehrere Stufen:

1. Maus/Tastatur → Windows (Hardware, USB-Polling): nicht beeinflussbar.
2. Windows → Spiel: Raw Input, Message-Queue, wann das Spiel den Input liest. **Beeinflussbar.**
3. Spiel verarbeitet den Input im nächsten Frame/Tick. **Beeinflussbar** (Reihenfolge, Pufferung).
4. Frame-Queue → GPU → Bildschirm (VSync, Render-Ahead, Swapchain-Puffer). **Stark beeinflussbar.**
5. Spiel → Netzwerkkarte → Router → Internet → Server (Ping). **Nur lokaler Teil beeinflussbar** (WLAN, Priorisierung, Hintergrundverkehr).
6. Server-Tick (Bedrock-Server: 20 Tick/s = 50 ms Raster) und Antwort. **Nicht beeinflussbar.**

**Ehrlich:** Der Ping zum Server und der Server-Tick sind kein Client-Thema. Ein Client kann sie nicht kleiner machen, und wer behauptet, einen "Server-Delay-Bypass" zu haben, verkauft entweder Cheats (Fake Lag, Paket-Manipulation, bannbar) oder Placebo. Mochi optimiert alles, was lokal wirklich Zeit kostet (Stufen 2 bis 5), und macht den Rest sichtbar: Ping, Jitter und Paketverlust werden gemessen und angezeigt, damit man weiß, wo das Problem liegt.

### 1.2 Latency Stack (Details in `INPUT.md`)

| Feature | Was es tut | Prio |
|---|---|---|
| Latenz-Overlay | Raw-Input-Zeitstempel → nächster Present, Frametime-Graph, 1%-Low, Frame-Queue-Tiefe, Overlay-Kosten pro Frame | 1 |
| Frame-Queue-Reduktion | `SetMaximumFrameLatency(1)`, Waitable-Swapchain nutzen, wenn vorhanden | 1 |
| Reflex-artiger Ablauf | Vor dem Input-Sampling warten, dann Input lesen, dann rendern | 1 |
| Präziser Frame-Limiter | Eigener Limiter im Present-Hook mit hochauflösendem Timer + kurzem Spin, statt VSync/Cap | 1 |
| Tearing erlauben | `ALLOW_TEARING` + SyncInterval 0, nur bei VSync aus | 2 |
| Raw Input Buffer (echt) | Alle `WM_INPUT`-Pakete per `GetRawInputBuffer` einsammeln, mit QPC-Zeitstempel, nichts verlieren, nichts erfinden | 1 |
| System-Tweaks | `timeBeginPeriod(1)`, Priorität "Über normal", Power-Throttling aus, Render-Thread leicht höher. Beim Entladen zurücksetzen | 1 |
| Render Options Preset "PvP Max FPS" | Partikel, Himmel, Wolken, Block-Entities, Schatten, Wetter einzeln abschaltbar | 1 |
| Overlay billig halten | Cache der Vertex-Buffer, Blur nur bei offenem Menü, Ziel < 0,3 ms | 1 |
| A/B-Test-Modus | Messung vor/nachher pro Option, Ergebnis als Tabelle, damit nur wirkt, was gemessen wurde | 2 |
| Flash-Test | Weißes Quadrat beim Klick für Hochgeschwindigkeitskamera-Messung | 3 |

### 1.3 Instant Hit Input

Name im Client: **Instant Input**. Ziel: Klick und Treffer-Feedback so früh wie möglich, ohne etwas zu tun, was der normale Client nicht tun könnte.

| Teil | Beschreibung |
|---|---|
| Sofort-Sampling | Klick wird beim Eintreffen (`WM_INPUT`) mit Zeitstempel erfasst und im nächsten Spiel-Input-Fenster eingespeist, nicht erst nach einem festen Tick-Warten |
| Reihenfolge | Mehrere Klicks zwischen zwei Ticks bleiben in richtiger Reihenfolge erhalten, kein Klick geht verloren |
| Sofort-Feedback | Schwung-Animation und Treffer-Sound lokal ohne Verzögerung (Flarial hat ein ähnliches Modul als "Instant Hurt Animation") |
| Hit Feedback Latenz | Zeigt die gemessene Zeit Klick → Animation im Latenz-Overlay |
| Keine zusätzlichen Treffer | Es werden **keine** Angriffe erzeugt, die der Spieler nicht geklickt hat, und Cooldowns/Reichweite bleiben vanilla |
| Tags | `input`, `timing`, **SR**. Auf Lifeboat und Galaxite gesperrt, auf Hive/CubeCraft Hinweis (siehe `SERVERS.md`) |

Das Modul wird mit dem Latenz-Overlay gemessen. Was nichts bringt, wird entfernt.

### 1.4 WLAN- und Netzwerk-Modul

Viele Spieler sind im WLAN. WLAN verursacht keinen höheren Durchschnittsping, aber Jitter (schwankende Latenz) und Paketverlust, und das fühlt sich im PvP schlimmer an als ein konstant hoher Ping. Das Modul macht das messbar und hilft, es zu verbessern.

| Funktion | Beschreibung | Prio |
|---|---|---|
| Verbindungsart erkennen | WLAN oder LAN (über `GetAdaptersAddresses`/WLAN-API), Band (2,4/5/6 GHz), Link-Speed, Signalstärke (RSSI in %/dBm) | 1 |
| Ping/Jitter/Loss HUD | Eigene Probe zum erkannten Server (UDP/ICMP im Hintergrund, kein Spielverkehr verändert): Ping, Jitter, Paketverlust, Graph | 1 |
| Ampel-Anzeige | Grün/Gelb/Rot für Stabilität, mit Grund ("WLAN-Signal schwach", "Jitter hoch, vermutlich Funk-Störung") | 1 |
| Hintergrund-Scan-Erkennung | Erkennt Ping-Spitzen im Takt von WLAN-Scans und warnt | 2 |
| Energiesparen-Check | Prüft, ob der WLAN-Adapter Energiesparen/Power-Throttling nutzt, und zeigt, wo man es abschaltet (nur Hinweis) | 1 |
| DSCP-Markierung (QoS) | Optional: Minecraft-UDP-Verkehr per Windows-QoS-API (qWAVE/`SetSockOpt`) als Spielverkehr markieren, damit Router mit WMM/QoS ihn bevorzugen. Verändert keine Paketinhalte | 2 |
| Bandbreiten-Hinweis | Zeigt Hintergrund-Programme mit hohem Netzwerkverkehr (Update, Cloud-Sync, Downloads) als Hinweis, beendet nichts selbst | 2 |
| Empfehlungen | Konkrete Tipps nach Messung: 5-GHz-Band, Router-Position, LAN-Kabel, Router-QoS, 20-MHz-Kanal | 1 |
| Verlauf | Speichert Messungen pro Server und Tageszeit, zeigt Trends | 3 |
| Server-Vergleich | Misst Ping zu bekannten Servern/Regionen, empfiehlt den besten (z. B. Hive-Region) | 3 |

Grenzen: Das Modul verändert nichts am Spielprotokoll, erzeugt kein künstliches Lag und umgeht keinen Server. Systemänderungen (Adapter-Einstellungen, QoS-Richtlinien) macht es nur nach ausdrücklicher Zustimmung und nur bei Bedarf mit Admin-Rechten.

### 1.4a Anzeige "Wer ist schuld?"

Ein Panel, das Verzögerung in Anteile aufteilt: Eingabe, Rendering, Netzwerk, Server. Beispiel: "Dein Treffer-Feedback braucht 38 ms. 4 ms Eingabe, 11 ms Rendering, 23 ms Ping. Der Rest liegt am Server-Tick." Das ist ein Alleinstellungsmerkmal, das die anderen nicht haben. Prio 2.

---

## 2. Kern, Menü, UI

| Feature | Umfang | Besser als | Prio |
|---|---|---|---|
| ClickGUI | Rechts-Shift, Kartenraster, Sidebar mit Kategorien und Unterkategorien, Suche mit Tags | Onix-Layout, aber mit Unterkategorien | 1 |
| Layout Modulliste links / Einstellungen rechts | Wie Onix, umschaltbar auf Kartenraster | – | 1 |
| Smooth Scroll | Trägheit, schlanke Scrollleiste | Onix | 1 |
| Karten-Animationen | Gestaffeltes Einblenden, Hover-Lift, Kreuzblende zwischen Seiten | Onix, Flarial | 1 |
| Federnde Toggles, Slider mit Wert-Bubble, Tooltips | Alle Controls animiert | alle | 1 |
| Modulsuche | Fuzzy-Suche, Tags, Filter nach Kategorie, Verfügbarkeit, Server-Regel | Latite | 1 |
| Themes | 5 Presets (Bubblegum, Sakura, Lavender, Strawberry Milk, Midnight Pink), alle Farben, Eckenradius, Blur, Deckkraft, Schrift, Animationsgeschwindigkeit, Glitzer an/aus | Onix Theme Editor | 1 |
| Theme-Export/Import | Als Code zum Teilen | – | 1 |
| HUD-Editor | Ziehen, Mausrad-Größe, Snap an Rändern/Mitte/anderen Elementen, Ausrichtung, Rechtsklick-Einstellungen | alle | 1 |
| HUD-Gruppen | Mehrere HUD-Module als Block bewegen und stapeln | neu | 2 |
| HUD-Ebenen | Reihenfolge, Sichtbarkeit pro Situation (Kampf, Menü, F5) | neu | 2 |
| Toasts | Benachrichtigungen mit Typ und Verlauf | Onix | 1 |
| Config-Profile | Mehrere Profile, Import/Export, Auto-Wechsel pro Server | Flarial, Onix | 1 |
| Modul-Keybinds | Pro Modul, auch Mausseitentasten, Kombinationen | alle | 1 |
| Eject-Hotkey | Strg+L entlädt sauber | – | 1 |
| Sig-Status-Seite | Zeigt, welche Module auf dieser Version laufen und warum nicht | neu | 1 |
| Onboarding | Erster Start: Profile wählen (Ranked PvP, Casual, Performance), Basismodule an | neu | 2 |
| Sprache | DE/EN | – | 2 |
| Eigene Schrift | Wählbar, Nunito eingebettet | Onix | 2 |
| Zugänglichkeit | Hoher Kontrast, große Schrift, Reduzierte Bewegung (Animationen aus) | neu | 2 |

---

## 3. HUD (eigene Werte)

| Modul | Optionen (Auswahl) | Tier | Prio |
|---|---|---|---|
| FPS | Aktualisierungsrate, 1%-Low, Min/Max, Frametime-Graph, Farbstufen nach Wert | ov | 1 |
| CPS | Links/Rechts getrennt, nur kombiniert, Farbverlauf nach Wert | ov | 1 |
| Keystrokes | WASD, Space, Shift, Maustasten, CPS in Tasten, Animation (Druck, Fade), eigenes Layout, Ripple, Tastenfarben | ov | 1 |
| Mouse Strokes | Mausbewegung als Linie/Spur, Länge, Farbe | ov | 2 |
| Clock | 12/24 h, Sekunden, Datum, Zeitzone, ingame Zeit (game, Prio 2) | ov | 1 |
| Stopwatch / Timer | Start/Stopp/Runde per Taste | ov | 2 |
| Session Timer | Spielzeit, Pausen | ov | 1 |
| Memory | RAM und VRAM des Prozesses | ov | 2 |
| Ping | Aktueller Ping, Jitter, Verlust, Graph | game/ov | 1 |
| Server Display | Name/IP des Servers, Region, Spielerzahl wenn bekannt | game | 1 |
| Coordinates | XYZ, Nachkommastellen, Block/Chunk, Nether-Umrechnung, Biom | game | 1 |
| Movable/Force Coordinates | Eigene Position, erzwungen auch wo der Server sie verbirgt | game | 2 |
| Direction HUD | Kompass, Himmelsrichtung, Grad, Yaw/Pitch | game | 1 |
| Speed Display | Blöcke/s, nur horizontal, Durchschnitt | game | 2 |
| Pitch/Yaw Display | Blickwinkel | game | 3 |
| Armor HUD | Haltbarkeit als Zahl/Balken/Prozent, Farbstufen, Ausrichtung, Offhand | game | 1 |
| Potion HUD | Restzeit, Icon, Sortierung, Farben, Blinken bei Ablauf | game | 1 |
| Pot Counter | Heiltränke im Inventar | game | 1 |
| Arrow Counter | Pfeile im Inventar | game | 2 |
| Totem Counter | Totems im Inventar | game | 2 |
| Item Counter | Beliebiges Item zählen | game | 2 |
| Durability Warning | Warnung bei niedriger Haltbarkeit, Ton, Schwelle | game | 2 |
| Low Health Indicator | Bildschirmrand-Effekt bei wenig Leben | game | 1 |
| Better Hunger Bar | Sättigung wie AppleSkin | game | 2 |
| Experience Info | Level, Prozent, nächstes Level | game | 3 |
| Day Counter | Spieltag, Movable | game | 3 |
| IP Display | Server-IP, ausblendbar für Streamer | game | 1 |
| Pack Display | Aktives Resource Pack | game | 3 |
| Tab List | Java-Style, Spaltenzahl, Ping, Farben | game | 1 |
| Java Debug Menu (F3) | Koordinaten, Chunk, Licht, Biom, FPS, Speicher | game | 2 |
| Subtitles | Audio-Untertitel mit Richtung | game | 3 |
| Target HUD | Gesundheit/Rüstung des anvisierten Spielers, nur normal sichtbare Info | game | 2 |
| Stats HUD | Frei kombinierbare Zeilen aus allen Werten | ov | 2 |
| Watermark | Client-Logo, abschaltbar | ov | 3 |

---

## 4. PvP / Kampf

Aufgeteilt in Unterkategorien, damit die Kategorie nicht nur "CPS" enthält (Felix' Wunsch).

### 4.1 Kampf-Anzeigen

| Modul | Beschreibung | Tags | Tier | Prio |
|---|---|---|---|---|
| Reach Counter | Eigene Trefferdistanz der letzten Schläge | `hud-self` | game | 1 |
| Opponent Reach | Trefferdistanz des Gegners | `info-others` | game | 2 |
| Combo Counter | Treffer in Folge, Rekord, Reset bei Treffer | `hud-self` | game | 1 |
| Hit Ping | Ping im Moment des Treffers | `info-others` | game | 2 |
| Hit Counter | Treffer pro Runde, Trefferquote | `hud-self` | game | 2 |
| Damage Indicator | Schadenszahlen über dem Gegner (nur sichtbare Info) | `info-others` | game | 2 |
| Kill/Death/Streak | Session-Statistik, Streak-Anzeige | `hud-self` | game | 2 |
| Crit/Sprint-Hit Anzeige | Zeigt, ob der Schlag ein Crit war | `hud-self` | game | 3 |
| Bow Charge Indicator | Ladezeit des Bogens | `hud-self` | game | 2 |
| Block Hit | Animation beim Blocken (nur visuell) | `cosmetic` | game | 2 |
| Cooldown Indicator | Angriffs-/Perl-/Apfel-Cooldowns | `hud-self` | game | 2 |
| Entity Counter | Anzahl Entities in der Nähe (Hive: warn) | `info-others` | game | 3 |

### 4.2 Treffer-Visuals

| Modul | Beschreibung | Prio |
|---|---|---|
| Hitbox | Sichtbare Hitboxen normaler Entities, nicht durch Wände, Farbe/Dicke | 1 |
| Hurt Color | Farbe beim Treffer, Intensität, Dauer | 1 |
| Hit Effects | Partikel/Flash beim Treffer, nur lokal | 3 |
| Glint Color | Farbe des Verzauberungs-Glanzes | 2 |
| Swing Animations | Eigene Schwunganimationen (1.8-Look, klein, schnell) | 2 |
| Hit Sound | Eigener Treffer-Sound, nur lokal | 3 |
| Low Fire / No Fire | Weniger Bildschirmfeuer | 2 |
| Totem Pop Hinweis | Visuelle/akustische Anzeige beim Totem | 3 |
| Particle Multiplier | Mehr/weniger Partikel | 3 |

### 4.3 Eingabe im Kampf

| Modul | Beschreibung | Tags | Prio |
|---|---|---|---|
| CPS Limiter | Begrenzt nur nach unten | `input` | 1 |
| Instant Input | siehe 1.3 | `input`, `timing`, **SR** | 1 |
| Null Movement | Neutralisiert gegensätzliche Tasten | `input`, **SR** | 2 |
| Item Use Delay Fix | Reduziert Verzögerung beim Item-Nutzen | `timing`, **SR** | 3 |
| Faster Inventory | Schnelleres Inventarwechseln | `timing`, **SR** | 3 |
| Insta Hurt Animation | Treffer-Animation sofort | `timing`, **SR** | 3 |
| Bow Sensitivity | Eigene Empfindlichkeit beim Bogenspannen | `input` | 3 |
| Sens Multiplier | Empfindlichkeit pro Situation (Zoom, Bogen, Block) | `input` | 2 |
| Snap Look | Kurzer Blick nach hinten per Taste | `camera` | 2 |
| Hotbar-Swap-Tasten | Eigene Tasten für Slots, Item-Tausch | `input` | 3 |

### 4.4 Gegner-Info (nur normal sichtbare Daten)

| Modul | Beschreibung | Prio |
|---|---|---|
| Nametag | Eigener F5-Nametag, Third Person Nametag | 2 |
| Waila | Was man anschaut (Block/Entity-Name) | 2 |
| Player Notifier | Hinweis, wenn bestimmte Namen joinen | 3 |
| Pot/Armor der Gegner | **Nicht gebaut**, wenn die Info nur über Server-Pakete jenseits der normalen Anzeige kommt | – |

---

## 5. Visuell

### 5.1 Kamera

| Modul | Optionen | Prio |
|---|---|---|
| Zoom | Smooth, Scroll-Stufen, Taste halten/toggeln, Sensitivität, Cinematic während Zoom | 1 |
| FOV Changer | Wert, FOV-Effekte aus (Sprint, Trank), Animation | 1 |
| Java Dynamic FOV | FOV-Änderung nach Java-Vorbild | 2 |
| Freelook | Kamera frei bewegen, Spieler-Ausrichtung bleibt, 360°, Taste halten/toggeln. **Hive: gesperrt** | 1 |
| **No View Bobbing** | Eigenständiges Modul: Wackeln beim Laufen aus, getrennt für Kamera und Hand, Unterwasser-Bobbing, Sprint-Bobbing | 1 |
| Minimal View Bobbing | Abgeschwächtes Bobbing, Stärke regelbar | 2 |
| Java View Bobbing | Bobbing wie in Java | 3 |
| No Hurt Cam | Kein Wackeln beim Treffer, Stärke regelbar | 1 |
| Cinematic Camera | Weiche Mausbewegung | 3 |
| Auto Perspective | Perspektive wechselt automatisch (Elytra, Bogen) | 3 |
| Tripod/Shoulder Surf | **Nicht gebaut** (CubeCraft verbietet Shoulder Surfing) | – |
| Camera Smoothing | Glättung der Kamerabewegung | 3 |
| Sneak-Kamera ohne Ruck | Keine Höhenänderung/Ruck beim Sneaken | 2 |

### 5.2 Welt

| Modul | Optionen | Prio |
|---|---|---|
| Fullbright | An/Aus, Stufe, Nachtsicht-Effekt | 1 |
| Block Outline | Farbe, Dicke, Füllung, Verlauf, Animation | 1 |
| Break Progress | Bruchfortschritt als Zahl/Balken | 2 |
| Time Changer | Clientseitig, Zeit festlegen | 2 |
| Weather Changer | Clientseitig, Wetter aus | 3 |
| Environment Changer | Himmel, Nebel, Wolken zusammen | 3 |
| Fog Color | Nebelfarbe | 3 |
| Water Color | Wasserfarbe | 3 |
| Chunk Border | Chunk-Grenzen anzeigen | 2 |
| Light Overlay | Lichtlevel-Anzeige auf Blöcken | 3 |
| Item Physics | Dropped Items physikalisch | 2 |
| Clear Glass | Glasscheiben-Rahmen ausblenden, klare Sicht | 3 |
| Waypoints | Marker mit Beam und Distanz, Gruppen, Farben | 2 |
| TNT Timer | Restzeit bis zur Explosion | 2 |
| Hotbar Animations | Hotbar-Animation | 3 |
| Vignette aus | Dunkle Ränder entfernen | 3 |

### 5.3 Post-Effekte (Present-Hook)

| Modul | Optionen | Prio |
|---|---|---|
| Motion Blur | Stärke, Samples, nur bei Kamerabewegung | 2 |
| Saturation / Hue | Sättigung, Farbton | 2 |
| Brightness/Contrast | | 2 |
| Depth of Field | | 3 |
| Deepfry | Spaß | 3 |
| Upside Down | Spaß | 3 |
| Screen Tint | Farbfilter, z. B. Pink | 3 |
| Sharpen | Schärfe | 3 |

### 5.4 Modell & Animation

| Modul | Beschreibung | Prio |
|---|---|---|
| View Model | Position/Größe/Rotation der Hand und des Items | 2 |
| Animations | 1.8-Look, kleineres Item, eigener Schwung | 2 |
| Offhand/Hand ausblenden | | 3 |
| Paperdoll | Eigene Spielerfigur im HUD, Größe, Position, Drehung | 2 |
| Skin Stealer | Skin eines anderen übernehmen, bis Verlassen des Servers (nur lokal) | 2 |
| Cape/Cosmetics (lokal) | Nur für dich sichtbar | 3 |
| Nick (clientseitig) | Anderen Namen für dich anzeigen | 3 |

### 5.5 Crosshair

| Modul | Optionen | Prio |
|---|---|---|
| Custom Crosshair | Pixel-Editor, Formen, Farbe, Outline, Dynamik beim Laufen/Springen/Zielen, Import/Export | ov 1 |
| Crosshair Verhalten | Immer anzeigen, in F5 ausblenden | 2 |
| Hit-Marker | Kurzer Marker beim Treffer (rein visuell) | 2 |
| Target-Farbe | Crosshair wechselt die Farbe, wenn ein Spieler in normaler Reichweite anvisiert wird | 3 |

---

## 6. Komfort & Steuerung

| Modul | Beschreibung | Prio |
|---|---|---|
| Toggle Sprint | Dauerlauf, Status im HUD, Flug-Variante | 1 |
| Toggle Sneak | Dauer-Sneak | 1 |
| Modern Keybind Handling | Bewegung nach Inventar/Chat korrekt fortsetzen | 1 |
| Java Inventory Hotkeys | Shift-Klick, Zahlen-Tasten, Drop | 2 |
| Inventory Lock | Slots sperren, Item bleibt | 2 |
| Inventory Sort | Sortieren per Taste (nur Anordnung), Zeiten wie vanilla | 3 |
| Disable Mouse Wheel | Hotbar-Scroll aus | 1 |
| Command Hotkey | Tastenkürzel für Befehle | 2 |
| Text Hotkey | Tastenkürzel für Chat-Text | 2 |
| Auto GG | Hive, Zeqa, CubeCraft, Lifeboat, Galaxite. "Beim Kill" auf Hive gesperrt | 2 |
| Death Logger | Todespunkt merken, Koordinaten in Chat/HUD | 3 |
| Message Logger | Chat speichern | 3 |
| Chat-Verbesserungen | Hintergrund entfernen, Compact Chat, Zeitstempel, Chat-Suche, Wort-Hervorhebung, Verlauf verlängern, Movable Chat | 2 |
| Scoreboard | Movable, Zahlen ausblenden, Hintergrund, Skalierung, Clear Scoreboard | 1 |
| Movable HUD-Teile | Bossbar, Titel, Hotbar, Paperdoll, Chat | 2 |
| GUI Scale | Fein einstellbar | 2 |
| Mumble Link | Positional Audio für Mumble | 3 |
| Screenshot+ | Mit/ohne HUD per Taste, Ordner, Teilen-Link | 2 |
| Mod-Profile | Wechsel nach Server/Modus | 2 |

---

## 7. Performance

| Modul | Beschreibung | Prio |
|---|---|---|
| Render Options | Himmel, Wolken, Partikel, Block-Entities, Schatten, Wetter | 1 |
| Low Latency | Frame-Queue, Limiter, Input-Timing | 1 |
| System Boost | Prozess-Prioritäten, Timer, Power-Throttling | 1 |
| Entity Culling Hinweis | Optional, nur Darstellung | 3 |
| Auto-Profile | Performance-Preset nach Hardware | 2 |
| Hintergrundlast-Hinweis | Zeigt, was FPS/Ping stört | 2 |
| Shader/MaterialBin Loader | Eigene Shader aus Packs | 3 |

---

## 8. Server

| Modul | Beschreibung | Prio |
|---|---|---|
| Server-Regeln | Automatisches Sperren/Warnen nach `servers.json` | 1 |
| Server-Profile | Modul-Set pro Server-IP | 2 |
| Hive Utils | Auto-Requeue, Stats | 2 |
| Hive Statistics | Spielerstatistik | 3 |
| Zeqa Utils | | 3 |
| CubeCraft Utils | | 3 |
| Server-Liste | Eigene Favoriten, Ping je Server | 3 |
| Reconnect | Schneller Neuverbinden nach Disconnect | 3 |

---

## 9. Spaß

Doom, Snake, Flappy Bird, DVD Screen, Block Game, 20-20-20 Augenpause, Pet (kleines Haustier im HUD), Pomodoro, Wallpaper-Overlay. Prio 3 für die neuen.

---

## 10. Luna/Lunar-Vorbild (Java, hier für Bedrock)

Wir orientieren uns an dem, was Lunar auf Java bietet, soweit es auf Bedrock möglich ist.

| Lunar-Feature | Mochi |
|---|---|
| Mod-Profile je Server | Server-Profile (8) |
| Cosmetics, Emotes | Lokale Cosmetics (nur für dich sichtbar) |
| Replay | Replay-Clip (letzte 15 s) |
| Screenshot-Manager | Screenshot+ |
| Keystrokes, CPS, Reach, Combo | Siehe 3 und 4 |
| Freelook, Zoom, Motion Blur | Siehe 5 |
| Friends/Party | Lokale Freundesliste mit Hinweis beim Join (kein Netzwerk-Eingriff) |
| FPS-Boost | Siehe 1 und 7 |
| Menü-Blur | Glas-Blur im Menü |
| Mod-Marktplatz | Script-Marktplatz (Lua) |

---

## 11. Neu, gibt es bei keinem

| Feature | Beschreibung | Prio |
|---|---|---|
| Latenz-Overlay und "Wer ist schuld?" | siehe 1.1 und 1.4a | 1 |
| WLAN-/Netzwerk-Modul | siehe 1.4 | 1 |
| Instant Input mit Messung | siehe 1.3 | 1 |
| Sig-Status | Welche Module laufen auf welcher Version | 1 |
| Auto-Heilung | Fehlerhaftes Modul wird abgeschaltet und beim nächsten Start gemeldet | 1 |
| Signaturen-Update ohne Release | `sigs/<version>.json` von GitHub | 1 |
| Replay-Clip | Letzte 15 s puffern, als GIF/MP4 speichern | 3 |
| Kill Effects (lokal) | Herzchen-Partikel passend zum Theme | 3 |
| Modul-Empfehlung | "Dein PC hat wenig FPS. Willst du das Performance-Profil?" | 3 |
| Config-Marktplatz | Profile teilen und bewerten | 3 |
| Trainings-Statistik | Klick- und Treffer-Verläufe über Zeit (nur eigene Daten) | 3 |

---

## 12. Launcher & Updates

| Feature | Beschreibung | Prio |
|---|---|---|
| Launch & Inject | Minecraft starten, Prozess abwarten, DLL injizieren | 1 |
| Auto-Update | GitHub Releases, DLL + Launcher, SHA256-Prüfung, Rename-Trick für Selbst-Update | 1 |
| Sig-Updates | `sigs.json` beim Start nachladen | 1 |
| Version-Switcher | Installieren/Umschalten verschiedener MC-Versionen, nur über die Microsoft-Berechtigung des Nutzers, Auto-Update des Spiels für gepinnte Versionen verhindern | 2 |
| Kompatibilitäts-Badge | "Mochi-kompatibel" pro Version | 2 |
| Update-Kanäle | Stable/Beta | 2 |
| News/Changelog | Aus dem GitHub-Release | 2 |
| Dev-Pfad | Eigene DLL-Pfade für Entwickler | 1 |
| Antivirus-Hinweis | Erklärt, warum Injection Alarme auslösen kann | 2 |

---

## 13. Scripting

Lua 5.4, gratis (Onix: nur bezahlt). Module und HUD-Elemente per Script, Hot-Reload, API-Doku, Script-Ordner, Marktplatz im Client. Phase 9, Prio 2.

---

## 14. Zahlenziele

| Bereich | Start | Ziel v1.0 | Ziel danach |
|---|---|---|---|
| Module | 20 | 135 | 200+ |
| Spiel-Module mit Signaturen | 0 | 40 | 100 |
| Themes | 5 | 5 + Editor | Community-Themes |
| Scripts | – | – | Lua gratis |
| Server mit Regeln | 5 | 5 | 10+ |

---

## 15. Reihenfolge der Umsetzung

1. Overlay-Module (brauchen keine Signaturen): HUD, Crosshair, Post-Effekte, Fun, Latenz-Overlay, Netzwerk-Modul.
2. Onix-Layout und Animations-Feinschliff.
3. Spiel-Schnittstelle (SDK) mit simulierten Demo-Daten, damit alle Module testbar sind.
4. Spiel-Module nach Prioritäten aus diesem Dokument, sobald am PC Signaturen gefunden sind.
5. Launcher, Auto-Update, Version-Switcher.
6. Lua-Scripting, Server-Profile, Feinschliff.

---

## 16. Stand der Umsetzung (Session B, `docs/PLAN_B.md` Phasen 0 bis 7)

Die Tabelle je Modul steht in `docs/MODULES.md` ("Umsetzungsstand", erzeugt mit `tools/modules_doc.py`). Hier nur die Abweichungen von dieser Spezifikation und die Sachen, die erst am PC prüfbar sind.

- **Alle Module bauen und laufen mit Demo-Daten.** Im echten Spiel wirkt ein Modul, sobald seine Signaturen da sind. Fehlt eine, ist es grau (Regel 7 in `CLAUDE.md`). Die Liste aller Signaturen und Offsets steht in `docs/SDK.md`.
- **Hitbox** zeichnet nicht selbst, sondern stellt den Zeichenweg des Spiels um (Boxform, Dicke, Deckkraft, Augen- und Blicklinie, Reichweite, eigene Box, Java-Größe). Eigene 2D-Boxen oder Boxen, die vom Abstand abhängen, würden durch Wände zeigen und sind deshalb nicht gebaut.
- **Item Counter** hat farbige Symbole statt Item-Texturen, **Debug Menu** keinen Sound-Zähler (beides braucht Spieldateien beziehungsweise Engine-Daten).
- **Crystal PvP:** Der getroffene Crystal verschwindet lokal sofort (Geisterliste, siehe `SDK.md`). Gemessen ist nichts, es gibt keine Zahlen zu versprechen, bevor das Latenz-Overlay am PC Klick bis Bild misst.
- **Hive Utils, Zeqa Utils, Hive Stats:** Die Chat-Wortlisten und die Hive-API-Felder sind Annahmen und in den Einstellungen änderbar. Echte Zeilen mit dem Message Logger sammeln, dann die Standardwörter anpassen. Die Regelseiten von Hive, NetherGames und Mineville waren von der Cloud-Maschine aus nicht lesbar.
- **Gemeinsame HUD-Optik:** Rand, Glow, Schatten, echter Blur, Rotation, Padding X und Y, Ausrichtung gelten für jedes HUD-Modul. Der Blur braucht einen DX11-Pass (`post::blur`) und ist unter Wine geprüft.
- **Skripte:** Lua 5.4 mit Sandbox, siehe `docs/SCRIPTING.md`. Discord Rich Presence braucht eine eigene Discord-Anwendungs-ID.
