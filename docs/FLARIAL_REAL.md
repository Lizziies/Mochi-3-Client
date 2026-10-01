# Flarial: was die Module wirklich können (aus dem Quellcode gelesen)

Dies ist die Grundlage für den Funktionsvergleich. Alles hier stammt aus den echten Quelldateien von Flarial (`flarialmc/dll-oss`, `src/Client/Module/Modules/<Name>/<Name>.cpp`, Einstellungen aus `defaultConfig`/`settingsRender`), nicht aus Vermutung. Wir lesen es, um das Verhalten zu verstehen, und schreiben alles selbst (AGPL, siehe `CLAUDE.md`).

Nicht geprüft werden konnte: Flarial-Webseite, Changelog und das neue Modul Crystal Optimizer (die Quelldatei ist unter dem erwarteten Pfad nicht vorhanden, das Netzwerk sperrt flarial.xyz). Bei Onix ist nur die Modulliste bekannt, der Quellcode ist geschlossen.

Geprüft: 62 Module. Nicht abgefragt: restliche Hilfs- und Spaßmodule.

## Querschnittsfunktionen (kommen bei fast jedem Modul vor)

1. **Frei einstellbarer Text** mit Platzhaltern, z. B. `CPS: {lmb} | {rmb}`, `Reach: {value}`, `{value}ms`, `{D} X: {X} Y: {Y} Z: {Z}`. Dazu **Text Scale**, **Text Alignment** (links/mitte/rechts), **Text Shadow** mit Versatz. Wir hatten das nicht. Seit heute gibt es für alle Text-HUDs das Feld "Format" mit `{label}` und `{value}`. Modulspezifische Platzhalter (`{lmb}`, `{X}` …) fehlen noch.
2. **Aussehen pro Modul:** Rounding, Hintergrund, Hintergrund-Schatten, Rand mit Dicke, Glow (an/aus, Menge, Geschwindigkeit), Blur/Translucency. Bei uns: Hintergrund, Rundung, Padding, Schatten, Größe. Glow, Rand und Blur fehlen.
3. **Spoof-Optionen** bei FPS und Ping (angezeigte Zahl vervielfachen). Das bauen wir bewusst **nicht**: es ist irreführend und kann als Betrug gelten.

## Kampf-Module

