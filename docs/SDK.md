# SDK: Spiel-Schnittstelle der Module

Alles, was Module vom Spiel lesen oder ins Spiel schreiben, läuft über `dll/src/sdk`. Kein Modul fasst Speicher oder Hooks selbst an. Dadurch fehlen am PC nur Signaturen und Offsets, nicht neue Modulcode-Teile.

## Demo-Daten

Im Modul "Sig Status" (Kategorie Performance) gibt es den Schalter "Demo-Daten". Dann liefert `sdk/Demo.cpp` simulierte Spielwerte (Position, Leben, Rüstung, Tränke, Kämpfe mit Treffern, Chat, Scoreboard, Tab-Liste, Weltzeit). Alle Spiel-Module werden dabei benutzbar, die Effekt-Kanäle (siehe unten) werden angefordert, aber nichts im Spiel verändert. Zum Testen der HUDs und Einstellungen am PC ohne Signaturen. Standard ist aus.

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

## Ins Spiel schreiben (`sdk/Effects.hpp`)

Ein Modul fordert pro Frame einen Effekt an, zum Beispiel `fx::set(fx::Id::Fov, 90.f)` oder `fx::skip(fx::Id::ViewBob)`. `fx::apply()` hookt die Funktion beim ersten Bedarf, wenn die Signatur gefunden wurde, und gibt den Wunschwert zurück. Wird nichts mehr angefordert, ruft der Hook nur noch das Original auf.

Arten pro Kanal (Standard, pro Signatur mit dem Offset `<sig>.kind` überschreibbar: 0 Value, 1 Flag, 2 Skip, 3 Out, 4 Data):

- **Value**: Funktion gibt einen Float zurück. Der Hook ersetzt, skaliert oder addiert.
- **Flag**: Funktion gibt bool/int zurück. Der Hook erzwingt den Wert.
- **Skip**: Funktion wird nicht aufgerufen, solange das Modul aktiv ist.
- **Out**: Original wird aufgerufen, danach schreibt der Hook Floats in den Zeiger-Parameter. Der Index des Parameters steht im Offset `<sig>.arg` (Standard 1 = `rdx`). Bei `fx.handMatrix` ist es eine 4x4-Matrix (Spaltenmajor, mit `<sig>.rowMajor = 1` für Zeilenmajor), die verschoben/skaliert/gedreht wird.
- **Data**: die Signatur zeigt auf Daten (Float oder Float4), die direkt überschrieben und beim Abschalten zurückgesetzt werden.

Alle Detours haben die Form `(a, b, c, d)` mit vier Integer-Argumenten und reichen sie ans Original weiter. Passt eine echte Funktion nicht dazu (Float-Argumente in xmm), muss der Detour in `Effects.cpp` für diesen Kanal angepasst werden.

| Signatur | Wirkung | Standard-Art |
|---|---|---|
| `fx.fov` | Sichtfeld | Value |
| `fx.fovEffects` | Sichtfeld-Effekte | Value |
| `fx.gamma` | Helligkeit | Value |
| `fx.viewBob` | Kamera-Wackeln | Skip |
| `fx.handBob` | Hand-Wackeln | Skip |
| `fx.hurtCam` | Treffer-Wackeln | Skip |
| `fx.sneakCam` | Schleich-Kamera | Value |
| `fx.sensitivity` | Empfindlichkeit | Value |
| `fx.time` | Tageszeit | Value |
| `fx.rain` | Regen | Value |
| `fx.thunder` | Gewitter | Value |
| `fx.clouds` | Wolken | Flag |
| `fx.sky` | Himmel | Flag |
| `fx.particles` | Partikel | Flag |
| `fx.blockEntities` | Block-Entities | Flag |
| `fx.shadows` | Schatten | Flag |
| `fx.fog` | Nebel | Flag |
| `fx.vignette` | Vignette | Flag |
| `fx.fire` | Feuer-Overlay | Value |
| `fx.hitbox` | Hitboxen | Flag |
| `fx.glintColor` | Glanzfarbe | Data |
| `fx.hurtColor` | Trefferfarbe | Data |
| `fx.fogColor` | Nebelfarbe | Out |
| `fx.waterColor` | Wasserfarbe | Out |
| `fx.particleScale` | Partikelmenge | Value |
| `fx.guiScale` | GUI-Skalierung | Value |
| `fx.hideHand` | Hand ausblenden | Skip |
| `fx.hideOffhand` | Nebenhand ausblenden | Skip |
| `fx.hideChat` | Original-Chat ausblenden | Skip |
| `fx.hideScoreboard` | Original-Scoreboard ausblenden | Skip |
| `fx.hideCrosshair` | Original-Fadenkreuz ausblenden | Skip |
| `fx.hideHud` | Original-HUD ausblenden | Skip |
| `fx.handMatrix` | Hand-Transformation | Out |
| `fx.lookTurn` | Blickdrehung des Spielers | Skip |
| `fx.lookCamera` | Kamera-Rotation | Out |
| `fx.selfNametag` | Eigener Nametag | Flag |
| `fx.itemPhysics` | Item-Physik | Flag |
| `fx.useDelay` | Item-Nutzungs-Verzögerung | Value |
| `fx.inventoryDelay` | Inventar-Verzögerung | Value |
| `fx.hurtAnim` | Treffer-Animation | Flag |
| `fx.blockOutline` | Original-Blockumriss | Skip |
| `fx.swingSpeed` | Schwung-Dauer | Value |

## Tasten und Chat senden (`sdk/Inject.hpp`)

`inject::key`, `inject::tap` und `inject::say` senden Tastenereignisse per `SendInput` mit Scancode, nur wenn das Spielfenster im Vordergrund ist. Die Ereignisse tragen eine Markierung (`inject::ours()` in `onKey`), damit Module ihre eigenen Eingaben erkennen. Toggle Sprint/Sneak, Null Movement und Chat-Hotkeys laufen darüber und brauchen keine Signaturen. `say` öffnet den Chat wie ein Spieler (Taste, Text tippen, Enter), mit mindestens 1,2 s Abstand.

## Grenzen

Kein Modul schreibt Pakete. Hitboxen werden über den Spiel-eigenen Zeichenweg angefordert (`fx.hitbox`), damit sie dem Tiefentest des Spiels folgen und nicht durch Wände sichtbar sind. Nicht gebaut: Skin Stealer, Replay-Clip, alles, was Informationen zeigt, die der normale Client nicht anzeigen würde.
