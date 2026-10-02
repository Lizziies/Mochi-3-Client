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

## Neues Menü (Entscheidung Felix)

Basis ist Vorschlag A (Palette) mit einem Schalter oben rechts, der auf B (Panels) umschaltet. Bilder in `docs/ui_proposals/` (`d_combo_list.png`, `e_combo_panels.png`).

- Obere Leiste: Logo, Kategorien (PvP, HUD, Visual, Comfort, Performance, Server), Favoriten, Cosmetics, Einstellungen, Suche (Strg+K), Schalter Liste/Panels, Edit HUD.
- Ansicht Liste: Module als Zeilen mit Symbol, Name, Kurzbeschreibung, Taste, Schalter und Zahnrad. Rechts ein Panel mit den Einstellungen des gewählten Moduls.
- Ansicht Panels: eine Spalte pro Kategorie, verschiebbar, Einstellungen klappen unter dem Modul auf.
- Hub (`f_hub.png`): Rechtsshift öffnet zuerst ein kleines Overlay mit drei großen Karten (Modules, Favorites, Cosmetics), Suchfeld und Schnellschaltern für die Favoriten. Enter öffnet das Ziel, Rechtsshift noch einmal das volle Menü.
- Einstellungs-Tab (`g_settings.png`): eigener Tab mit Chat-Tag, Watermark (Inventar, HUD), Sprache, Design, Animationen an/aus, Menügröße, Standard-Größe der HUD-Module, Modul-Voreinstellungen, Tasten und Profilen.
- Cosmetics-Tab (`h_cosmetics.png`): Karten mit Filter nach Slot, rechts 3D-Vorschau mit Drehen, Farben und Ausrüsten. Siehe `docs/COSMETICS.md`.
- Animationen: Menü blendet ein, Karten und Zeilen laufen gestaffelt ein, Schalter gleiten, Tab-Wechsel mit Schieben, Hub mit Skalierung. Alles abschaltbar im Einstellungs-Tab (Animationen aus = sofort).

### Änderung nach Rückmeldung (Felix)

- Oben nur noch vier Tabs: Modules, Favorites, Cosmetics, Settings. Keine Aufteilung in PvP, HUD usw. als Tabs.
- Modules ist eine lange Liste mit Abschnitten (PvP, HUD, Visual, Comfort, Performance, Server), wie bei Onix. Eine Sprungleiste oben springt zum Abschnitt. Bild `i_list_long.png`.
- Panels-Ansicht: jede Kategorie ist eine verschiebbare Spalte. Bild `k_panels_new.png`.
- Mochi-Nutzer in Tab-Liste und Chat erkennbar (rotes Herz, Namensfarbe, Regenbogen, Tag). Bild `l_players.png`, Konzept in `docs/ONLINE.md`.

### Zweite Änderung (Felix): schlichte Liste

Entwürfe `m1_plain_twopane.png` und `m2_plain_inline.png`. Eine einzige Liste mit allem, geordnet in Abschnitten. Reihenfolge: Server (Hive, Zeqa), HUD, PvP, Visual, Comfort, Performance. Zwischenüberschriften klein, farbig (je Abschnitt eine Pastellfarbe) und dezent, nicht auffällig. Keine Kategorie-Tabs, keine Sprungleiste.

### Dritte Änderung (Felix, 2026-10-02): Richtung Onix

Die Liste passte nicht zu einem PvP-Client, zu viele Einzelmodule. Jetzt:

- Rechts-Shift öffnet direkt das Menü (kein Hub). Links eine schmale Leiste: Alle, PvP, HUD, Visual, Nützliches, Leistung, Server, Extras, darunter Cosmetics, Settings und Edit HUD. Zahlen in der Leiste = eingeschaltete Module im Abschnitt.
- Rechts ein Raster aus Kacheln: Symbol, Name, Status ("An", "Aus", "3 von 8 an", "Braucht Spieldaten"). Klick schaltet ein Modul um, Zahnrad oder Rechtsklick öffnet seine Einstellungen. Eingeschaltete Kacheln haben einen blauen Rand und eine Linie unten.
- Zusammengefasst wird nur in der Oberfläche (`gui/Tiles.cpp`), die Module und ihre Configs bleiben einzeln. Eine Gruppen-Kachel öffnet rechts ein Panel mit allen Teilen, jeder mit Schalter und aufklappbaren Einstellungen. HUD-Teile behalten ihre eigene Position im HUD-Editor.
- Suche und Favoriten zeigen einzelne Module statt Gruppen.
- Standard-Theme "Carbon": fast schwarz, eine Akzentfarbe (Blau), 8 px Rundung, keine Verläufe, Herzen oder Funken.

### Vierte Änderung (Felix, 2026-10-02): lange Liste links wie bei Onix

Die Kacheln wirkten zu sehr nach KI (viele Symbole) und nicht nach Minecraft-Client. Jetzt:

- Rechts-Shift öffnet eine Leiste am linken Bildschirmrand, das Spiel bleibt rechts sichtbar (nur leicht abgedunkelt). Oben Name, "HUD bearbeiten", "Cosmetics", "Einstellungen", darunter Suche und Favoriten-Stern, unten Server-Status.
- Eine lange Liste zum Runterscrollen mit Abschnitten (PvP, HUD, Visual, Nützliches, Leistung, Server, Extras). Abschnitte lassen sich einklappen, Server und Extras sind standardmäßig zu (Server klappt auf, sobald man auf einem Server ist).
- Doppelungen sind zu einem Punkt zusammengefasst (`gui/Catalog.cpp`), z. B. Keystrokes = Keystrokes + CPS + Mouse Strokes. Ein Klick auf die Zeile klappt darunter die Teile auf, jeder Teil hat Schalter und eigene Einstellungen, die wiederum darunter aufklappen. Der Schalter der Gruppe merkt sich, welche Teile an waren.
- Keine Symbole mehr. Schrift Barlow, kleine Rundung, eckigere Schalter. Rahmenfarbe ist unter Einstellungen → Aussehen → "Rand" einstellbar.
- Einstellungen und Cosmetics öffnen sich als Fenster rechts neben der Liste, die Liste bleibt stehen.
- Nichts lädt mehr neu: Slot-Filter bei Cosmetics und Tabs in den Einstellungen wechseln ohne Einblend-Animation der ganzen Seite.