| Modul | Was es wirklich tut / Einstellungen | Unser Stand und Lücke |
|---|---|---|
| **Reach Counter** | Zeigt die Distanz des letzten Nahkampf-Treffers, 2 Nachkommastellen. Zurück auf 0 nach 15 s ohne Treffer. Text `Reach: {value}`, Text Scale | prüfen: Format, 15-s-Reset |
| **Combo Counter** | Treffer zählen, wenn der nächste Treffer innerhalb **480 ms** nach dem vorigen kommt. Reset nach 15 s Ruhe. Wenn du Schaden nimmst: Reset, oder bei "Count to Negatives" geht der Zähler ins Minus. Text `Combo: {value}` | prüfen: 480-ms-Regel, Negatives-Option |
| **Hit Ping** | Misst die Zeit zwischen deinem Angriff und der Bestätigung durch den Server (ActorEvent-Paket) und berechnet dazu die Reach. Ignoriert zweite Treffer auf dasselbe Ziel innerhalb 480 ms. Reset nach 15 s. Text `{value} ms` | Das ist der "Minecraft-Ping beim Treffer", den du meinst. Bei uns vorhanden, prüfen |
| **Opponent Reach** | Wenn du Schaden nimmst, sucht es Spieler im Umkreis von 10 Blöcken, nimmt den nächsten innerhalb 5,5 Blöcken als Angreifer und zeigt dessen Reach. "Try To Exclude Team". Reset nach 15 s | vorhanden, prüfen |
| **Hitbox** | Hitbox-Quader für Mobs bis 30 Blöcke. Optionen: Java-ähnlich per Taste umschalten, **2D-Modus**, Dicke (skaliert mit Entfernung oder fest), **Deckkraft**, **Augenlinie** (Farbe), **Blickrichtungslinie** (Länge 0,5 bis 10, Farbe), **sich selbst zeigen**, Farbe | **Große Lücke:** bei uns nur Farbe und Regenbogen |
| **Hurt Color** | Eine Farbe mit Deckkraft (Standard weiß, 0,65) für die Treffer-Einfärbung | ✔ (wir haben mehr) |
| **Block Hit** | 1.8-Schwertblock-Pose per Matrix-Transformation, nur wenn ein Schwert in der Hand ist und die rechte Maustaste gehalten wird. Keine Einstellungen | ✔ gebaut (Block Hit) |
| **Instant Hurt Animation** | Spielt die Treffer-Animation sofort, wenn du schlägst. Optionen: Team ausschließen, **nur bei Gegnern mit voller Rüstung** | vorhanden (server-rules), Optionen prüfen |
| **Swing Animations** | **Swing Speed** 0,1 bis 3, **Flux Swing** (Patch für flüssigere Animation), **Swing Angle** −180 bis 90 | Lücke: Swing Angle, Flux Swing |
| **Animations** | Flarial: Geschwindigkeit der **Hotbar-Auswahl-Animation** (Standard 7). Das ist nicht die 1.8-Animation | unser "Animations" ist etwas anderes. Hotbar-Animation fehlt |
| **CPS Limiter** | Max. Klicks links (16) und rechts (24) | ✔ |
| **Null Movement** | Getrennt für W/S und A/D: nur die zuletzt gedrückte Taste zählt | ✔ |
| **Faster Inventory / Item Use Delay Fix** | Keine Einstellungen | ✔ |
| **Particle Multiplier** | Menge bis 500, plus "Normal Hit Crit": zeigt Crit-Partikel bei jedem Schlag | Lücke: Normal Hit Crit |
| **Snap Look** | Taste halten oder umschalten, Modus 3rd Person Front/Back/1st Person | ✔ |
| **Auto GG** | Sendet beim Spielende automatisch einen Text (Standard "GG"). Erkennt das Ende über Pakete auf Zeqa, Galaxite, Mineville, The Hive, CubeCraft, Lifeboat | ✔ (unseres hat mehr Optionen), Erkennung pro Server prüfen |
| **Crystal Optimizer** (neu) | Quelle nicht lesbar. Auf Bedrock sind "Crystal Optimizer" üblicherweise vereinfachte Crystal-Darstellungen (kein Drehen/Wippen, schlankes Modell). | Unser Modul macht jetzt Drehen/Wippen aus, Sockel aus, optional beim Treffen ausblenden. **Verhalten bei Flarial noch bestätigen** |

## Kamera & Sicht

| Modul | Was es wirklich tut | Lücke bei uns |
|---|---|---|
| **Zoom** | Taste C, per Scroll ändern, Toggle, Modifier 0 bis 30, Animation (Geschwindigkeit, aus, immer animieren), Modifier speichern, **Hand ausblenden**, **Module ausblenden**, Cinematic-Kamera, Glättung, **Cinematic-Balken** (Höhe, Farbe), Low-Sensitivity (Stärke) | Lücke: Hand und HUD beim Zoomen ausblenden, Cinematic-Balken |
| **Fullbright** | Gamma-Slider 0 bis 25 | ✔ |
| **FOV Changer** | 0 bis 359, auf Servern über 150 gedeckelt, "Hand-Größe mitändern" | ✔ (mehr), Hand-Größe prüfen |
| **Freelook** | Taste, Toggle oder Halten, Modus 1st/3rd back/3rd front | ✔ |
| **Java Dynamic FOV** | FOV-Ziel beim Sprinten und Animationsgeschwindigkeit | ✔ |
| **Minimal/Java View Bobbing, No Hurt Cam** | keine Einstellungen | ✔ (No View Bobbing besser) |
| **Cinematic Camera** | Toggle (Java-Verhalten), Glättung 0 bis 10, Cinematic-Balken | Lücke: Balken |
| **Auto Perspective** | Wechselt die Perspektive bei Elytra, **Schwimmen**, Reiten, **Emote** | Lücke: Schwimmen, Emote |
| **View Model** | **Dritte Person**, **Item-FOV** 0 bis 180, Position/Rotation/Skalierung je XYZ | ✔ fast, Lücke: Item-FOV, dritte Person |
| **Motion Blur** | 5 Arten (Average Pixel, Real Motion Blur, Ghost Frames, Time Aware, "V4" nach Onix-Vorbild), Dynamic Mode, Intensität, Samples, "unter dem UI rendern" | prüfen, ob unsere Arten gleichwertig sind |
| **Render Options** | Chunk-Grenzen, Himmel, Wetter, **Entities**, Block-Entities, Partikel, **Terrain**, **Item in der Hand**, **HUD** an/aus | Lücke: Entities, Terrain, Item in Hand |
| **Block Outline** | Umriss (3D, Farbe, Breite) und Overlay (3D, Farbe, Deckkraft) | ✔ (unseres hat mehr) |
| **Custom Crosshair** | Eigene **PNG-Dateien** aus dem Ordner Crosshairs plus Editor, Farbe, Gegner-Farbe beim Anvisieren, Dritte Person | ✔ Editor und Gegnerfarbe. Lücke: PNG-Import |
| **Item Physics** | Geschwindigkeit, Y-Versatz, Rotation behalten, weiche Rotation | **fehlt komplett** |

