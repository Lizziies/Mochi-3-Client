# SDK: Spiel-Schnittstelle der Module

Alles, was Module vom Spiel lesen oder ins Spiel schreiben, läuft über `dll/src/sdk`. Kein Modul fasst Speicher oder Hooks selbst an. Dadurch fehlen am PC nur Signaturen und Offsets, nicht neue Modulcode-Teile.

## Demo-Daten

Im Modul "Game Support" (Kategorie Performance) gibt es den Schalter "Demo-Daten". Dann liefert `sdk/Demo.cpp` simulierte Spielwerte (Position, Leben, Rüstung, Tränke, Kämpfe mit Treffern, Chat, Scoreboard, Tab-Liste, Weltzeit). Alle Spiel-Module werden dabei benutzbar, die Effekt-Kanäle (siehe unten) werden angefordert, aber nichts im Spiel verändert. Zum Testen der HUDs und Einstellungen am PC ohne Signaturen. Standard ist aus.

## Daten lesen (`sdk/Game.hpp`, `sdk/Live.cpp`)

`game::state()` liefert pro Frame den Zustand, `game::events()` die Ereignisse dieses Frames (Treffer, Schaden, Kill, Tod, Totem, Schwung, Chat). Kampfstatistik (Combo, Reichweite, Treffer, Streak) rechnet das SDK selbst aus den Ereignissen.

Ein Modul meldet mit `needs(game::Domain::...)`, welche Daten es braucht. Es ist verfügbar, wenn alle seine `sigs` gefunden sind **und** der Live-Provider die Bereiche liefert. Sonst wird es grau.

Der Live-Provider liest feste Strukturen über Signaturen und Offsets aus `sigs/<version>.json`:

| Name | Art | Bedeutung |
|---|---|---|
| `LocalPlayer` | Signatur | Adresse einer statischen Variable, die den Zeiger auf den lokalen Spieler hält (`mov rax,[rip+x]`, `rel: "mov"`) |
| `Level` | Signatur | wie oben, Zeiger auf das Level |
| `AttackEntity` | Signatur | Funktion `bool GameMode::attack(Actor*)`, wird für Treffer-Ereignisse gehookt (Detour nimmt `(void* gm, void* actor)`) |
| `player.posX` | Offset | drei Floats x,y,z hintereinander (Füße) |
| `player.velX` | Offset | drei Floats, Blöcke pro Tick |
| `player.yaw`, `player.pitch` | Offset | Float, Grad |
| `player.health`, `player.maxHealth`, `player.hunger`, `player.saturation`, `player.eyeHeight` | Offset | Float |
| `player.air`, `player.level`, `player.dimension` | Offset | Int |
| `player.onGround`, `player.sprinting`, `player.sneaking` | Offset | Byte |
| `level.time` | Offset | Int, Ticks |
| `level.rain`, `level.thunder` | Offset | Float |
| `actor.posX` | Offset | drei Floats, für die Trefferdistanz |
| `actor.aabbMinX` | Offset | sechs Floats min/max, genauer als `actor.posX` |

Bereiche, die der Live-Provider noch nicht liefert (Inventar, Tränke, Ziel, Chat, Scoreboard, Tab-Liste): Module mit `needs(Inventory)` usw. bleiben grau, bis `Live.cpp` den Bereich liest. Das ist Arbeit für den PC, weil dafür die Spielstrukturen bekannt sein müssen. Die Reihenfolge nach Nutzen: Inventar (Armor, Pots, Zähler), Effekte, Ziel, Chat.


Marker-Signaturen: Namen für Spielfunktionen oder Daten, die der Live-Provider noch nicht liest. Sie stehen nur in den `sigs` der Module, damit diese grau bleiben, bis jemand am PC die Funktion findet und `Live.cpp` den Bereich liest. Gemeint ist jeweils:

