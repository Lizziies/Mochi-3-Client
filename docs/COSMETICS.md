# Cosmetics

Kostenlose Cosmetics für alle: Flügel, Capes, Bandanas und mehr. Dieses Dokument ist die Ideenliste, der technische Rahmen und der Auftrag für den Cowork-Chat, der die Modelle baut (ohne Blender).

## Ehrlich vorab: was wo sichtbar ist

| Stufe | Was du siehst | Was nötig ist | Stand |
|---|---|---|---|
| 1. Menü-Vorschau | Cosmetic dreht sich im Menü an einer Spielfigur, Farben einstellbar | eigener kleiner 3D-Renderer im Overlay (ImGui-Zeichenliste), keine Spiel-Signaturen | wird gebaut |
| 2. Eigene Figur im Spiel | Du siehst dein Cosmetic in der 3. Person und im Inventar | Position und Haltung des eigenen Spielers (Signaturen `LocalPlayer`, Kamera, Skelett), Overlay zeichnet das Modell an die richtige Stelle | braucht PC-Test |
| 3. Andere sehen es | Andere Mochi-Nutzer sehen dein Cosmetic | ein kleiner Server (wer trägt was) plus Stufe 2 für fremde Spieler | später, eigenes Projekt |

Nicht-Mochi-Spieler sehen nichts. Wir ändern keine Pakete, das Cosmetic ist reine Anzeige auf deinem Bildschirm. Wie Flarial (Capes über eigene Server) ist Stufe 3 nur mit eigenem Backend möglich.

## Ideenliste

Priorität 1 (zuerst bauen, je Slot mehrere Varianten, damit das Menü sofort voll wirkt):

| Slot | Ideen |
|---|---|
| **Flügel** | Sakura-Flügel (rosa, Blütenblätter, Flügelschlag), Engelsflügel (weiß, Federn), Dämonenflügel (dunkel, Glühen), Schmetterlingsflügel (Muster, langsam), Pixel-Flügel (einfarbig, Regenbogen-Verlauf), Drachenflügel |
| **Capes** | Mochi-Cape (rosa mit Herz), Verlauf-Cape (zwei Farben, einstellbar), Sternenhimmel-Cape (dunkel, Sterne funkeln), Flammen-Cape, Königs-Cape mit Pelzkragen, Pixel-Cape mit eigenem Motiv, Mini-Umhang kurz |
| **Bandanas / Stirnbänder** | Bandana mit Knoten und flatternden Enden (Farbe einstellbar), Stirnband mit Herz, Ninja-Stirnband, Piraten-Kopftuch, Sport-Schweißband |
| **Kopf** | Heiligenschein (schwebt), Katzenohren (zucken), Hasenohren, Krone (funkelt), Kopfhörer, Sonnenbrille, Hörner, Blumenkranz, Mütze mit Bommel, Tiara |
| **Rücken** | Mini-Rucksack, Schwert auf dem Rücken, Köcher mit Pfeilen, Gitarre, Kuschel-Plüsch |
| **Körper / Hüfte** | Flauschiger Schwanz (wedelt), Fuchsschwanz mit mehreren Schwänzen, Gürtel mit Herz, Schal (weht), Ketten |
| **Details** | Schulter-Pet (kleines Mochi-Wesen), Schmetterlinge, die um dich fliegen, Funken-Spur beim Sprinten, Herzchen bei Treffern (Anzeige, nicht im Spiel) |

Priorität 2: Saison-Sets (Halloween, Winter, Sakura-Fest), Sets mit passenden Teilen (Engel-Set: Flügel, Heiligenschein, Cape), Farbvarianten und Muster-Editor, Community-Einsendungen.

Alle Cosmetics sind eigene Entwürfe, keine Marken, keine bekannten Figuren, keine Minecraft-Originalassets.

## Technischer Rahmen: Format `MochiCosmetic`

Kein Blender nötig. Die Modelle sind Quader (wie Minecraft selbst), beschrieben in JSON, mit einer PNG-Textur. Ein Python-Skript erzeugt beides und rendert Vorschaubilder. Der Client lädt die Ordner direkt.

```
cosmetics/<id>/
  item.json      Beschreibung
  tex.png        Textur, 64x64 oder 128x128, Pixel-Art, transparent erlaubt
  preview.png    Vorschau 512x512, Vorderseite und Rückseite
```

`item.json`:

```json
{
  "id": "sakura_wings",
  "name": "Sakura Wings",
  "slot": "wings",
  "tags": ["wings", "pink", "animated"],
  "texture": "tex.png",
  "tint": [{"name": "Main", "default": "#ff7eb6"}, {"name": "Accent", "default": "#ffffff"}],
  "bones": [
    {
      "name": "wing_l",
      "pivot": [2, 22, 2],
      "rotation": [0, -12, 8],
      "anim": {"type": "flap", "axis": "y", "amplitude": 25, "speed": 1.4, "phase": 0},
      "cubes": [{"origin": [2, 12, 2], "size": [10, 14, 1], "uv": [0, 0], "tint": "Main"}]
    }
  ]
}
```

Regeln:

