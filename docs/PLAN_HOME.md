# Plan für den PC: Mochi im echten Minecraft verbinden, testen, besser machen als Flarial und Onix

Dieses Dokument ist für Claude Code am PC von Felix (Windows, Minecraft Bedrock GDK installiert). Alles, was in der Cloud ging, ist gebaut und unter Wine mit Demo-Daten geprüft. Jetzt kommt das, was nur mit dem echten Spiel geht. Lies zuerst `CLAUDE.md` (Regeln, Stil, Rule #1), dann `docs/STATUS.md`, `docs/PC_TEST.md`, `docs/SDK.md`, `docs/VERSIONING.md`, `docs/MODULES.md`, `docs/PARITY.md`, `docs/FLARIAL_REAL.md`, `docs/ONLINE.md`, `docs/HANDOFF.md`.

## Reihenfolge (verbindlich)

1. **Zuerst den ganzen Code holen.** Im Ordner des Repos: `git fetch origin`, dann `git checkout -B claude/pc-test origin/claude/onix-ui-input-fixes`. Dieser Branch enthält `main` vollständig plus das neue Menü, den neuen Launcher, Rollen in Mochi Online und alle Reparaturen vom 2. Oktober. Die konkrete Arbeitsliste steht in `docs/HOME_TODO.md`. Danach einmal bauen (Befehle unten) und `MOCHI_SELFTEST=1` laufen lassen, damit klar ist, womit du startest.
2. **Alles testen und reparieren, im Terminal auf dem PC** (Phasen 0 bis 6). Gebaut wird dabei lokal mit `cmake`, die DLL wird mit dem Dev-Weg oder dem Launcher in das echte Minecraft geladen.
3. **Die fertige Exe zum Schluss** (Phase 7): erst wenn alles geprüft ist, die einzelne `MochiLauncher.exe` bauen (lokal im Terminal mit `-DMOCHI_DLL` und `-DMOCHI_COSMETICS`) und als Release veröffentlichen (Tag pushen, die GitHub-Action baut dieselbe Exe). Vorher keine Exe verteilen.

## Kein Modul darf ausgegraut bleiben

Das ist das wichtigste Ziel dieses Plans. Heute sind rund 100 Module grau, weil Signaturen fehlen. Zum Release darf **kein Modul grau sein**. Ein graues Modul zeigt dem Nutzer ein Feature, das nichts tut, und das ist schlechter als keins.

Für jedes graue Modul gibt es genau drei Wege, in dieser Reihenfolge:

1. **Reparieren:** Signatur oder Datenquelle finden, Hook einbauen, im Spiel prüfen (Logzeile plus sichtbare Wirkung). Das ist der Normalfall, und es gilt für alle.
2. **Anders lösen:** Geht der geplante Weg nicht (zum Beispiel weil Bedrock die Daten gar nicht an den Client schickt), eine andere Quelle nehmen (Scoreboard, Chat-Text, Server-API) und das Modul darauf umbauen.
3. **Entfernen:** Geht es wirklich nicht, fliegt das Modul aus der Liste (`Manager.cpp`, `Tiers.cpp`, `docs/MODULES.md`), mit einer Zeile in `docs/STATUS.md`, warum. Nie als graues Modul ausliefern.

Die einzige Ausnahme sind Module, die ein **anderer Server** sperrt (Server-Regeln, `servers/servers.json`). Die sind auf diesem Server absichtlich aus und mit Grund beschriftet, aber auf allen anderen Servern an.

Fortschritt messen: beim Start schreibt der Client `usable:` und `locked:` ins Log (`%LOCALAPPDATA%\Mochi\logs\latest.log`). Die Liste hinter `locked:` muss am Ende leer sein. Nach jeder Welle in `docs/STATUS.md` die Zahl eintragen (grau vorher, grau nachher).

Zeitplan: Die Wellen in Phase 2 sind nach Nutzen sortiert (53 Module hängen allein an `LocalPlayer`), damit die Zahl schnell fällt. Es wird nicht weitergegangen zu Phase 6 oder 7, solange Module grau sind, außer Felix sagt es ausdrücklich.

## Ziel

Der beste Minecraft-Bedrock-PvP-Client, besser als Flarial und Onix: bessere Eingabelatenz, bessere Oberfläche, mehr und tiefere Module, die wirklich im Spiel etwas tun. "Läuft" reicht nicht. Jedes Modul muss mindestens so gut sein wie das Gegenstück bei Flarial oder Onix und in etwas besser (schneller, hübscher, einstellbarer).

## Wo wir stehen (ehrlich)

- 178 Module sind registriert. Etwa 74 laufen ohne Spiel-Zugriff (Overlay). Rund 100 sind grau, weil es noch **keine einzige Signatur** gibt (`sigs/` existiert noch nicht).
- Nichts davon lief je im echten Minecraft. DX12, Launcher, Injektion und Release-Pipeline sind ungetestet.
- Mit "Game Support" → Demo-Daten laufen alle Module mit simulierten Werten. Das zeigt nur das Aussehen.
- Mochi Online: Dienst läuft bei Cloudflare (`https://mochi-online.lisawer008.workers.dev`), der Client hat die Adresse als Standard. Ende-zu-Ende mit dem Client ist nicht geprüft.
- Cosmetics: 21 Teile (Flügel, Capes, Ohren, Schwänze, Bandana, Mütze, Zaubererhut, Sneaker) laden im Menü mit Farbwahl, Animation und Physik (`tools/cosmetics/build.py`, Ordner `cosmetics/`). Im Spiel gezeichnet werden sie noch nicht.
- Der Launcher ist eine einzige Exe: Client und Cosmetics stecken als Ressource darin (`-DMOCHI_DLL`, `-DMOCHI_COSMETICS`). Unter Wine geprüft: `MochiLauncher.exe --extract` legt Client und Cosmetics nach `%LOCALAPPDATA%\\Mochi`. Im echten Windows nicht getestet.
- Der Launcher hat eine Versionswahl (Seite Versions). Der direkte Start einer Exe außerhalb des Stores ist ungeprüft.

## Harte Regeln (aus CLAUDE.md, hier wiederholt, weil sie am PC gelten)

1. **Legit only.** Kein Reach, keine Killaura, kein Aim Assist, kein Autoclicker, keine Velocity, kein Scaffold, kein ESP durch Wände, nichts, was Pakete schickt, die der normale Client nicht schickt.
2. **Kein Code aus Flarial (AGPL) oder Onix kopieren.** Ansehen, um einen Ansatz zu verstehen, ist erlaubt. Signaturen selbst finden und selbst prüfen.
3. **Keine Minecraft-Dateien ins Repo.** Auch keine Speicherabbilder, keine Ghidra-Projekte mit Spielcode. Ins `.gitignore`.
4. **Rule 7:** Ein Spiel-Modul gilt erst als fertig, wenn sein Hook installiert und geprüft ist (Logzeile plus sichtbare Wirkung im Spiel). Fehlt der Hook, bleibt das Modul grau, nie stillschweigend inaktiv.
5. Fehlende Signatur darf nie abstürzen. Jeder Hook läuft im Crash-Guard (`guard::call`).
6. Stil und Commits wie in `CLAUDE.md`. Neue Texte als `i18n::tr`, deutsche Tabelle in `Lang_B.cpp` oder `LangClient.cpp`. Nach jedem Schritt eine Zeile in `docs/STATUS.md` und `docs/HISTORY.md`.
7. Ergebnisse nachweisbar machen: Logzeile, Screenshot, Messwert. Felix schickt Screenshots, wenn sich etwas Sichtbares ändert. Wenn etwas nicht klappt, im Log und in `docs/TESTLOG.md` ehrlich festhalten.

## Arbeitsweise

- Branch: Arbeitsbranch `claude/pc-test`, abgezweigt von `origin/claude/onix-ui-input-fixes` (enthält `main` und `claude/modules-b`). Nie auf eine fremde Branch pushen.
- Bauen auf Windows (der Launcher bettet Client und Cosmetics ein, das ergibt eine einzige Exe):
  ```
  cmake -S dll -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release
  cmake -S launcher -B build-launcher -G "Visual Studio 17 2022" -A x64 -DMOCHI_DLL=%CD%\build\Release\Mochi.dll -DMOCHI_COSMETICS=%CD%\cosmetics
  cmake --build build-launcher --config Release
  ```
  Ohne `-DMOCHI_DLL` sucht der Launcher `Mochi.dll` neben sich (Entwicklung). Cosmetics neu erzeugen: `python3 tools/cosmetics/build.py cosmetics`.
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

- [ ] **Einzige Exe prüfen:** `MochiLauncher.exe` allein in einen leeren Ordner kopieren und starten. Danach liegen `%LOCALAPPDATA%\Mochi\bin\Mochi.dll` und der Ordner `cosmetics` da (`MochiLauncher.exe --extract` macht nur das, ohne Fenster). Play startet Minecraft und verbindet.
- [ ] **Versionswahl prüfen** (Launcher, Seite Versions): LeviLauncher installieren, dort eine ältere Version laden, "Rescan" (sonst "Add folder"), "Use this one", Play. Die Exe wird direkt gestartet, das ist ungeprüft: Läuft sie, sieht man die Version im Spiel und im Log (`version ...`). Schließt sie sich sofort, ist der direkte Start der falsche Weg. Dann herausfinden, wie LeviLauncher die Version startet (Registrierung des Pakets, Aufruf), und den Start in `launcher/src/Game.cpp` (`launchExe`) anpassen. Nach Neustart des Launchers muss die Wahl bleiben ("In use"). Nie Dateien im Store-Ordner überschreiben.

Gate: stabil, sauber entladbar, DX11 und DX12 funktionieren. Erst dann Signaturen.

## Phase 1: Version und Signatur-Werkzeuge

Ziel: Jede Signatur ist reproduzierbar, versioniert und prüfbar.

- [ ] Version aus der Exe und SHA256 loggen (siehe `docs/VERSIONING.md`). Prüfen, dass `sigs/<version>.json` geladen wird (Cache, mitgeliefert, GitHub).
- [ ] **Speicherabbild-Werkzeug** schreiben (nur Entwicklung, nicht ausliefern): ein Dev-Schalter in der DLL (`MOCHI_DUMP_IMAGE=<pfad>`) schreibt das geladene Modul `Minecraft.Windows.exe` aus dem Speicher auf die Platte, damit Ghidra es analysieren kann. Bei GDK-Builds sind die Dateien im Installationsordner geschützt, das Abbild aus dem Speicher ist der gangbare Weg. Ausgabe nie committen.
- [ ] `tools/sigcheck`: liest eine `sigs/<version>.json` und prüft gegen das laufende Spiel (oder gegen das Abbild) jede Signatur auf genau einen Treffer. Ausgabe pro Signatur: OK, 0 Treffer, mehrere Treffer, plus Adresse.
- [ ] Ghidra (headless oder GUI) mit dem Abbild einrichten. Skript, das nach Strings und Aufrufmustern sucht.
- [ ] Format prüfen: `rel` (`none`, `call`, `lea`, `mov`), mehrere Muster als Fallback, Offsets (`player.posX` usw.).

Gate: `sigcheck` läuft und ist ehrlich (findet auch kaputte Muster).

Hinweis zur Arbeitsmenge: Es gibt noch keine einzige Signatur. Das Ziel "nichts grau" heißt, dass diese Phase und Phase 2 der Hauptteil der Arbeit sind. Nicht abkürzen, nicht Module als "fertig" melden, die im Spiel nichts tun.

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

Reihenfolge nach Felix' Wunsch (Details in `docs/HOME_TODO.md`, Punkt 4): erst alle Wellen im Code fertig, dazwischen nur ein kurzer Rauchtest (Spiel startet, kein Absturz, Log ohne Fehler). Die Prüfung im Spiel Modul für Modul kommt danach in einem Durchgang.


- [ ] `MOCHI_SELFTEST=1` läuft ohne Fehler.
- [ ] `docs/MODULES.md` neu erzeugen (`tools/modules_doc.py`), Zahl "läuft im Spiel" aktualisieren.
- [ ] Module, die nach der Welle noch grau sind, mit Grund ("fehlt: X") in `docs/STATUS.md` listen. Sie müssen bis zum Release repariert, anders gelöst oder entfernt sein.
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

Ziel: Jeder Mochi-Nutzer sieht die Cosmetics aller anderen Mochi-Nutzer (Flügel, Capes, Ohren, Schwänze, Bandana, Mützen, Sneaker), auch an der eigenen Figur in der dritten Person. Wer den Client nicht hat, sieht nichts. Es werden keine Pakete verändert.

- **Nicht als Overlay malen.** Ein Overlay über dem Bild kennt keine Wände und würde Spieler hinter Blöcken zeigen. Das ist ein Wallhack und verboten. Der Weg ist, die Teile in die Figur selbst zu geben: Skin-Geometrie des Spielers beim Laden des Skins erweitern oder den Render-Aufruf der Figur hooken und die Teile mit der Skelett-Matrix des Spielers zeichnen. Dann sortiert das Spiel selbst, Sneaken, Schwimmen, Gleiten und Sichtbarkeit stimmen automatisch.
- Daten pro Figur: Skelett-Knochen (Kopf, Körper, Arme, Beine), Slim- oder Wide-Körper (Skin-Geometrie), Skin-Overlay-Schicht (Hut und Jacke, das Bandana liegt bei Radius 4.42, Hut-Schichten gehen bis 4.5, bei Bedarf anpassen). Skins mit eigener Körperform erkennen und dort ausblenden.
- Wer trägt was: Mochi Online (`worn` je Gamertag, Phase 4). Der Client fragt für die Spieler in der Tab-Liste ab.
- Physik: Bewegung aus dem echten Spieler (`game::state().player`), Anschlussstelle `cosmetics::Rig::step` mit `cosmetics::Moving` (siehe `dll/src/cosmetics/Preview.cpp`, Physik ist dieselbe wie in der Menü-Vorschau).
- Prüfen mit zwei Konten: beide sehen die Teile des anderen, hinter Wänden sieht man nichts, ohne Mochi sieht man nichts, die FPS-Kosten stimmen (Messung in Phase 6).

Dazu kommen zwei Module, die ebenfalls im Spiel geprüft werden müssen:

- **Third Person Nametag** (Session B): eigener Name über dem Kopf in F5 und Freecam. Braucht den Hook `fx.selfNametag`. Prüfen: Name steht in der dritten Person über dem Kopf, in der ersten nicht (außer Option).
- **Health Above Head** (neu, `dll/src/modules/world/HealthAbove.hpp`): Balken und Zahl über anderen Spielern. Braucht die Liste der anderen Spieler (`state.others`, `ActorList`) mit Leben und Höchstleben. Bedrock sendet fremdes Leben nicht immer; wenn nur Scoreboard-Werte unter dem Namen kommen (Server-Anzeige), diese lesen. Ausgeliefert ist es aus (Server-Regeln). Prüfen: Werte stimmen mit dem überein, was das Spiel oder der Server anzeigt.

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
- [ ] GitHub-Action (Release-Pipeline) einmal laufen lassen: Pre-Release `v0.1.0-alpha.1`. Das Release enthält nur `MochiLauncher.exe` (eine einzige Exe, Client und Cosmetics eingebettet wie bei Flarial) und `checksums.txt`. Herunterladen und Schritt für Schritt wie ein Nutzer testen, auch das Selbst-Update auf ein zweites Pre-Release.
- [ ] Vor dem ersten öffentlichen Release: Arbeitstitel "Mochi" global ersetzen (suchen/ersetzen), README im lockeren Ton, Datenschutz-Seite, Lizenz prüfen.

## Was Felix prüfen oder liefern muss

- Screenshots vom Menü, von HUD-Änderungen und von Fehlern. Beim ersten Start das Log.
- Minecraft-Version (Einstellungen im Spiel) und ob DX11 oder DX12.
- Konten für den Online-Test, Zugang zu Hive und Zeqa zum Prüfen der Chat-Texte.
- Entscheidung zu Namen, Domain (optional eigene Adresse statt `workers.dev`), Discord-Anwendung (ID und Bilder `mochi`, `heart`).

## Fertig heißt

1. Der Client läuft stabil im echten Spiel auf DX11 und DX12.
2. **Kein Modul ist grau.** Jedes Modul hat im Spiel eine geprüfte Wirkung, oder es wurde anders gelöst oder entfernt (siehe "Kein Modul darf ausgegraut bleiben"). Die `locked:`-Liste im Log ist leer.
3. Gemessene Eingabelatenz und FPS-Kosten sind mindestens so gut wie bei Flarial und Onix, `docs/PARITY.md` zeigt Zahlen.
4. Mochi Online und Cosmetics funktionieren mit mehreren Spielern.
5. Die einzelne `MochiLauncher.exe` wurde erst nach allen Tests gebaut, die Release-Pipeline hat einmal ein echtes Pre-Release erzeugt, das ein Fremder installieren kann.

Wenn etwas davon nicht erreicht wird, steht das ehrlich in `docs/STATUS.md` mit dem Grund. Das ist besser als eine schöne Zahl, die nicht stimmt. Ein graues Modul im Release zählt aber nie als erreicht.