- `Inventory`: Spieler-Inventar (Hotbar, Rüstung, Nebenhand, Hauptinventar) mit Itemname, Menge, Haltbarkeit.
- `Effects`: aktive Trankeffekte mit Stufe und Restzeit.
- `Target`: Ziel unter dem Fadenkreuz (Block oder Entity, Distanz, Leben, Abbaufortschritt).
- `ChatEvents`: Hook auf eingehende Chatzeilen.
- `ScoreboardData`, `TabListData`, `PackList`, `EntityList`: Scoreboard, Spielerliste, aktive Packs, Entity-Zähler.
- `KillEvents`, `TotemEvents`, `ItemUseEvents`: Ereignisquellen für Kill, Totem-Verbrauch und Item-Nutzung.
- `UseState`: `usingItem` und `useProgress` des Spielers (Bogen, Essen).
- `ActorList`: Spieler in der Nähe (Name, Position, Team) für Opponent Reach und die Team-Ausnahme.
- `ActorEvent`: eingehendes Entity-Ereignis "verletzt", für die Zeit vom Angriff bis zur Bestätigung des Servers (Hit Ping, Ereignis `Confirm`).

| Name | Gebraucht von |
|---|---|
| `ActorEvent` | Hit Ping |
| `ActorList` | Opponent Reach |
| `AttackEntity` | Reach Counter, Combo Counter, Hit Counter, Hit Ping, Session Stats, Hit Info, Hit Effects, Damage Indicator, Hit Sound, Match Summary |
| `ChatEvents` | Session Stats, Auto GG, Message Logger, Chat Plus |
| `Effects` | Potion HUD |
| `EntityList` | Entity Counter |
| `Inventory` | Held Item, Armor HUD, Pot Counter, Arrow Counter, Totem Counter, Item Counter, Durability Warning, Better Hunger Bar, Paperdoll |
| `ItemUseEvents` | Cooldown Indicator |
| `KillEvents` | Kill Effects |
| `Level` | Entity Counter, Day Counter, Pack Display |
| `LocalPlayer` | Java Dynamic FOV, Freelook, Auto Perspective, Reach Counter, Opponent Reach, Combo Counter, Hit Counter, Hit Ping, Session Stats, Hit Info, Hit Effects, Kill Effects, Damage Indicator, Hit Sound, Totem Pop, Target HUD, Waila, Bow Charge, Cooldown Indicator, Bow Sensitivity, Snap Look, Death Logger, Mumble Link, Coordinates, Direction HUD, Speed Display, Look Angles, Health Display, Experience Info, Held Item, Break Progress, Armor HUD, Potion HUD, Pot Counter, Arrow Counter, Totem Counter, Item Counter, Durability Warning, Low Health Indicator, Better Hunger Bar, Paperdoll, Match Summary, Waypoints, Block Outline, Chunk Border |
| `PackList` | Pack Display |
| `ScoreboardData` | Scoreboard |
| `TabListData` | Player Notifier, Tab List |
| `Target` | Target HUD, Waila, Break Progress, Block Outline |
| `TotemEvents` | Totem Pop |
| `UseState` | Bow Charge |

## Ins Spiel schreiben (`sdk/Effects.hpp`)

Ein Modul fordert pro Frame einen Effekt an, zum Beispiel `fx::set(fx::Id::Fov, 90.f)` oder `fx::skip(fx::Id::ViewBob)`. `fx::apply()` hookt die Funktion beim ersten Bedarf, wenn die Signatur gefunden wurde, und gibt den Wunschwert zurück. Wird nichts mehr angefordert, ruft der Hook nur noch das Original auf.

Arten pro Kanal (Standard, pro Signatur mit dem Offset `<sig>.kind` überschreibbar: 0 Value, 1 Flag, 2 Skip, 3 Out, 4 Data):