- Einheit: 1 Einheit = 1 Pixel der Figur, die Figur ist 32 hoch (Kopf 8, Körper 12, Beine 12) und 8 tief, 16 breit (Körper 8).
- Ursprung: Mitte der Figur auf Bodenhöhe, X nach rechts, Y nach oben, Z nach vorn.
- `slot` ist einer von: `head`, `face`, `back`, `wings`, `cape`, `body`, `waist`, `shoulder`, `aura`.
- Animationen sind Voreinstellungen mit Parametern: `flap`, `sway`, `bob`, `wag`, `twitch`, `float`, `spin`, `sparkle`, `none`. Keine Skripte, nichts Ausführbares.
- Quader pro Cosmetic: höchstens 80, Textur höchstens 128x128, Ordner höchstens 200 KB. Das hält die Menü-Vorschau flüssig.
- Cape und Schal bekommen `"physics": "cloth"` mit Segmenten (zum Beispiel 6 Streifen), der Client lässt sie mit der Bewegung schwingen.
- Optional: Export nach Bedrock-Geometrie (`.geo.json`), damit die Modelle später auch in der Skin-Welt nutzbar sind.

### Festlegungen des Client-Ladecodes (`dll/src/cosmetics/`)

- Ordner: `%LOCALAPPDATA%\Mochi\cosmetics\index.json` mit `{"items": [{"id", "name", "slot"}]}` und je Cosmetic ein Ordner `<id>/item.json` + `tex.png` (nur PNG, RGBA).
- Koordinaten: X nach rechts, Y nach oben, Z nach vorn (Vorderseite der Figur ist +Z, der Rücken -Z). Die Figur steht auf Y = 0, Kopf ab Y = 24 bis 32, Körper 12 bis 24, Beine 0 bis 12.
- UV wie bei Minecraft-Quadern (Würfel mit Größe w, h, d und Ursprung u, v in der Textur): erste Reihe Oberseite (`u+d`, `v`, Breite w, Höhe d) und Unterseite (`u+d+w`, `v`), zweite Reihe Höhe h ab `v+d`: rechte Seite (-X) bei `u`, Vorderseite (+Z) bei `u+d`, linke Seite (+X) bei `u+d+w`, Rückseite (-Z) bei `u+2d+w`.
- `tint`: Name aus der `tint`-Liste des Items. Die Texturfarbe wird mit der Farbe multipliziert, deshalb Texturen hell/weiß-nah halten, wenn der Spieler die Farbe ändern soll.
- `anim.axis` ist `"x"`, `"y"` oder `"z"`, `anim.type` einer von `flap`, `sway`, `bob`, `wag`, `twitch`, `float`, `spin`. Winkel in Grad, `speed` in Schwingungen pro Sekunde.
- Beispiele zum Testen: `tools/testdata/cosmetics/` (Flügel, Cape, Bandana). Zum Ausprobieren den Ordner nach `%LOCALAPPDATA%\Mochi\cosmetics` kopieren.

## Auftrag für den Cowork-Chat

Kopiere den folgenden Text in den Cowork-Chat.

> Du baust kostenlose Cosmetics für den Minecraft-Bedrock-PvP-Client "Mochi". Es gibt kein Blender. Du arbeitest in einem Ordner `cosmetics/` im Repo `Lizziies/Mochi-3-Client` auf einer eigenen Branch `claude/cosmetics` und pushst nur dorthin. Lies zuerst `docs/COSMETICS.md` (Ideenliste und Format `MochiCosmetic`) und `CLAUDE.md`.
>
> Baue als Erstes das Werkzeug: `cosmetics/tools/build.py` (Python 3, Pillow und numpy sind erlaubt, sonst nichts). Es liest eine kurze Python-Beschreibung pro Cosmetic (`cosmetics/src/<id>.py`, mit Hilfsfunktionen für Quader, Spiegeln, Farbverläufe, Pixel-Muster) und schreibt `cosmetics/<id>/item.json`, `tex.png` und `preview.png`. `preview.png` kommt von einem eigenen kleinen Software-Renderer (Quader mit Textur, Beleuchtung, Blick von vorn, hinten und schräg, auf einer 32 Pixel hohen Spielfigur in Rosa). Prüfe jedes Ergebnis selbst an den Vorschaubildern und verbessere, bis es gut aussieht.
>
> Dann baue in dieser Reihenfolge: Sakura-Flügel, Engelsflügel, Mochi-Cape, Verlauf-Cape, Bandana mit flatternden Enden, Heiligenschein, Katzenohren, flauschiger Schwanz, Mini-Rucksack, Krone. Danach der Rest aus Priorität 1. Jedes Cosmetic soll hübsch, klar erkennbar und im Pixel-Stil des Spiels sein, mit zwei bis drei Farben, die der Spieler ändern kann (`tint`). Halte die Grenzen aus dem Dokument ein.
>
> Schreibe `cosmetics/README.md` (wie man ein neues Cosmetic hinzufügt) und `cosmetics/index.json` (Liste aller Cosmetics mit Slot, Name, Tags). Committe kurz und klein (kleingeschrieben, Imperativ). Frage nur, wenn du wirklich blockiert bist. Gib mir am Ende ein Sammelbild (alle Vorschauen in einem Raster).

Der Client-Teil (Laden, Vorschau im Menü, Farbwahl, Ausrüsten, später Stufe 2) wird im Hauptchat gebaut, damit nichts doppelt gemacht wird.
