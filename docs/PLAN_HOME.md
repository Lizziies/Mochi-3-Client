# Plan für den PC: Mochi im echten Minecraft verbinden, testen, besser machen als Flarial und Onix

Dieses Dokument ist für Claude Code am PC von Felix (Windows, Minecraft Bedrock GDK installiert). Alles, was in der Cloud ging, ist gebaut und unter Wine mit Demo-Daten geprüft. Jetzt kommt das, was nur mit dem echten Spiel geht. Lies zuerst `CLAUDE.md` (Regeln, Stil, Rule #1), dann `docs/STATUS.md`, `docs/PC_TEST.md`, `docs/SDK.md`, `docs/VERSIONING.md`, `docs/MODULES.md`, `docs/PARITY.md`, `docs/FLARIAL_REAL.md`, `docs/ONLINE.md`, `docs/HANDOFF.md`.

## Ziel

Der beste Minecraft-Bedrock-PvP-Client, besser als Flarial und Onix: bessere Eingabelatenz, bessere Oberfläche, mehr und tiefere Module, die wirklich im Spiel etwas tun. "Läuft" reicht nicht. Jedes Modul muss mindestens so gut sein wie das Gegenstück bei Flarial oder Onix und in etwas besser (schneller, hübscher, einstellbarer).

## Wo wir stehen (ehrlich)

- 178 Module sind registriert. Etwa 74 laufen ohne Spiel-Zugriff (Overlay). Rund 100 sind grau, weil es noch **keine einzige Signatur** gibt (`sigs/` existiert noch nicht).
- Nichts davon lief je im echten Minecraft. DX12, Launcher, Injektion und Release-Pipeline sind ungetestet.
- Mit "Game Support" → Demo-Daten laufen alle Module mit simulierten Werten. Das zeigt nur das Aussehen.
- Mochi Online: Dienst läuft bei Cloudflare (`https://mochi-online.lisawer008.workers.dev`), der Client hat die Adresse als Standard. Ende-zu-Ende mit dem Client ist nicht geprüft.
- Die detaillierten Cosmetics in `tools/cosmetics_hd/` lädt der Loader aus `main` noch nicht (Wunsch in `docs/HANDOFF.md`).

## Harte Regeln (aus CLAUDE.md, hier wiederholt, weil sie am PC gelten)

1. **Legit only.** Kein Reach, keine Killaura, kein Aim Assist, kein Autoclicker, keine Velocity, kein Scaffold, kein ESP durch Wände, nichts, was Pakete schickt, die der normale Client nicht schickt.
2. **Kein Code aus Flarial (AGPL) oder Onix kopieren.** Ansehen, um einen Ansatz zu verstehen, ist erlaubt. Signaturen selbst finden und selbst prüfen.
3. **Keine Minecraft-Dateien ins Repo.** Auch keine Speicherabbilder, keine Ghidra-Projekte mit Spielcode. Ins `.gitignore`.
4. **Rule 7:** Ein Spiel-Modul gilt erst als fertig, wenn sein Hook installiert und geprüft ist (Logzeile plus sichtbare Wirkung im Spiel). Fehlt der Hook, bleibt das Modul grau, nie stillschweigend inaktiv.
5. Fehlende Signatur darf nie abstürzen. Jeder Hook läuft im Crash-Guard (`guard::call`).
6. Stil und Commits wie in `CLAUDE.md`. Neue Texte als `i18n::tr`, deutsche Tabelle in `Lang_B.cpp` oder `LangClient.cpp`. Nach jedem Schritt eine Zeile in `docs/STATUS.md` und `docs/HISTORY.md`.
7. Ergebnisse nachweisbar machen: Logzeile, Screenshot, Messwert. Felix schickt Screenshots, wenn sich etwas Sichtbares ändert. Wenn etwas nicht klappt, im Log und in `docs/TESTLOG.md` ehrlich festhalten.

## Arbeitsweise

- Branch: `claude/modules-b` ist der aktuelle Stand der Module. Zuerst `git fetch origin`, dann prüfen, ob `main` schon alles enthält (`git log origin/main..origin/claude/modules-b`). Ist es nicht gemergt, `git merge origin/claude/modules-b` in einen Arbeitsbranch `claude/pc-test` und dort arbeiten. Nie auf eine fremde Branch pushen.
- Bauen auf Windows:
  ```
  cmake -S dll -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release
  cmake -S launcher -B build-launcher -G "Visual Studio 17 2022" -A x64
  cmake --build build-launcher --config Release
  ```
- Vor jedem Neubau die DLL mit Strg+L entladen (die Datei ist sonst gesperrt).
- Log: `%LOCALAPPDATA%\Mochi\logs\latest.log`. Selbsttest: `MOCHI_SELFTEST=1` (prüft alle Module der Reihe nach, meldet Fehler, Aussetzer, fehlende Übersetzungen). Modulliste mit Signaturbedarf: `MOCHI_DUMP_MODULES=<pfad.json>`.
- Eine Testdatei führen: `docs/TESTLOG.md` mit Datum, Minecraft-Version, was getestet wurde, Ergebnis, Beleg.

## Phase 0: Start im echten Spiel

Ziel: Der Client lädt, zeichnet, entlädt sich sauber, ohne dass das Spiel abstürzt.

- [ ] Bauen, dann den Ablauf aus `docs/PC_TEST.md` (Launcher oder Dev-Injektor). `tools/inject.ps1` gibt es noch nicht: einen kleinen Injektor schreiben, der die DLL per `LoadLibrary` in `Minecraft.Windows.exe` lädt, falls der Launcher noch nicht reicht.
- [ ] Prüfen: `hook Present`, `renderer: dx11` oder `dx12` im Log. Menü mit Rechts-Shift. FPS, CPS und Keystrokes stehen oben links.
- [ ] **DX12-Pfad prüfen** (bisher nur Code). Wenn Minecraft DX12 benutzt: Menü, Blur, Cosmetics-Vorschau, Texturen, ResizeBuffers (Fenstergröße ändern, Vollbild umschalten, Alt-Tab).
- [ ] Strg+L entlädt, Spiel läuft weiter. Danach neu injizieren geht.
- [ ] 30 Minuten spielen mit eingeschaltetem Menü-HUD: kein Absturz, kein Ruckeln, kein Speicherwachstum.
- [ ] Eingabe prüfen: Maus, Tastatur, Raw Input, Cursor-Fang im Menü (Maus frei, wenn Menü offen), keine hängenden Tasten (Toggle Sprint, Strg+L-Fix in `hook/Input.cpp`).
- [ ] Eingabelatenz und Overhead messen (siehe Phase 6) und festhalten, bevor Module Hooks setzen.

Gate: stabil, sauber entladbar, DX11 und DX12 funktionieren. Erst dann Signaturen.

## Phase 1: Version und Signatur-Werkzeuge

Ziel: Jede Signatur ist reproduzierbar, versioniert und prüfbar.

- [ ] Version aus der Exe und SHA256 loggen (siehe `docs/VERSIONING.md`). Prüfen, dass `sigs/<version>.json` geladen wird (Cache, mitgeliefert, GitHub).
- [ ] **Speicherabbild-Werkzeug** schreiben (nur Entwicklung, nicht ausliefern): ein Dev-Schalter in der DLL (`MOCHI_DUMP_IMAGE=<pfad>`) schreibt das geladene Modul `Minecraft.Windows.exe` aus dem Speicher auf die Platte, damit Ghidra es analysieren kann. Bei GDK-Builds sind die Dateien im Installationsordner geschützt, das Abbild aus dem Speicher ist der gangbare Weg. Ausgabe nie committen.
- [ ] `tools/sigcheck`: liest eine `sigs/<version>.json` und prüft gegen das laufende Spiel (oder gegen das Abbild) jede Signatur auf genau einen Treffer. Ausgabe pro Signatur: OK, 0 Treffer, mehrere Treffer, plus Adresse.
- [ ] Ghidra (headless oder GUI) mit dem Abbild einrichten. Skript, das nach Strings und Aufrufmustern sucht.
- [ ] Format prüfen: `rel` (`none`, `call`, `lea`, `mov`), mehrere Muster als Fallback, Offsets (`player.posX` usw.).

Gate: `sigcheck` läuft und ist ehrlich (findet auch kaputte Muster).

## Phase 2: Signaturen und Live-Leser in Wellen

Reihenfolge nach Nutzen. Für **jede** Signatur: finden (statisch oder zur Laufzeit mit Haltepunkt und Überwachung), robustes Muster mit Platzhaltern für verschobene Adressen, genau ein Treffer, nach Möglichkeit auf **zwei Minecraft-Versionen** gegenprüfen, dann Hook oder Leser in `dll/src/sdk/Live.cpp` beziehungsweise `Effects.cpp` einbauen, im Spiel prüfen, `sigs/<version>.json` ergänzen, `docs/TESTLOG.md` und `docs/STATUS.md` aktualisieren.

Wie viele Module an einer Signatur hängen (aus dem Modul-Dump):

| Signatur | Module | Beispiele |
|---|---|---|
| `LocalPlayer` plus Offsets | 53 | Coordinates, Direction HUD, Speed, Look Angles, Health, Experience, Zoom, FOV |
| `Inventory` | 13 | Held Item, Paperdoll, Armor HUD, Pot, Arrow, Totem Counter |
| `AttackEntity` | 10 | Reach Counter, Combo Counter, Hit Counter, Hit Ping, Session Stats |
| `Target` | 6 | Block Outline, Break Progress, Target HUD, Waila, TNT Timer |
| `ChatEvents` | 6 | Auto GG, Better Chat, Message Logger, Hive Utils, Zeqa Utils |
| `fx.handMatrix` | 4 | View Model, Animations, Block Hit, Left Hand |
| `Level`, `fx.fov`, `TabListData` | je 3 | Day Counter, Zoom, FOV Changer, Tab List, Hive Stats |

### Welle 1: Spielerdaten (schaltet über 50 Module frei)

`LocalPlayer`, `Level` und die Offsets: Position, Geschwindigkeit, Yaw, Pitch, Leben, Hunger, Sättigung, Luft, Dimension, Boden, Sprint, Schleichen, Weltzeit. Prüfen gegen F3 und `Debug Menu`. Danach die Overlay-Module durchgehen: Coordinates, Speed Display, Direction HUD, Look Angles, Health, Experience, Day Counter.

### Welle 2: Kampfgefühl

`AttackEntity` (Treffer-Ereignis), `ActorEvent` (Bestätigung vom Server), `KillEvents`, `Target`, `UseState`, `Inventory`, `Effects`. Danach Combo, Reach (nur Anzeige), Hit Counter, Hit Ping, Target HUD, Armor HUD, Potion HUD, Pot/Arrow/Totem Counter. Alle Zahlen im Spiel gegen echte Treffer abgleichen.

### Welle 3: Sichtbare Effekte (`fx.*`, Hook-Kanäle)

Zuerst die Flaggschiffe: `fx.fov` (Zoom, FOV Changer), `fx.gamma` (Fullbright), `fx.viewBob`, `fx.hurtCam`, `fx.hideHand`, `fx.sensitivity`, `fx.handMatrix` (View Model, Animations, Left Hand), `fx.lookCamera` und `fx.lookTurn` (Freelook, Snap Look), dann Zeit, Regen, Nebel, Farben, Render Options. Jeder Kanal hat eine Art (Value, Flag, Skip, Out, Int, Data) wie in `docs/SDK.md`. Passt die echte Funktion nicht zum Standard-Detour (Float-Argumente in xmm), den Detour für diesen Kanal in `Effects.cpp` anpassen. Bei jedem Kanal prüfen: wird der Hook beim Abschalten des Moduls sauber zurückgenommen?

### Welle 4: Listen und Rest

`TabListData`, `ScoreboardData`, `ChatEvents` (auch für Hive/Zeqa), `PackList`, `EntityList`, `ActorList`, `ProjectileList`, `SoundEvents`, `LightLevels`, Crystal-Kanäle (`fx.crystal*`, `fx.ghostRender`, `fx.ghostPick`) und die Hitbox über den Zeichenweg des Spiels (**nicht** als eigene Linien über den Bildschirm, das wäre ESP).

### Nach jeder Welle

- [ ] `MOCHI_SELFTEST=1` läuft ohne Fehler.
- [ ] `docs/MODULES.md` neu erzeugen (`tools/modules_doc.py`), Zahl "läuft im Spiel" aktualisieren.
- [ ] Module, die weiter grau bleiben, im Log mit Grund ("fehlt: X").
- [ ] Commit klein und klar, `sigs/<version>.json` gehört in den Commit.

## Phase 3: Server und Regeln

- [ ] Servererkennung (`server/Rules.cpp`, Netzwerk-Hooks) auf **Hive, Zeqa, CubeCraft, NetherGames, Mineville** prüfen: Name im Menü, Sperren und Warnungen.
- [ ] `servers/servers.json` gegen die echten Regelseiten der Server prüfen und korrigieren. Die Einträge für Hive, NetherGames und Mineville sind Annahmen.
- [ ] Hive Utils, Zeqa Utils, Hive Stats: Chat-Texte (Wortlisten für Sieg, Kill, Bedwars-Ereignisse), API-Felder von `api.playhive.com`. Alles gegen echte Spiele prüfen und die Annahmen im Code korrigieren.
- [ ] Prüfen, dass **kein** Modul Pakete schickt, die der normale Client nicht schickt. Auto GG und Hotkeys senden nur Tastendrücke und Chat, wie ein Spieler.
- [ ] `Kill Cleanup`, `Crystal Optimizer`, `Light Overlay`: auf den Servern prüfen, auf denen sie als riskant markiert sind.

## Phase 4: Mochi Online live

- [ ] Modul "Mochi Online" einschalten: Status "Connected", Logzeile `online: signed in as ...`.
- [ ] Zwei Konten oder ein Freund: sehen sich Herz und Namensfarbe in Tab-Liste und Chat? Hinter dem Namen, nur Farbe, kein freier Text.
- [ ] Prüfen im Cloudflare-Dashboard (Worker, Observability, Logs): Fehler, Aufrufzahl pro Spieler. Erwartung nach der Optimierung: höchstens grob 30 bis 100 Aufrufe pro Spieler und Stunde.
- [ ] "Delete my data" löscht wirklich. Wechsel zwischen Servern, Abmelden beim Beenden.
- [ ] Ausgerüstete Cosmetics gehen als `worn` mit (`ClientSettings::equipped()`). Wenn der Loader (Session A) die Format-Erweiterungen aus `docs/HANDOFF.md` bekommen hat, `tools/cosmetics_hd` ausprobieren: Flügel und Capes mit Physik.
- [ ] Offen für später: Beweis, dass ein Gamertag wirklich dem Nutzer gehört (Xbox-Anmeldung), und eine Datenschutz-Seite vor dem Release.

## Phase 5: Cosmetics im Spiel (Stufe 2)

Wenn die Spielerdaten laufen: eigene Figur in der dritten Person mit Cosmetics (Position und Haltung des eigenen Spielers aus Welle 1, Kamera, Skelett). Danach fremde Spieler (`ActorList`). Nur Anzeige auf dem eigenen Bildschirm, keine Pakete. Physik: Bewegung aus dem echten Spieler (`game::state().player`), siehe `docs/HANDOFF.md`.

## Phase 6: Messen und mit Flarial und Onix vergleichen

Rule #1: nicht raten, messen. Gleiche Szene, gleiche Einstellungen, gleiche Auflösung.

- **Eingabelatenz:** Zeit von Mausklick bis Bild. Mit Hochgeschwindigkeitskamera oder einem Photodioden-Aufbau, sonst mit dem eingebauten Latenz-HUD und Frametime. Vergleichen: Vanilla, Mochi, Flarial, Onix.
- **FPS-Kosten:** mittlere FPS, 1-%-Tiefstwerte und Frametime-Graph mit 5, 20 und allen Modulen. Die Modul-Kostenanzeige im Menü benutzen (`Module::costMs`).
- **Start- und Ladezeit** der DLL, Speicherverbrauch nach einer Stunde.
- **Stabilität:** 2 Stunden PvP, Serverwechsel, Alt-Tab, Auflösungswechsel.
- **Parität:** `docs/PARITY.md` und `docs/FLARIAL_REAL.md` Punkt für Punkt durchgehen. Pro Funktion eintragen: gleich gut, besser oder schlechter. Alles "schlechter" wird zur Aufgabe.
- **Was uns abheben soll** (im Spiel prüfen, nicht nur im Menü): Hub und lange Modulliste, Auto-Sperren pro Server, Modul-Einstellungstiefe, HUD-Look mit Blur, Cosmetics mit Physik, Mochi Online, Lua-Skripte, Discord-Anzeige, Konfig-Codes, Deutsch und Englisch.

Ergebnis: eine Tabelle in `docs/PARITY.md` mit Messwerten. Nur Zahlen, die gemessen wurden.

## Phase 7: Versionen, Updates, Veröffentlichung

- [ ] Mindestens drei Minecraft-Versionen: aktuelle, vorherige, Preview. Pro Version `sigs/<version>.json` mit `inherits`. Unbekannte Version: Module werden grau, kein Absturz.
- [ ] Windows 10 und 11, Defender (falsche Warnung bei Injektoren: Ausnahme beschreiben), Launcher-Ablauf, Selbst-Update, Version-Switcher über LeviLauncher.
- [ ] GitHub-Action (Release-Pipeline) einmal laufen lassen: Pre-Release `v0.1.0-alpha.1` mit `Mochi.dll`, `MochiLauncher.exe`, Prüfsummen. Herunterladen und Schritt für Schritt wie ein Nutzer testen.
- [ ] Vor dem ersten öffentlichen Release: Arbeitstitel "Mochi" global ersetzen (suchen/ersetzen), README im lockeren Ton, Datenschutz-Seite, Lizenz prüfen.

## Was Felix prüfen oder liefern muss

- Screenshots vom Menü, von HUD-Änderungen und von Fehlern. Beim ersten Start das Log.
- Minecraft-Version (Einstellungen im Spiel) und ob DX11 oder DX12.
- Konten für den Online-Test, Zugang zu Hive und Zeqa zum Prüfen der Chat-Texte.
- Entscheidung zu Namen, Domain (optional eigene Adresse statt `workers.dev`), Discord-Anwendung (ID und Bilder `mochi`, `heart`).

## Fertig heißt

1. Der Client läuft stabil im echten Spiel auf DX11 und DX12.
2. Die Spiel-Module sind nicht mehr grau und haben **im Spiel** eine geprüfte Wirkung. Rest ist grau mit Begründung.
3. Gemessene Eingabelatenz und FPS-Kosten sind mindestens so gut wie bei Flarial und Onix, `docs/PARITY.md` zeigt Zahlen.
4. Mochi Online und Cosmetics funktionieren mit mehreren Spielern.
5. Release-Pipeline hat einmal ein echtes Pre-Release erzeugt, das ein Fremder installieren kann.

Wenn etwas davon nicht erreicht wird, steht das ehrlich in `docs/STATUS.md` mit dem Grund. Das ist besser als eine schöne Zahl, die nicht stimmt.
