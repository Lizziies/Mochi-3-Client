# Übergabe: PC-Sitzung an Cloud-Sitzung

Stand 2026-10-02, Branch `claude/pc-test`, geschrieben auf Marcs PC (Windows 11, Minecraft Bedrock GDK installiert) für eine Claude-Code-Sitzung in der Cloud, die ohne Minecraft weiterarbeitet. Regeln und Stil stehen in `CLAUDE.md`, der Gesamtplan in `docs/PLAN_HOME.md`, jeder Beleg mit Datum in `docs/TESTLOG.md`. Die älteren Docs nennen den Nutzer Felix, dieses Dokument hält sich an die Anrede im Auftrag (Marc).

Der Branch liegt 18 Commits vor `origin/main`. Einen `origin/claude/pc-test` gibt es laut den lokalen Remote-Refs nicht, die PC-Arbeit ist also erst sichtbar, wenn Marc pusht (siehe Abschnitt 5).

## 1. Stand

1. Getestet wurde nur gegen Minecraft 1.26.52.3 (GDK) mit DX12 (`renderer: dx12 (11on12)`). Der Client baut mit MSVC 19.44 ohne Fehler (`Monchi.dll`, 4,3 MB) und lädt per Dev-Injektor in `Minecraft.Windows.exe`. TESTLOG nennt die Version einmal als `1.26.5203.0`, der Client loggt `1.26.52.3`.
2. Belegt im echten Spiel: Hooks auf Present, ResizeBuffers, Present1, ExecuteCommandLists, sendto, WSASendTo, getaddrinfo, ClipCursor, SetCursorPos, GetCursorPos, GetAsyncKeyState, GetKeyState, GetRawInputData, GetRawInputBuffer, PeekMessageW, GetMessageW. HUD (FPS, CPS, Keystrokes) und Hub-Menü mit Blur erscheinen, Strg+L entlädt sauber, die Server-IP wird erkannt (Logzeile `server:`).
3. Bestätigt nach Neustart: das HUD ist im Hauptmenü weg, auch bei offenem Monchi-Menü. RTSS (`RTSSHooks64.dll`) im selben Prozess stört die Hooks nicht.
4. Im Code behoben, im Spiel noch nicht bestätigt: Esc öffnet nicht mehr Minecrafts Beenden-Dialog, Rechts-Shift im Hauptmenü, Toast oben links, Eingabesperre mit sofortigem Loslassen gehaltener Tasten, Maus im Menü, DX12-Absturz beim Vollbild-Wechsel (Back-Buffer werden pro Frame gewrappt und freigegeben).
5. Phase-0-Gate nicht erreicht: Fenstergröße, Vollbild, Alt-Tab, 30-Minuten-Lauf, DX11 erzwingen, Eingabelatenz und Versionswahl sind ungeprüft.
6. Signaturen: keine. Es gibt kein `sigs/`, das Log sagt `sigs: 0/0`. 179 Module sind registriert, 107 stehen hinter `locked:` (grau). Ziel laut Plan ist eine leere `locked:`-Liste zum Release.
7. Die Strukturfakten aus Abschnitt 2 sind belegt, aber noch in keine Signatur, keinen Offset und keinen Live-Leser übernommen. Position, Leben, Hunger und Co. sind noch nicht eindeutig zugeordnet.
8. Launcher: die Einzel-Exe baut (5,9 MB), `MonchiLauncher.exe --extract` legt Client und Cosmetics nach `%LOCALAPPDATA%\Monchi`. Play, Spielstart, Auto-Inject, Versionswahl und der Direktstart einer Exe sind ungeprüft.
9. Ohne Minecraft geprüft: Selbsttest 177 Module mit Demo-Daten, 0 Fehler und 0 Aussetzer, Server-Tests 17 von 17, `i18n_check.py` ohne fehlende Einträge, MSVC /W4 nur 7 Kleinigkeiten, `/analyze`-Befunde behoben.
10. Ungeprüft im Spiel: Monchi Online (der Worker antwortet auf `GET /v1/health` mit `{"ok":true,"online":0}`), Cosmetics im Spiel, Discord-Anwendungs-ID, Hive-/Zeqa-Chattexte und Hive-API-Felder.