- **Value**: Funktion gibt einen Float zurück. Der Hook ersetzt, skaliert oder addiert.
- **Flag**: Funktion gibt bool/int zurück. Der Hook erzwingt den Wert.
- **Skip**: Funktion wird nicht aufgerufen, solange das Modul aktiv ist.
- **Out**: Original wird aufgerufen, danach schreibt der Hook Floats in den Zeiger-Parameter (mit dem Offset `<sig>.before = 1` davor). Bei `fx.lookDelta` glättet der Hook die Blickbewegung (Zeiger auf zwei Floats, vor dem Original). Der Index des Parameters steht im Offset `<sig>.arg` (Standard 1 = `rdx`). Bei `fx.handMatrix` ist es eine 4x4-Matrix (Spaltenmajor, mit `<sig>.rowMajor = 1` für Zeilenmajor), die verschoben/skaliert/gedreht wird.
- **Int**: wie Flag, aber der Hook gibt einen Integer zurück (zum Beispiel die Perspektive).
- **Data**: die Signatur zeigt auf Daten (Float oder Float4), die direkt überschrieben und beim Abschalten zurückgesetzt werden.

Alle Detours haben die Form `(a, b, c, d)` mit vier Integer-Argumenten und reichen sie ans Original weiter. Passt eine echte Funktion nicht dazu (Float-Argumente in xmm), muss der Detour in `Effects.cpp` für diesen Kanal angepasst werden.

| Signatur | Wirkung | Standard-Art | Gebraucht von |
|---|---|---|---|
| `fx.fov` | Sichtfeld | Value | FOV Changer, Java Dynamic FOV, Zoom |
| `fx.fovEffects` | Sichtfeld-Effekte | Value | – |
| `fx.gamma` | Helligkeit | Value | Fullbright |
| `fx.viewBob` | Kamera-Wackeln | Skip | No View Bobbing |
| `fx.handBob` | Hand-Wackeln | Skip | – |
| `fx.hurtCam` | Treffer-Wackeln | Value | No Hurt Cam |
| `fx.bobStrength` | Wackel-Stärke | Value | Minimal View Bobbing |
| `fx.perspective` | Perspektive | Int | Auto Perspective |
| `fx.sneakCam` | Schleich-Kamera | Value | Smooth Sneak |
| `fx.sensitivity` | Empfindlichkeit | Value | Sens Multiplier, Bow Sensitivity |
| `fx.time` | Tageszeit | Value | Time Changer |
| `fx.rain` | Regen | Value | Render Options, Weather Changer |
| `fx.thunder` | Gewitter | Value | – |
| `fx.clouds` | Wolken | Flag | Render Options |
| `fx.sky` | Himmel | Flag | Render Options |
| `fx.particles` | Partikel | Flag | Render Options |
| `fx.blockEntities` | Block-Entities | Flag | Render Options |
| `fx.shadows` | Schatten | Flag | Render Options |
| `fx.fog` | Nebel | Flag | Render Options, Environment Changer |
| `fx.vignette` | Vignette | Flag | Render Options |
| `fx.fire` | Feuer-Overlay | Value | Low Fire |
| `fx.hitbox` | Hitboxen | Flag | Hitbox |
| `fx.hitboxColor` | Hitbox-Farbe | Data | – |
| `fx.glintColor` | Glanzfarbe | Data | Glint Color |
| `fx.hurtColor` | Trefferfarbe | Data | Hurt Color |
| `fx.fogColor` | Nebelfarbe | Out | Fog Color |
| `fx.waterColor` | Wasserfarbe | Out | Water Color |
| `fx.particleScale` | Partikelmenge | Value | Particle Multiplier |
| `fx.guiScale` | GUI-Skalierung | Value | GUI Scale |
| `fx.hideHand` | Hand ausblenden | Skip | Hide Hand |
| `fx.hideOffhand` | Nebenhand ausblenden | Skip | – |
| `fx.hideChat` | Original-Chat ausblenden | Skip | – |
| `fx.hideScoreboard` | Original-Scoreboard ausblenden | Skip | – |
| `fx.hideCrosshair` | Original-Fadenkreuz ausblenden | Skip | – |
| `fx.hideHud` | Original-HUD ausblenden | Skip | – |
| `fx.handMatrix` | Hand-Transformation | Out | View Model, Animations |
| `fx.lookTurn` | Blickdrehung des Spielers | Skip | Freelook |
| `fx.lookCamera` | Kamera-Rotation | Out | Freelook, Snap Look |
| `fx.lookDelta` | Blickbewegung | Out | Cinematic Camera |
| `fx.selfNametag` | Eigener Nametag | Flag | – |
| `fx.itemPhysics` | Item-Physik | Flag | – |
| `fx.useDelay` | Item-Nutzungs-Verzögerung | Value | Item Use Delay Fix |
| `fx.inventoryDelay` | Inventar-Verzögerung | Value | Faster Inventory |
| `fx.hurtAnim` | Treffer-Animation | Flag | Insta Hurt Animation |
| `fx.blockOutline` | Original-Blockumriss | Skip | – |
| `fx.swingSpeed` | Schwung-Dauer | Value | – |
| `fx.crystalHide`, `fx.crystalSimple`, `fx.crystalNoBase` | Crystal ausblenden, ohne Drehen, ohne Sockel | Flag | Crystal Optimizer |
| `fx.ghostRender` | Entity nicht zeichnen, wenn sie in der Geisterliste steht (Aufruf mit dem Actor-Zeiger im Argument `<sig>.arg`, Standard 1) | Ghost | Crystal Optimizer, Kill Cleanup |
| `fx.ghostPick` | Zielsuche gibt Entities aus der Geisterliste nicht zurück (Rückgabewert ist der Actor-Zeiger) | Filter | Crystal Optimizer, Kill Cleanup |
| `fx.critParticle` | Crit-Partikel erzwingen | Flag | Particle Multiplier |
| `fx.hitboxEye`, `fx.hitboxLook` | Augenlinie und Blickrichtungslinie der Hitboxen | Flag | Hitbox |
| `fx.hitboxEyeColor`, `fx.hitboxLookColor` | Farben der beiden Linien (vier Floats) | Data | Hitbox |
| `fx.hitboxLookLength`, `fx.hitboxWidth`, `fx.hitboxRange` | Länge der Blicklinie, Liniendicke, Reichweite (je ein Float) | Data | Hitbox |
| `fx.hitboxSelf`, `fx.hitboxJava`, `fx.hitbox2D` | eigene Hitbox, Java-Größe (+0,1), flache Box | Flag | Hitbox |
| `fx.itemFov` | Sichtfeld, mit dem Hand und Item gezeichnet werden | Value | View Model |
| `fx.handMatrixThird` | Hand-Matrix in der dritten Person | Out | View Model |
| `fx.renderEntities`, `fx.renderTerrain` | Entities, Gelände zeichnen | Flag | Render Options |

