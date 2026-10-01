# Übergaben zwischen Sessions

Kurze Notizen für die jeweils nächste Session. Neueste Einträge unten. Regeln: `docs/TEAM.md`.

## Session A (Launcher)

- Launcher ist ein C++/ImGui-DX11-Programm (nicht C# WPF wie in PLAN.md steht). `launcher/src/Ui.*` ist die plattformunabhängige UI, `tools/preview` rendert sie auf Linux in ein PNG.
- Offen: Main (Fenster, DX11), Injector, Updater, Spielstart. Siehe `docs/STATUS.md`.

## Session B (Module)

- Spiel-Schnittstelle, Demo-Daten und Effekt-Kanäle: `docs/SDK.md`. Demo-Schalter sitzt im Modul "Game Support".
- Neue Module liegen in `dll/src/modules/<ordner>/`, registriert in `Manager.cpp`. Kategorie-Zählung: `grep -c "^    add<" dll/src/modules/Manager.cpp`.
- Auf dieser Cloud-Maschine geht `apt-get install g++-mingw-w64-x86-64-posix`; damit lässt sich `x86_64-w64-mingw32-g++ -std=c++20 -fsyntax-only` auf jede Datei laufen lassen (Include-Pfade wie in `dll/CMakeLists.txt`). Der HLSL-Shader aus `post/PostFx.cpp` lässt sich mit `glslang-tools` (`glslangValidator -D -V -e ps -S frag`) auf Syntax prüfen.

### Wünsche an andere Sessions

- (Session B an C) `Module::sub()` liefert eine Unterkategorie (z. B. "Kampf-Anzeigen", "Post-Effekte", "Kamera", "Netzwerk"). Die Modulliste im ClickGUI soll danach gruppieren, mit eigener Überschrift je Unterkategorie.
- (Session B an C) "Game Support" ist ein normales Modul der Kategorie Performance mit eigener `drawSettings()`. Wenn die Info-Seite eine Zeile "Demo-Daten aktiv" zeigt, kann sie `game::demo()` aus `sdk/Game.hpp` fragen.
- (Session B an C) Module können mit `ImGui::GetBackgroundDrawList()` einen Draw-Callback als allererstes Kommando legen (Post-Effekte). Bitte in `gui::draw()` vor dem Menü-Hintergrund nichts in die Background-Liste legen, was vor `modules::frame` gezeichnet werden müsste, und die Reihenfolge `modules::frame` dann `gui::draw` beibehalten.
- (Session B an A/C) Das Netzwerk-Modul nutzt `rules::status().ip`. Falls `hook/Net.cpp` später auch den Port liefert, kann `probe::Config.port` damit gefüllt werden (aktuell Standard 19132).

## Session A → Session B (Module)

Stand 2026-10-01: Dein Branch ist in `main` gemergt (136 Module). Bitte vor der Weiterarbeit `main` in deine Branch mergen.

1. **Sprachen:** Alle sichtbaren Texte auf Englisch schreiben und über `i18n::tr` zeigen, deutsche Texte in eine eigene Tabelle `dll/src/modules/Lang_B.cpp` (Vorlage: `dll/src/core/Lang.cpp`). Deine bisherigen deutschen Texte (Beschreibungen, Einstellungs-Labels, Choices, Unterkategorien wie "Eigene Werte", "Kampf-Anzeigen", Toasts) bitte in einem Durchlauf auf Englisch umstellen und die deutschen Originale als Tabelle eintragen. Regeln: Abschnitt Languages in `CLAUDE.md`.
2. **Neues Menü:** Unterkategorien (`sub()`) erscheinen jetzt als Überschriften in der Modulliste. Die Namen dafür englisch halten.
3. **Fehlende Standard-Module** (aus Flarial/Onix/Latite): Light Overlay, Nick (clientseitig), Java View Bobbing, Movable Chat, Movable Bossbar/Title/Hotbar (prüfen), Inventory Lock, Raw Input Buffer, Hive Utils (Auto-Requeue, Stats), Replay-Clip, MaterialBin Loader, Doom, Clear Chat (falls nicht in Chat Plus).
4. **Regel 7 in `CLAUDE.md`:** Ein Spiel-Modul ist erst fertig, wenn es im Spiel wirkt, nicht nur im Menü.

## Session A → Session B: Prioritäten neu (wichtig)

Felix will keine unnötigen PvP-Spielereien mehr, sondern die Standard-Module von Flarial und Onix, perfekt umgesetzt. Grundlage: `docs/FEATURE_AUDIT.md` (alle Features nach Wichtigkeit, mit Abgleich zu unserem Stand).

- **Stopp:** keine neuen Effekt-/Spaß-/Statistik-Module mehr. Die vorhandenen Extras sind im Menü per Stufe 4 ausgeblendet (`dll/src/modules/Tiers.cpp`).
- **Als Nächstes bauen, in dieser Reihenfolge:** Hive Utils (Auto-Requeue), Animations ausbauen (Block Hit, Swing, 1.8-Look), Movable Chat/Title/Bossbar/Hotbar/Day Counter/Coordinates, Clear/Compact Chat, Inventory Lock, Java Inventory Hotkeys, Modern Keybind Handling, Nametag Modifier + Nick, Item Physics, TNT Timer, Light Overlay, Skin Stealer, Pack Changer, Subtitles, Discord RPC. Danach Lua-Scripting.
- Neue Module in `Tiers.cpp` eintragen (Stufe 1 oder 2), sonst landen sie unter "Mehr Module".
- Module haben jetzt `hold()` (Hold-Modus) und `captureDefaults()`/`resetSettings()` (Reset all). Neue Einstellungen werden im Menü automatisch in General/Style/Colors gruppiert (Farben = SettingType::Color).

## Session A → Session B: Parität mit Flarial (Pflicht, vor allem anderen)

Grundlage: `docs/FLARIAL_REAL.md` (aus dem echten Quellcode gelesen). Dort steht pro Modul, was Flarial wirklich kann und was bei uns fehlt. Reihenfolge:

1. **Hive Utils** komplett (Auto-Requeue, Solo, Team-Ausscheiden, Taste, Map Avoider, Rollen-Requeue, Death-Limit, Custom-Server-Code kopieren, Chat aufräumen, Auto-Accept, Auto Map Vote). Eigenes großes Modul, Serverregel nur Hive.
2. **Hitbox ausbauen:** 2D-Modus, Dicke (fest oder entfernungsabhängig), Deckkraft, Augenlinie, Blickrichtungslinie (Länge, Farbe), sich selbst zeigen, Java-Umschaltung.
3. **Keystrokes:** Glow, Rand, Leertasten-Breite/-Höhe, Tastenabstand, eigene Texte für WASD und LMB/RMB, CPS-Text, Highlight-Tempo.
4. **Modulspezifische Platzhalter** für das neue Feld "Format" in `TextHud` (`{lmb}`, `{rmb}`, `{X}`, `{Y}`, `{Z}`, `{D}`).
5. **Coordinates:** vertikaler Modus mit Geschwindigkeit, Koordinaten der anderen Dimension, Taste zum Kopieren.
6. **Neu bauen:** Inventory Lock (nur Werkzeuge, Doppelklick-Droppen), Modern Keybind Handling, Item Physics, TNT Timer, Nametag Modifier, Java Inventory Hotkeys.
7. **Kleine Lücken:** Zoom (Hand und Module ausblenden, Cinematic-Balken), Cinematic Camera (Balken), Auto Perspective (Schwimmen, Emote), Particle Multiplier ("Normal Hit Crit"), Swing Animations (Swing Angle), Hotbar-Auswahl-Animation, Render Options (Entities, Terrain, Item in Hand), Tab List (Köpfe, Plattform-Icons, Hervorhebung), Chat (Erwähnungs-Ton), View Model (Item-FOV, dritte Person), Custom Crosshair (PNG-Import).
8. **Nicht bauen:** FPS-/Ping-Spoof (zeigt falsche Zahlen).
9. Combo Counter (480-ms-Regel, 15-s-Reset, Negatives), Reach Counter (15-s-Reset), Hit Ping (Zeit Angriff bis Server-Bestätigung), Opponent Reach (nächster Spieler im Radius) bitte gegen die Beschreibung in `FLARIAL_REAL.md` prüfen und angleichen.

## Session A → Session B: Der große Plan

Ab jetzt gilt `docs/PLAN_B.md` (Phasen 0 bis 7, mit Hive Utils, Zeqa Utils, Hive Stats, Crystal Speed, allen fehlenden Modulen, Lua, Discord RPC). Alle früheren Wunschlisten in dieser Datei sind darin enthalten. Beginne mit Phase 0.

## Session B → Session C (HUD-Optik)

- `HudModule` hat jetzt viele neue Style-Einstellungen für jedes HUD-Modul (Padding Y, Text-Schattenversatz, Ausrichtung, Mindestbreite, Rand, Glow, Kastenschatten, Hintergrund-Blur, Rotation). Jede dieser Einstellungen trägt das Feld `Setting::style = true`. Bitte in `gui/Gui.cpp` die Lambda `isStyle` so erweitern, dass sie `st.style` (für Nicht-Farben) mitprüft, statt nur die feste Id-Liste. Bis dahin stehen die neuen Schalter unter "General".
- Neu in `modules/post/PostFx`: `post::blur(dl, min, max, rounding, radius, tint)` legt einen Blur-Callback in die Draw-Liste (eine Kopie des Backbuffers pro Frame, Gauß-Spirale mit 28 Taps, abgerundetes Rechteck). Falls ihr Blur auch für das Menü wollt, kann es genutzt werden.
- `tools/testhost` kann jetzt per `TESTHOST_SCRIPT="3:k:161;5:c:376,76;6:t:67"` Tasten (`k`), Klicks (`c`), Mausrad (`w`), Zeichen (`t`) und Entladen (`u`) ausführen und mit `TESTHOST_PATTERN=1` ein Schachbrett-Muster als Hintergrund zeichnen (für Blur-Tests).

## Session B → Session A (Eingabe-Hook)

- Inventory Lock und Modern Keybind Handling müssen wissen, ob der Mauszeiger frei ist (Inventar, Chat, Menü) oder vom Spiel gefangen. Aktuell fragt `modules/common/Options.hpp` (`mcopt::cursorFree()`) nur `GetCursorInfo` ab, das reicht vermutlich nicht für Bedrock. Bitte in `hook/Input` eine Funktion `input::cursorCaptured()` anbieten, die aus den echten Aufrufen des Spiels (`ClipCursor`, `SetCursorPos`, `ShowCursor`) den Zustand ableitet. Dann ersetze ich den Inhalt von `mcopt::cursorFree()` durch diese Funktion.

## Session B → Session A (Build)

- `dll/CMakeLists.txt` holt jetzt Lua 5.4.8 per `FetchContent` von `github.com/lua/lua` (für das Modul "Lua Scripts", siehe `docs/SCRIPTING.md`). Der erste CMake-Lauf braucht also Netz und `git`. Falls der Launcher oder die CI offline bauen, bitte die Quellen vorher zwischenspeichern oder `FETCHCONTENT_SOURCE_DIR_LUA` setzen.
- Für Discord Rich Presence braucht Mochi eine eigene Discord-Anwendung (discord.com/developers). Die Anwendungs-ID gehört in die Einstellung "Discord application ID" des Moduls; die Bilder `mochi` (groß) und `heart` (klein) müssen in der Anwendung unter Rich Presence Assets hochgeladen werden. Falls ihr eine feste ID in `core/Build.hpp` wollt, kann `modules/platform/Presence.hpp` sie als Standardwert nehmen.

## Session B → Session C (Mochi Online in der Oberfläche)

- `online::count()`, `online::users()` und `online::find(name, user)` (`modules/online/Online.hpp`) liefern, wer von den Spielern in der Tab-Liste Mochi benutzt, samt Stil (Modus, Farben, Tag, Herz). Für die Pille "♥ 4 Mochi users on this server" und die Spielerliste aus `l_players.png` reicht das. `online::heartIcon(dl, mitte, größe, farbe)` zeichnet das Herz, `online::paint(...)` malt einen Namen mit Verlauf, Regenbogen oder Puls.
- Das Modul "Mochi Online" hat ein eigenes Einstellungsfeld (`drawSettings`) mit Status, Nutzerliste und dem Knopf zum Löschen der Daten. Wer das lieber im Einstellungs-Tab haben will, kann `online::state()` und `online::forget()` benutzen.
- Der Dienst ist in `server/` (Cloudflare Worker), Anleitung in `server/README.md`.

## Session B → Session A (Eingabe, Build)

- `hook/Input.cpp`: Strg+L entlädt den Client jetzt nur noch bei echtem Strg. Toggle Sprint hält per `SendInput` Strg gedrückt, vorher entlud ein versehentliches L dann den Client. Dafür merkt sich `realCtrl` nur Tasten, die nicht von `inject::` stammen.
- `dll/CMakeLists.txt` linkt jetzt `bcrypt` (Zufallsschlüssel für Mochi Online).

## Session B → Session A (Cosmetics-Format, Erweiterungen)

Das Cosmetics-Modul und meine Vorschau sind gelöscht, die Seite im Menü und der Lader in `dll/src/cosmetics/` gelten. Für die detaillierten Flügel und Capes in `tools/cosmetics_hd/` braucht der Lader diese Erweiterungen des Formats (alle optional, alte Dateien laufen unverändert weiter). Eine vollständige Umsetzung von Lader, Physik und Zeichnen steht im Commit `a3385e5` unter `dll/src/modules/cosmetics/Cosmetics.cpp`, zum Übernehmen oder Nachbauen:

- `"texel": 4` (Item): Texeln pro Einheit, Standard 1. Bei Werten über 1 mit linearem Filter zeichnen, sonst Nearest, und das UV-Rechteck jeder Fläche um einen halben Texel verkleinern, sonst sieht man Nähte.
- Würfel mit `"flat": true`: eine einzige UV-Fläche `(u, v, Breite*texel, Höhe*texel)` für Vorder- und Rückseite, die Kanten nehmen den Texel direkt darunter (`v + Höhe*texel`). Die Rückseite ist an der x-Achse gespiegelt (das Bild läuft auf beiden Seiten in +x). `"mirror": true` dreht beides um, für linke Flügel. So passt ein ganzer Flügel oder eine Feder mit Alpha-Silhouette auf ein Rechteck.
- `"tint2"` und `"mix"` (Würfel): Farbe als Mischung zweier Tints, zum Beispiel für Verlaufs-Capes.
- Textur bis 256x256 statt 128x128.
- `"anim": {"type": "sparkle"}`: Alpha flackert pro Würfel.
- `"physics"` (Bone) als Objekt statt Text: `{"type": "spring" | "cloth", "stiffness", "damping", "inertia", "wind", "drive": {"air": [x,y,z], "sprint": [...], "sneak": [...], "speed": [...]}}`. Federn bekommen eine gedämpfte Feder pro Achse mit Nachschwingen, die Zielwinkel der Animation, Offsets je nach Zustand (Luft, Sprint, Schleichen, Tempo) und Anstöße durch Beschleunigung und Drehung. `cloth` ist eine Pendelkette über die Würfel von oben nach unten (Schwerkraft, Wind nach hinten durch Tempo und Fallen, Kopplung zwischen den Streifen, Kollision mit dem Körper). Eingabe ist ein `Motion` (Tempo vorwärts, seitlich, hoch, Drehrate, sprinten, schleichen, in der Luft), das Menü kann es aus `game::state().player` oder aus einer Demo-Bewegung füllen.

Außerdem: Die Namens-Regeln aus `docs/ONLINE.md` sind umgesetzt (kein freier Text, Herz hinter dem Namen, nur Farbe). Mochi Online liest die ausgerüsteten Cosmetics aus `ClientSettings::equipped()` und sendet sie als `worn` an den Dienst.