Offene lokale Änderungen beim Schreiben (nicht committet, existieren nur auf Marcs PC): `dll/src/sdk/Explore.cpp` (neue Explorer-Funktion `callf`) und `tools/explore/probe.lua`.

## 2. Fakten zur Spiel-Struktur (Minecraft 1.26.52.3)

Gefunden mit dem Dev-Explorer per Disassembly und Heap-Suche (TESTLOG 2026-10-01 abends). Alle RVAs gelten nur für genau diesen Build. Nach jedem Minecraft-Update müssen sie neu abgeleitet werden, und auch die vtable-Indizes können sich verschieben, sobald Klassen virtuelle Funktionen dazubekommen. Haltbar sind die Wege (Zeichenkette, Xref, vtable), nicht die Zahlen.

| Fakt | Wert |
|---|---|
| RTTI für Spielklassen | fehlt. Nur 14.699 Namen für Standardbibliothek-Typen und Lambdas |
| Funktionsnamen | stehen als vollständige Signatur-Zeichenketten in der Exe, zum Beispiel `virtual void __cdecl Player::tickWorld(const Tick &)`, `MinecraftGame::update`, `GameRenderer::renderCurrentFrame(float)`, `ClientInstance::requestLeaveGame` |
| Weg zu einer Funktion | Zeichenkette suchen (`rt.bytes`), Xref (`rt.xrefs`), Funktionsgrenzen über die Unwind-Tabelle (`rt.func`), vtable-Slots über `vtablesWith` in `tools/explore/lib.lua` |
| `ClientInstance` vtable | RVA `0e9731b0`, gefunden über `requestLeaveGame` (Index 15). Vier Instanzen liegen im Heap (`rt.heap(vtable)`) |
| `ClientInstance::getLocalPlayer` | vtable-Index `0x540/8` (= 168), Funktion RVA `05dc2630`. `__fastcall(this)` liefert einen gültigen `LocalPlayer*` |
| `LocalPlayer` | vtable RVA `0e7adab0`, 419 Einträge |
| Spielerzustand | vtable-Index `0x560/8` (= 172) von `LocalPlayer`, Aufruf mit `this` liefert den Zeiger auf die Zustandsdaten. Viele Getter lesen daraus, zum Beispiel an `0x368`, `0x37c`, `0x3b8`, `0x3e0`, `0x4ab` bis `0x4c4` |
| Datenmodell | die Entity-Daten liegen in einem ECS (`LocalPlayerComponent`, `entt::...`). Feste Feldoffsets am Player-Objekt sind nicht zu erwarten, getter-Aufrufe über die vtable sind der wahrscheinlichere Weg |
| Position | offen. Für die angezeigte Position (3, 109, 57) gab es zwei Heap-Treffer ohne Gegenprobe (`tools/explore/findpos.lua`). Zustandsdumps in vier Spielsituationen: `snap_a` bis `snap_d` |
| Menü-Eingabe des Spiels | im Menü fragt das Spiel per `GetCursorPos` (hunderte Aufrufe pro Sekunde), `GetKeyState` und `PeekMessageW` ab, Raw Input wird dort nicht gelesen (`RawData=0`, `RawBuffer=0`). Die UI läuft über `cohtml.WindowsDesktop.dll` |

Folgerung, die noch nicht im Spiel geprüft ist: `dll/src/sdk/Live.cpp` geht heute von `sigs::address("LocalPlayer")` als Zeiger und festen Offsets (`player.posX` usw.) aus. Zu den Funden passt eher der Weg ClientInstance (vtable im Heap oder ein Hook, der `this` liefert), dann `getLocalPlayer`, dann Getter über die vtable. Das Format von `sigs/<version>.json` (Muster, `rel`, Offsets, siehe `docs/VERSIONING.md`) hat dafür noch keine Form.