## HUD / Anzeigen

| Modul | Was es wirklich tut | Lücke bei uns |
|---|---|---|
| **Keystrokes** | Sehr viele Optionen: Größe, Rundung, Hintergrund, Schatten, Rand, **Glow** (an/aus getrennt), Tastenabstand, **Leertaste Breite/Höhe**, Highlight-Geschwindigkeit, **eigene Texte für W A S D und LMB/RMB**, CPS-Text `{value} CPS`, Text-Offsets | Lücke: Glow, Rand, Leertasten-Maße, eigene Tastentexte |
| **CPS Counter** | Format `CPS: {lmb} \| {rmb}`, Scale, Ausrichtung, Schatten | Lücke: Format mit `{lmb}`/`{rmb}` |
| **Potion HUD** | UI Scale, **Bottom Up**, Abstand, Text an/aus, **Effektname**, **römische Ziffern** (auch über V), Textgröße/Versatz/links, Schatten, Farben. **Rot, wenn ≤ 5 s übrig** | prüfen |
| **Arrow Counter** | Optional nur anzeigen, wenn Bogen oder Armbrust in der Hand | prüfen |
| **Pot Counter** | Zählt Splash-Tränke (Item `splash_potion`) in den Slots 0 bis 35. Text `{value} Pots` | prüfen |
| **Totem Counter** | Optional nur anzeigen, wenn ein Totem in der Hand ist | prüfen |
| **Direction HUD** | Kompassleiste: Pixel pro Grad, Rand-Ausblenden, Kardinal-/Ordinal-Skalen und -Texte, **Grad-Anzeige**, **Waypoints auf dem Kompass**, viele Farben | Unseres vermutlich einfacher |
| **Coordinates** | Format `{D} X: {X} Y: {Y} Z: {Z}`, Nachkommastellen, **vertikaler Modus mit Geschwindigkeit (+/−)**, Dimensionsname/-format, **Koordinaten der anderen Dimension**, **Taste: in Zwischenablage kopieren** | Lücke: Format, Geschwindigkeit, andere Dimension, Kopieren |
| **Ping Counter** | Format `{value}ms`, **letzter Ping oder Durchschnitt** | ✔ |
| **FPS** | Format `FPS: {value}` | ✔ |
| **Speed Display** | Strecke pro Tick mal 20 | ✔ |
| **Tab List** | Spielerköpfe, **Plattform-Icons** (Mobil/Konsole/PC), Weltname, Server-Ping mit Farbsymbol, Spieler pro Spalte, Sortierung, **Spieler-Hervorhebungen** mit Farbe, Toggle-Taste | Lücke: Köpfe, Plattform-Icons, Hervorhebung |
| **Paperdoll** | UI Scale, "Always Show" | ✔ |
| **Waila** | Blockname (einfach oder mit Namespace), Luft anzeigen | ✔ |
| **Sprint / Sneak** | Taste zum Umschalten, **Always Sprint**, Status-Text mit "(Toggled)/(Vanilla)" | prüfen |
| **TNT Timer** | Zeigt am TNT den Countdown. Format `Timer: {value}`, Nachkommastellen 0 bis 3, Farbschwellen (Standard 2,5 s und 1 s), Farben | **fehlt komplett** |
| **Durability Warning** | Schwelle (Standard 10), Meldung mit Itemname | ✔ |
| **Low Health Indicator** | Roter Innenschatten, Farbe, max. Deckkraft, Schwelle (Standard 12 HP) | ✔ |
| **Mousestrokes** | Rechteck mit Maus-Spur, Glow, Rand, Blur | teils |

