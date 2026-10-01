# UI

Look: süß, girly, sauber. Pastell-Pink auf dunklem Pflaume-Lila, weiche Ecken, kleine Glitzer- und Herz-Akzente, flüssige Animationen. Nicht kitschig überladen: wenige Akzente, viel Ruhe.

Logo: `assets/logo_a.jpg` / `assets/logo_b.jpg` (Pixel-Herz mit Glanz). Eins auswählen, als PNG mit transparentem Hintergrund freistellen, daraus `.ico` für Launcher und Icons in 16/32/64/256 px erzeugen.

## Farben (Preset "Bubblegum", Standard)

| Token | Wert | Verwendung |
|---|---|---|
| bg | `#1A0F1E` | Fensterhintergrund (85 % Deckkraft + Blur) |
| surface | `#2A1730` | Karten, Modul-Kacheln |
| surface-hover | `#36203D` | |
| accent | `#FF7EB6` | aktiv, Toggles, Slider |
| accent-2 | `#FFB3D1` | Verlauf, Highlights |
| text | `#FFF1F7` | |
| text-dim | `#C9A9BB` | Beschreibungen |
| ok | `#8BE9B0` | |
| warn | `#FFD27E` | server-rules-Hinweis |
| off | `#6B5570` | deaktivierte Module |

Weitere Presets: **Sakura** (rosa/weiß, hell), **Lavender** (lila), **Strawberry Milk** (hell, Creme + Pink), **Midnight Pink** (fast schwarz + Neon-Pink). Alle Tokens im Theme-Editor einzeln einstellbar, plus Eckenradius, Blur-Stärke, Deckkraft, Schrift, Animationsgeschwindigkeit, Akzent-Verlauf an/aus, Glitzer an/aus. Themes als Code exportieren/importieren.

Schrift: rund und gut lesbar, z. B. "Nunito" oder "Quicksand" (OFL-Lizenz, einbetten).

## ClickGUI (Rechts-Shift)

- Öffnen: Fade + leichtes Hochskalieren (150 ms), Hintergrund weichgezeichnet.
- Links Seitenleiste mit Icons: Alle, HUD, Visuell, PvP, Komfort, Server, Spaß, Scripts, Themes, Einstellungen. Oben das Logo.
- Oben Suchleiste ("Modul suchen …"), tippt man los, springt der Fokus automatisch rein.
- Rechts Raster aus Modul-Kacheln: Icon, Name, kurze Beschreibung, Toggle-Pille. Klick auf Zahnrad öffnet Einstellungen als Slide-Panel von rechts.
- Graue Kachel + Schloss bei "nicht verfügbar auf dieser Version", gelbes Dreieck bei `server-rules`.
- Unten links: Button "HUD bearbeiten", Config-Profil-Auswahl.
- Kleine Herz-Partikel beim Einschalten eines Moduls (abschaltbar).

## HUD-Editor

- Spiel abgedunkelt, alle HUD-Elemente mit gestrichelter Umrandung.
- Ziehen, Mausrad = Größe, Rechtsklick = Einstellungen, Doppelklick = Position und Größe zurücksetzen, Pfeiltasten = 1 px verschieben (Shift = 10 px).
- Hilfslinien und Einrasten an Rändern, Mitte und den Rändern und Mitten anderer Elemente (gebaut). Shift beim Ziehen schaltet das Einrasten ab. Bei Überlappung wird das kleinere Element gegriffen.
- Pro Element: Hintergrund an/aus, Farbe, Eckenradius, Textschatten, Ausrichtung.

## Launcher

- Fenster ca. 960×600, randlos, eigene Titelleiste, gleiche Farben.
- Links: Logo, Navigation (Start, Versionen, Einstellungen, Über).
- Start: großer Button "Spielen ♡" mit Zustand (Spiel startet → injiziert → fertig), darunter Version und News/Changelog aus dem GitHub-Release.
- Versionen: Liste mit installiert/verfügbar, Badge "Mochi-kompatibel".
- Einstellungen: Update-Kanal, Auto-Inject, DLL-Pfad (für Entwickler), Sprache (DE/EN).