## 3. Werkzeuge

Gebaut wird auf dem PC mit den Befehlen aus `CLAUDE.md`. Die Ordner `build*` und `dev-data` sind in `.gitignore`.

**`tools/inject.ps1`** (Dev-Injektor, nur Windows). Parameter: `-Dll` (Standard `build\Release\Monchi.dll`), `-WaitSeconds` (120), `-ProcessId` (direkt in diesen Prozess), `-Launch` (startet Minecraft über `shell:AppsFolder\Microsoft.MinecraftUWP_8wekyb3d8bbwe!Game`, versucht es alle 30 s erneut), `-Dev` (nimmt `build-dev\Release\Monchi.dll` und legt den Explorer-Marker an). Das Skript kopiert die DLL als `dev-data\bin\Monchi-<Zeitstempel>.dll` (alte Kopien werden gelöscht), schreibt `Monchi.root` mit dem Datenordner `dev-data\data` daneben und gibt der Kopie Leserechte für App-Pakete. Es wartet auf Fenster plus `d3d12.dll` oder `d3d11.dll`, wartet 2 s und injiziert per `CreateRemoteThread` und `LoadLibraryW`; bei Fehlern meldet es den echten Fehlercode. Läuft `Monchi.dll` schon, meldet es `already injected`. Grund für `dev-data`: Laden aus `%LOCALAPPDATA%\Monchi\bin` schlug einmal mit Fehler 126 fehl (Ursache nicht belegt, Verdacht Dateirechte der aus der Sandbox kopierten Datei).