## Komfort / Chat / Eingabe

| Modul | Was es wirklich tut | Lücke bei uns |
|---|---|---|
| **GUI Scale** | 1 bis 4, verschiebt Hotbar, Scoreboard, Chat mit | ✔ |
| **Movable Scoreboard / Chat / Title / Bossbar / Hotbar / Day Counter / Coordinates** | Position in Prozent. **Chat: Ping-Ton bei Erwähnung und `@here`** (XP-Orb oder eigene Datei) | Scoreboard ✔, Chat: Mention-Ping fehlt, Title/Bossbar/Hotbar/Day Counter fehlen |
| **Compact Chat** | Wiederholte Zeilen mit Zähler, Format `{msg} ({count})`, Klammerart, Farben | ✔ grob |
| **Clear Chat / Clear Scoreboard** | Blendet Chat/Scoreboard aus | ✔ |
| **Inventory Lock** | Gesperrte Slots, **nur Werkzeuge**, **Doppelklick zum Droppen** (300 ms) | **fehlt** |
| **Modern Keybind Handling** | Stellt Bewegung und Sprint nach Inventar, Pause und Chat wieder her (einzeln oder alle) | **fehlt** |
| **Java Inventory Hotkeys** | Liest die Hotbar-Tasten aus der Optionsdatei, Java-Tastenlogik im Inventar | **fehlt** |
| **Command Hotkey** | Mehrere Tasten, jede mit eigenem Befehl | ✔ |
| **Nametag Modifier** | Textfarbe und Hintergrundfarbe der Nametags | **fehlt** |
| **Player Notifier** | Prüft alle 80 s, ob bestimmte Spieler online sind, Taste zum Prüfen | ✔ ähnlich |
| **Waypoints / Death Logger** | Marker mit Strahl (inner/äußer, Seiten), Hintergrund, Rand, Text, Entfernung 1000, **Todes-Waypoints** | ✔ (ähnlich) |

## Server: Hive

**Hive Utils** (bei Flarial ein einziges großes Modul):
- **Auto Requeue** (`/q` oder `/hub`), **Solo-Modus**, bei **Team-Ausscheiden**, Taste zum Requeue.
- **Map Avoider:** pro Spiel (BedWars, SkyWars, Treasure Wars …) und Variante (Solos, Duos, Squads, Mega) eine Liste von Maps, bei denen automatisch neu eingereiht wird.
- **Rollen-Requeue:** Murder Mystery (Murderer, Sheriff, Innocent), Hide and Seek (Hider, Seeker), Death Run (Death, Runner, Todes-Limit 1 bis 100).
- **Custom-Server-Code kopieren** (mit `/cs` davor).
- **Chat aufräumen:** Promo-Nachrichten `[!]`, Unlock-Hinweis, Join-Meldungen, Nachrichten von Nicht-Rang-Spielern oder Hive+, "No Teaming".
- **Auto-Accept** für Freundschafts- und Party-Anfragen.
- **Auto Map Vote** mit Benachrichtigung und Ansage (`@here vote for {map}!`).

Bei uns: **nichts davon.** Für Hive-Spieler ist das das wichtigste Modul überhaupt.

## Was das für uns heißt

Viele unserer Module haben auf den ersten Blick Einstellungen, aber bei einigen entscheidenden (Hitbox, Keystrokes, CPS, Coordinates, Tab List) fehlt Tiefe. Und vier ganze Module fehlen: **Hive Utils, Inventory Lock, Modern Keybind Handling, Item Physics, TNT Timer, Nametag Modifier**. Die Arbeitsliste dazu steht in `docs/HANDOFF.md`.
