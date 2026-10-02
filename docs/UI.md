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

### Fünfte Änderung (Felix, 2026-10-02): genau wie Onix, kleine Farbakzente

Die Leiste am Rand sah zu sehr nach Minecraft aus. Felix hat vier Onix-Screenshots geschickt, danach ist das Menü jetzt gebaut:

- Drei schwebende Panels, zusammen mittig: oben links die Suche ("Search..." + Favoriten-Stern), darunter die Liste, rechts daneben das Einstellungs-Panel. Jedes Panel: dunkelgrau, leicht durchsichtig, Blur dahinter (Stärke = "Menu background blur"), dünner Rand in der Rahmenfarbe, Rundung aus dem Theme. Kein Vollbild-Blur mehr, nur eine leichte Abdunklung.
- Liste: oben fest "Global Settings", "Cosmetics", "Edit HUD", danach kleine Abschnitts-Labels (PVP, HUD, …) und Zeilen. Zeile = abgerundetes Grau mit Mini-Schalter rechts. An = dunkelblau getönt, ausgewählt = heller mit Rand, gesperrt = blass mit "no data" bzw. "blocked". Gruppen zeigen "2/3". Orange Punkt = auf vielen Servern nicht erlaubt.
- Klick auf die Zeile wählt aus, der Schalter schaltet um. Rechts erscheint das Panel: großer Titel, Beschreibung, oben rechts "Enabled"/"Disabled" (Klick schaltet) und bei Einzelmodulen ein Stern.
- Einstellungs-Zeilen wie Onix: Taste = blaue "Unbind"-Pille + umrandetes Tastenfeld, Schalter = Mini-Schalter, Slider = die ganze Zeile füllt sich blau von links, Wert rechts. Farbe = blaue "Change Color"-Pille + Farbfeld. Auswahl = umrandetes Feld mit Pfeil.
- Gruppen (z. B. Combat Info) zeigen rechts ihre Teile als Zeilen mit Pfeil, Schalter und Stern; ein Klick klappt die Einstellungen des Teils darunter auf.
- Global Settings: Tabs als kleine Pillen (General, Chat & watermark, Appearance, Modules, Profiles, About), Inhalt darunter. Cosmetics im selben Panel, das Panel wird dafür breiter (animiert).
- Tippen irgendwo im Menü schreibt in die Suche. Esc leert die Suche, ein weiteres Esc schließt das Menü.
- Schrift Poppins (Regular + Medium). Standard-Theme "Slate": Panels #24252A, Zeilen #393A40, Akzent #1E7CB5, Rand #5E6068, 8 px Rundung, 86 % Deckkraft. Wer "Carbon" oder das alte Graphite hatte, bekommt Slate.
- Scrollbalken 4 px, nur sichtbar, wenn es etwas zu scrollen gibt.

### Sechste Änderung (Felix, 2026-10-02 abends): Akzentfarben, Launcher, Owner

- Akzent wählbar: 8 Farbpunkte unter Global Settings → Appearance (Blau #1E7CB5, Cyan #13899A, Grün #23905A, Lila #6E4FC4, Pink #B83D80, Rot #B8342D, Orange #C2702A, Grau #5F626B, jeweils mit hellerer Zweitfarbe). Die Theme-Presets sind nur noch dunkle Onix-Looks (Slate, Onyx, Steel, Glass) und behalten den gewählten Akzent. Keine Verläufe, Glitzer oder Herzchen mehr im Menü.
- Einstellungen-Tabs als Pillen über dem Inhalt statt seitlicher Leiste. Toasts unten rechts im Panel-Stil.
- Launcher im selben Look: Hintergrund #1C1D21, Seitenleiste #17181B, Karten #26272C mit Rand #393B42, flache Knöpfe, kleinere Schalter. Logo und Icon: blaues Pixel-Herz. Unter Settings dieselben 8 Akzentfarben; ohne eigene Wahl nimmt der Launcher die Farbe aus dem Client.
- Owner-Abzeichen: hinter `vlisya` steht in Tab-Liste und Chat `[Owner]` in Blau (#3BA7EC), vergeben über die Rolle in Mochi Online. `[Team]` für Staff ist vorbereitet.