**`dev-data/`** (gitignored): `bin/` (DLL, `Monchi.root`, `Monchi.explore`), `data/` (Datenordner des Clients mit `logs\latest.log`, `out\`, `configs\`, `settings.json`), `test/` (Testhost mit eigener DLL-Kopie), `launcher-test/`. Das Log liegt in `dev-data\data\logs\latest.log`. Mit dem Launcher gilt stattdessen `%LOCALAPPDATA%\Monchi`.

**`dev.cmd`-Kanal** (`dll/src/gui/DevCmd.cpp`). Aktiv nur, wenn `Monchi.root` neben der DLL liegt, also mit `inject.ps1`, nie mit dem Launcher. Eine Textdatei `dev-data\data\dev.cmd` anlegen, eine Zeile pro Befehl. Der Client prüft alle 20 Frames, liest die Datei, löscht sie und loggt jede Zeile als `dev command: ...`.

| Befehl | Wirkung |
|---|---|
| `open`, `close` | Hub öffnen, Menü und HUD-Editor schließen |
| `page modules\|cosmetics\|settings` | Seite öffnen (alles andere ergibt den Hub) |
| `settings <n>` | Settings-Unterseite n |
| `module <Name>` | Modulpanel öffnen |
| `hudedit`, `more`, `search <Text>` | HUD-Editor, "More modules", Suche |
| `demo on\|off`, `enable <Modul>` | Demo-Daten in "Game Support", Modul einschalten |
| `shot <Name>` | Screenshot der letzten Stufe (`Final`, mit Monchi-Overlay) nach `dev-data\data\out\<Name>.png` |
| `explore <Skript>` | Explorer-Skript ausführen (Name ohne `.lua`) |

**Dev-Explorer** (`dll/src/sdk/Explore.cpp`, `tools/explore/*.lua`). Nur im Dev-Build:
`cmake -S dll -B build-dev -G "Visual Studio 17 2022" -A x64 -DMONCHI_DEV=ON`, `cmake --build build-dev --config Release`, dann `inject.ps1 -Dev`. Capstone 5.0.3 kommt per FetchContent. Im Release-Build ist der Code ausgeschlossen (`#ifdef MONCHI_DEV`), er darf nie ausgeliefert werden. Eine frühere, offenere Fassung wurde von der Werkzeug-Sicherheitsprüfung blockiert und entfernt, die heutige hat eine eingeschränkte Skript-API.
- Ein Skript läuft in einem eigenen Thread, immer nur eines zugleich. Ausgabe: `rt.out("datei.txt", text)` nach `dev-data\data\out\`, Zeilen im Log über `rt.log` oder `print` (Präfix `explore:`).
- Lua-Sandbox: nur base, string, table und math. Kein `io`, `os`, `require`, `load`, `dofile`. Dateinamen nur als einfache Namen.
- API `rt.*`: `base`, `sections`, `hex`, `u8`, `u16`, `u32`, `u64`, `i32`, `f32`, `f64`, `cstr`, `find` (Code), `findd` (Daten), `bytes` (Text suchen), `xrefs`, `callers`, `func`, `vtable`, `heap` (64-Bit-Wert im Heap), `heapf` (drei Floats in Bereichen), `call` und `callf` (nur Funktionen im Code der Exe, bis zu vier Integer-Argumente, `callf` liefert xmm0), `disasm`, `disfunc`, `sleep` (max. 10 s), `log`, `out`, `run`.
- Skripte: `lib.lua` (Hilfen `BASE`, `hexa`, `strAt`, `stringUsers`, `vtablesWith`), `wave1` bis `wave3` (Anker-Zeichenketten, ClientInstance- und LocalPlayer-vtable), `snapcore` mit `snap_a` bis `snap_d` (Dump des Zustandsblocks, `TAG` pro Situation), `findpos`, `probe` (ruft kurze Getter ohne Speicherzugriffe auf).
- Vorsicht: `call` führt echten Spielcode aus. Nur kurze, schreibfreie Getter aufrufen, SEH-Fehler werden abgefangen, Spielzustand nicht.
- Die Ausgaben enthalten Disassembly von Spielcode. Sie bleiben in `dev-data` und gehören nie ins Repo, auch nicht in Docs. In Docs gehören nur RVAs, Namen und Indizes.

**`tools/testhost`** (kleines D3D11-Fenster, das `Monchi.dll` lädt). Aufruf `testhost.exe <Monchi.dll> [Sekunden]`, Standard 26 s. Es nutzt wie Minecraft das Flip-Modell (`FLIP_DISCARD`). Umgebungsvariablen: `TESTHOST_SIZE=1280x720`, `TESTHOST_MANUAL` (keine Standard-Aktionen), `TESTHOST_OFFSCREEN` (Fenster bei x=-20000), `TESTHOST_PATTERN`, `TESTHOST_SCRIPT` (Schritte `Sekunde:Art:a,b`, getrennt mit `;`, Art `k` Taste mit VK-Code, `t` Zeichen, `c` Klick x,y, `w` Mausrad x,y, `u` Strg+L). In der Cloud baut und startet `tools/cross.sh` (`setup`, `build`, `shots`, `tour`) alles mit MinGW und Wine unter Xvfb und macht Screenshots. Auf echtem Windows kommt nur der erste Frame durch den Present-Hook, und der `shot`-Befehl hängt in manchen Läufen (Ursache offen).

**`MONCHI_SELFTEST=1`** (Umgebungsvariable vor dem Prozessstart, `dll/src/modules/SelfTest.cpp`). Schaltet jedes Modul mit Demo-Daten nacheinander ein (120 Frames), ändert zufällig Einstellungen, simuliert Tasten und Maus und meldet Fehler, Aussetzer über 350 ms (`selftest STALL`), zu langsame Module (über 1 ms) und `untranslated [Modul]: Text`. Es speichert die Config nicht. `MONCHI_SELFTEST_ONLY="Modul A,Modul B"` beschränkt den Lauf. `MONCHI_DUMP_MODULES=<pfad.json>` schreibt die Modulliste, daraus erzeugt `tools/modules_doc.py` die Tabelle in `docs/MODULES.md`.

**`tools/i18n_check.py`**: `python tools/i18n_check.py` listet englische Texte in `dll/src`, für die keine der `Lang*.cpp`-Tabellen und `common/I18n.cpp` einen deutschen Eintrag hat. Es sucht per Regex nach `i18n::tr/fmt`, `Module(...)`, Setting-Labels und einigen Widget-Aufrufen, die Summe steht auf stderr.

**`server/`** (Monchi Online, Cloudflare Worker): Node 20 oder neuer, keine Abhängigkeiten. `npm test` läuft gegen Speicher, D1-Shim, Turso-Attrappe und echte SQL (`node:sqlite`). `npm run dev` startet lokal auf `127.0.0.1:8787`. Nach jeder Änderung an `src/` `node bundle.js` ausführen und `dist/worker.js` mit committen. Der veröffentlichte Worker ist `https://mochi-online.lisawer008.workers.dev`, Veröffentlichen macht Marc (`server/README.md`).

Sonst für die Cloud nützlich: `tools/preview` (rendert die Launcher-UI in ein PNG), `tools/cosmetics/build.py`, `tools/cosmetics_hd`, `tools/pack.py`.

## 4. Was in der Cloud sinnvoll ist und was einen PC-Test braucht

Ohne Minecraft sinnvoll:

- Dokumentation: `docs/STATUS.md`, `docs/HISTORY.md`, `docs/MODULES.md` (über `modules_doc.py`), `docs/PARITY.md`, README im lockeren Ton, Entwurf der Datenschutz-Seite, Release-Checkliste in `docs/LAUNCH.md`. In `docs/TESTLOG.md` nur Einträge mit echtem Beleg vom PC.
- Servercode in `server/`: Tests, Limits, Löschen der Daten, Entwurf für den Besitznachweis eines Gamertags.
- Launcher: Code, Versions-Seite, Updater, Selbst-Update, Oberfläche unter Wine und über `tools/preview`.
- Übersetzungen: `i18n_check.py`, `Lang.cpp`, `Lang_B.cpp`, `launcher/src/LangUi.cpp`, `LangApp.cpp`.
- Code-Review gegen `CLAUDE.md`: Hook-Rückbau beim Abschalten, Crash-Guard um jeden Hook und Thread, Lebensdauer der Hintergrund-Threads (`core/Bg`), MSVC-Strenge, Regel 5 (fehlende Signatur darf nie abstürzen) und Regel 7 (kein stilles Modul).
- Vorarbeit für die Signatur-Infrastruktur, die nur kompiliert und gegen Fake-Daten läuft: Format von `sigs/<version>.json` um vtable-Einträge erweitern (Anker-Zeichenkette plus Index), Entwurf `tools/sigcheck`, Umbau von `Live.cpp` auf den ClientInstance-Weg. Das alles zählt erst als fertig, wenn der PC es im Spiel bestätigt, bis dahin bleiben die Module grau.
- Overlay-Module und UI-Feinschliff mit Screenshots aus Wine (`cross.sh shots`, `tour`), Lua-Beispielskripte, Cosmetics-Generator.
- Die Cloud-Maschine erreichte frühere Male weder die Hive-Seiten noch die Hive-API. Chat-Texte und API-Felder lassen sich dort nicht prüfen.

Zwingend PC mit Minecraft:

- Jede Signatur, jeder Offset, jeder vtable-Index und jeder Hook. Ein Spiel-Modul ist erst fertig mit Logzeile plus sichtbarer Wirkung im Spiel (Regel 7).
- Alles am DX12-Pfad: Fenstergröße, Vollbild, Alt-Tab, Dauerlauf, DX11 erzwingen.
- Eingabe im echten Spiel (Menüsperre, Esc, Toggle Sprint, hängende Tasten) und jede Latenz- und FPS-Messung (Phase 6).
- Launcher: Play, Auto-Inject, Versionswahl mit LeviLauncher, Direktstart einer Exe, Defender.
- Monchi Online Ende zu Ende (der Gamertag kommt aus dem Spiel), Serverkennung auf Hive, Zeqa, CubeCraft, NetherGames, Mineville, echte Chat-Zeilen und API-Antworten.
- Cosmetics und Skin-Vorschau im Spiel (Render-Hook, Skin zur Laufzeit lesen).
- Bau und Test der finalen `MonchiLauncher.exe` und das erste Pre-Release.

## 5. Bekannte Fallen

- **Sandbox-Shell ohne GitHub-Login.** Aus der Shell der PC-Sitzung gehen kein `git push` und kein `gh`. Marc pusht im eigenen Terminal. Absprechen, wer wann pusht und auf welchen Branch, bevor beide Seiten am selben Code arbeiten. Die Shell hat außerdem eine Dateizugriffs-Einschränkung (siehe Fehler 126 oben), und das Speicherabbild der Exe wurde vom Werkzeug-Schutz blockiert. `docs/PLAN_HOME.md` Phase 1 (`MONCHI_DUMP_IMAGE`, Ghidra) ist deshalb offen, bis geklärt ist, wie die Signatursuche laufen darf.
- **Testfenster-Hänger.** `tools/testhost` unter echtem Windows liefert nur einen Frame und verarbeitet nach `shot` manchmal keine Befehle mehr. Hängende `testhost.exe` beenden. Oberflächen-Screenshots gehen verlässlich nur in der Cloud unter Wine.
- **Build-Ordner nie committen.** Im Repo liegen lokal `build`, `build-an`, `build-dev`, `build-launcher`, `build-w4` und `dev-data`, alle sind ignoriert, ebenso `*.dll`, `*.exe`, `*.pdb`. Nie `git add -f`. Im Repo-Root liegt außerdem ein Überbleibsel `True/` (ein alter Aufruf von `inject.ps1`, von `.gitignore` verdeckt), das gelöscht werden darf. Keine Minecraft-Dateien, Speicherabbilder, Ghidra-Projekte oder Explorer-Ausgaben ins Repo.
- **Zeilenenden.** Auf Marcs PC wandelt Git LF in CRLF um (Warnung bei `Explore.cpp`). In der Cloud keine Zeilenenden-Massenänderungen committen.
- **RTSS und Flarial auf Marcs PC.** Die Aufgabe "RTSS Autostart (Minecraft Reflex)" lädt `RTSSHooks64.dll` als Watcher in Minecraft, und Flarial ist dort laut Marc ebenfalls vorhanden. Beide hooken dieselben DXGI-Funktionen. Bisher hat Monchi trotz RTSS sauber gehookt. Bei merkwürdigen Hook-Fehlern und vor jeder Messung prüfen, welche Module im Prozess stehen, und Flarial nie gleichzeitig mit Monchi laden.
- **Monchi vor 60 s nach dem Minecraft-Start injizieren.** Vorgabe aus dem Übergabeauftrag. Der Grund steht nirgends im Repo, im Zweifel Marc fragen. `inject.ps1 -Launch` injiziert normalerweise kurz nach dem ersten Fenster plus `d3d12.dll`.
- **DLL entladen vor dem nächsten Test.** Strg+L, sonst meldet `inject.ps1` `already injected` und die Datei bleibt gesperrt. Die Explorer-Skripte ändern nie den Spielzustand absichtlich, `call` auf unbekannte Funktionen kann ihn trotzdem ändern.
- **Hive und Zeqa.** Die Regeln für beide, NetherGames und Mineville stehen in `servers/servers.json` als Annahmen (`warn`) und müssen geprüft werden.

## 6. Nächste Schritte in der Reihenfolge von `docs/PLAN_HOME.md`

Regel aus dem Plan: Es geht nicht zu Phase 6 oder 7, solange Module grau sind. Nach jeder Welle die Zahl vor und nach in `docs/STATUS.md` eintragen (`locked:` im Log).

1. Marc: lokale Änderungen committen und `claude/pc-test` pushen, damit die Cloud den Stand sieht (nicht von dieser Sitzung aus möglich).
2. Phase 0 abschließen (PC): die ungeprüften Punkte aus Abschnitt 1, Zeile 4 und 5, im Spiel bestätigen, DX11 und DX12, 30-Minuten-Lauf, Einzel-Exe und Versionswahl. Latenz und Overhead messen, bevor Module Hooks setzen. Cloud: Review der Eingabe- und DX12-Pfade, Texte und Tests.
3. Phase 1, Signatur-Werkzeuge: Entscheidung, wie die Suche läuft (Dev-Explorer statt Speicherabbild), `tools/sigcheck` mit ehrlicher Ausgabe (OK, 0 Treffer, mehrere Treffer), Format von `sigs/<version>.json` um vtable-Einträge erweitern. Entwurf in der Cloud, Prüfung am PC.
4. Phase 2, Welle 1 (PC): Spielerdaten aus dem Zustandsblock. Offsets für Position, Geschwindigkeit, Yaw, Pitch, Leben, Hunger, Dimension, Boden, Sprint, Schleichen und `Level` aus `snap_a` bis `snap_d` ableiten, gegen F3 und das Debug-Menü prüfen, `sigs/1.26.52.3.json` schreiben und `Live.cpp` anpassen. Schaltet rund 53 Module frei.
5. Welle 2, Kampfgefühl: `AttackEntity`, `ActorEvent`, `KillEvents`, `Target`, `UseState`, `Inventory`, `Effects`. Danach Combo, Reach (nur Anzeige), Hit Counter, Hit Ping, Target HUD, Armor, Potion und Item-Zähler gegen echte Treffer abgleichen.
6. Welle 3, sichtbare Effekte (`fx.*`): zuerst `fx.fov`, `fx.gamma`, `fx.viewBob`, `fx.hurtCam`, `fx.hideHand`, `fx.sensitivity`, `fx.handMatrix`, `fx.lookCamera`, `fx.lookTurn`, dann Zeit, Wetter, Nebel, Farben, Render Options. Bei jedem Kanal prüfen, ob der Hook beim Abschalten sauber zurückgenommen wird.
7. Welle 4, Listen und Rest: `TabListData`, `ScoreboardData`, `ChatEvents`, `PackList`, `EntityList`, `ActorList`, `ProjectileList`, `SoundEvents`, `LightLevels`, Crystal-Kanäle und die Hitbox über den Zeichenweg des Spiels (nie als eigene Linien über das Bild).
8. Nach jeder Welle: `MONCHI_SELFTEST=1` sauber, `modules_doc.py` neu, graue Module mit Grund in `docs/STATUS.md`, kleiner Commit mit der `sigs`-Datei. Jedes graue Modul wird repariert, anders gelöst oder entfernt.
9. Phase 3, Server und Regeln: Erkennung, `servers.json`, Chat-Wortlisten, Hive-API-Felder, sicherstellen, dass kein Modul Pakete schickt, die der normale Client nicht schickt.
10. Phase 4, Monchi Online live, dann Phase 5, Cosmetics im Spiel (nie als Overlay, das wäre ein Wallhack).
11. Phase 6, messen und mit Flarial und Onix vergleichen, Ergebnis als Tabelle mit echten Zahlen in `docs/PARITY.md`.
12. Phase 7, Versionen und Release: mindestens drei Minecraft-Versionen mit `inherits`, Windows 10 und 11, Release-Pipeline mit `v0.1.0-alpha.1`, Umbenennen von "Monchi", Datenschutz-Seite. Die Release-Exe wird erst nach allen Tests gebaut.