**Geisterliste:** `fx::ghost(actor, sekunden)` merkt sich einen Actor-Zeiger, `fx::ghosted` fragt ab. Die Liste ist rein lokal, hat 16 Plätze und läuft von selbst ab. Treffer auf Crystals kommen als `Hit`-Ereignis mit `crystal = true` und `actor`; der Live-Provider erkennt sie über die Offsets `actor.typeId` (Int im Actor) und `type.crystal` (der Wert für den End Crystal, steht als Offset-Zahl in der Signaturdatei). Fehlt einer davon, wird nie ein Crystal ausgeblendet. Crystal-Treffer zählen nicht für Combo und Reichweite.

## Tasten und Chat senden (`sdk/Inject.hpp`)

`inject::key`, `inject::tap` und `inject::say` senden Tastenereignisse per `SendInput` mit Scancode, nur wenn das Spielfenster im Vordergrund ist. Die Ereignisse tragen eine Markierung (`inject::ours()` in `onKey`), damit Module ihre eigenen Eingaben erkennen. Toggle Sprint/Sneak, Null Movement und Chat-Hotkeys laufen darüber und brauchen keine Signaturen. `say` öffnet den Chat wie ein Spieler (Taste, Text tippen, Enter), mit mindestens 1,2 s Abstand.

## Grenzen

Kein Modul schreibt Pakete. Hitboxen werden über den Spiel-eigenen Zeichenweg angefordert (`fx.hitbox`), damit sie dem Tiefentest des Spiels folgen und nicht durch Wände sichtbar sind. Nicht gebaut: Skin Stealer, Replay-Clip, alles, was Informationen zeigt, die der normale Client nicht anzeigen würde.
