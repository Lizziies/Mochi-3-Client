# Testlog

Jeder Eintrag: Datum, Minecraft-Version, was getestet wurde, Ergebnis, Beleg.

## Neu installierte Werkzeuge (PC von Felix)

| Werkzeug | Weg | Stand |
|---|---|---|
| Git 2.55.0 | winget, `--scope user`, still | installiert, kein Autostart, kein Dienst |
| Python 3.12.10 | winget, `--scope user`, still | installiert, kein Autostart, kein Dienst |

Schon vorhanden und nur benutzt: CMake 4.4.3, Visual Studio 2022 Build Tools 17.14 (MSVC 14.44, Windows SDK 10.0.26100). Gebaut wird nur über `cmake` auf der Kommandozeile, die VS-Oberfläche wird nicht gestartet. Nach jedem Build werden `vctip`, `mspdbsrv` und `MSBuild` beendet.

Autostart-Ausgangslage vor der Arbeit (nicht von uns): OneDrive, LGHUB, RazerAppEngine, EpicGamesLauncher, Edge in `HKCU\...\Run`, SecurityHealth und RtkAudUService in `HKLM\...\Run`, Aufgabe "RTSS Autostart (Minecraft Reflex)". Git hat nichts hinzugefügt.

## 2026-10-01, Minecraft 1.26.5203.0 (GDK), Phase 0

- Build der DLL mit MSVC 19.44 (`cmake -S dll -B build -G "Visual Studio 17 2022" -A x64`, Release). Der Code war bisher nur mit MinGW gebaut. Drei Fehler behoben: `small` ist in `windows.h` ein Makro (Variable umbenannt), `<cwctype>` fehlte in `Music.cpp`, `/bigobj` für `Manager.cpp`. Ergebnis: `Mochi.dll`, 4,3 MB, 0 Fehler. Beleg: Commit `fix msvc build: small macro clash, towlower include, bigobj`.

- Start im echten Spiel: `LoadLibrary` in `Minecraft.Windows.exe` schlug zuerst mit Fehler 126 fehl. Der Fehler trat nur beim Laden aus `%LOCALAPPDATA%\Mochi\bin\Mochi.dll` auf, das ich aus meiner Test-Shell (mit Dateizugriffs-Einschränkung) kopiert hatte. Die Ursache ist nicht belegt (Verdacht: Dateirechte der dort angelegten Datei). Laden aus dem Repo-Ordner funktioniert. Lösung für den Test: `tools/inject.ps1` kopiert die DLL nach `dev-data\bin` im Repo (gitignored) und legt dort `Mochi.root` ab. Der Client nimmt dann `dev-data\data` als Datenordner (`core/Paths.cpp`), so lässt sich auch das Log lesen. Der Injektor meldet jetzt den echten Fehlercode von `LoadLibraryW`.
- Ergebnis: Client lädt, `renderer: dx12 (11on12)`, `ui ready`, `ready`, Hooks auf Present, ResizeBuffers, Present1, ExecuteCommandLists, sendto, WSASendTo, getaddrinfo, ClipCursor, SetCursorPos. Version im Log: `minecraft 1.26.52.3`. 179 Module registriert, 107 gesperrt (`locked:`), `sigs: 0/0`. Screenshot: HUD (FPS, CPS, Keystrokes) und Hub-Menü mit Blur nach Rechts-Shift erscheinen im Spiel.
- Beobachtung: RTSS (`RTSSHooks64.dll`) war nach dem Start ebenfalls im Prozess (Watcher), Mochi hat trotzdem sauber gehookt.
- Launcher als einzelne Exe (`-DMOCHI_DLL`, `-DMOCHI_COSMETICS`, MSVC): `MochiLauncher.exe`, 5,9 MB, 0 Fehler. `MochiLauncher.exe --extract` aus einem leeren Ordner: Exit 0, danach liegen `%LOCALAPPDATA%\Mochi\bin\Mochi.dll` (4,3 MB) und der Ordner `cosmetics` mit `index.json` da. Play, Spielstart und Versionswahl noch nicht geprüft.

## 2026-10-01, Minecraft 1.26.52.3, Fehler aus dem ersten Spieltest (Felix)

| Nr | Fehler | Stand |
|---|---|---|
| 1 | FPS, CPS und alle Module wurden auch in Menüs (Hauptmenü, Welten-Liste) gezeichnet, sie sollen nur im Spiel erscheinen | Code behoben (`Manager::frame` zeichnet nur bei `inWorld` oder im HUD-Editor, `inWorld` ohne Signatur aus Cursor-Zustand und Netzwerk-Rate), im Spiel noch nicht geprüft |
| 2 | Bei offenem Mochi-Menü reagiert das Spiel dahinter auf Maus und Tasten | Code behoben (zusätzlich zu WndProc jetzt `GetCursorPos`, `GetAsyncKeyState`, `GetKeyState`, `GetRawInputData`, `GetRawInputBuffer`, `PeekMessageW`, `GetMessageW`), Log-Zeile `menu open, game polled: ...` zeigt, welchen Weg das Spiel nutzt, im Spiel noch nicht geprüft |
| 3 | Der Toast "Mochi loaded" erscheint oben rechts, soll oben links | Code behoben, im Spiel noch nicht geprüft |

Offen aus der Sicherheitsprüfung: Das Speicherabbild von `Minecraft.Windows.exe` (Phase 1, Ghidra) wurde vom Werkzeug-Schutz blockiert. Das Skript liegt nicht im Repo, bis klar ist, wie die Signatursuche laufen darf.

Geprüft im Spiel nach dem Neustart (Build mit Eingabesperre): HUD (FPS, CPS, Keystrokes) ist im Hauptmenü weg, auch bei offenem Mochi-Menü (Screenshot). Strg+L entlädt sauber (`unloading`, `bye`, Modul weg, Spiel läuft weiter). Das Spiel fragt im Menü per `GetCursorPos` (hunderte Aufrufe pro Sekunde), `GetKeyState` und `PeekMessageW` ab, nicht über Raw Input (`RawData=0`, `RawBuffer=0`).

| Nr | Fehler | Stand |
|---|---|---|
| 4 | Esc schließt das Mochi-Menü und öffnet gleichzeitig Minecrafts Dialog "Möchtest du Minecraft verlassen?" (das Loslassen der Taste gelangte ins Spiel) | Code behoben (jede verschluckte Taste und Maustaste wird samt Loslassen verschluckt), im Spiel noch nicht geprüft |
| 5 | Im Hauptmenü ließ sich das Mochi-Menü mit Rechts-Shift öffnen, Module waren bedienbar | Code behoben (`Manager::dispatchKey` und `dispatchMouse` arbeiten nur im Spiel, bei offenem Menü oder im HUD-Editor), im Spiel noch nicht geprüft |

## 2026-10-01 abends, Phase 1 begonnen (Signatur-Werkzeuge)

- Gefunden (rein aus Zeichenketten und Laufzeitzustand, ohne Datei-Kopie der Exe): Die Minecraft-Exe enthält keine RTTI für Spielklassen (nur 14.699 Namen für Standardbibliothek-Typen und Lambdas), aber sehr viele Zeichenketten mit vollständigen Funktionssignaturen, zum Beispiel `virtual void __cdecl Player::tickWorld(const Tick &)`, `MinecraftGame::update`, `GameRenderer::renderCurrentFrame(float)`, `ClientInstance::requestLeaveGame`. Daraus lassen sich Funktionen, vtables und Objekte ableiten. Die Entity-Daten liegen in einem ECS (`LocalPlayerComponent`, `entt::...`), Feldoffsets am Player-Objekt sind deshalb nicht direkt zu erwarten, getter-Aufrufe über die vtable sind der wahrscheinlichere Weg.
- Ein Lua-Explorer im Client (Funktionssuche, Disassembler, Aufruf von Spielfunktionen, gesteuert über `dev.cmd`) wurde von der Werkzeug-Sicherheitsprüfung als mögliche Angriffsfläche blockiert und wieder entfernt (Commit `9a84faa` enthält den Stand). Die Signatursuche ist damit offen, bis geklärt ist, wie sie laufen darf.
- Weitere Fehler gefunden und behoben: DX12-Absturz beim Vollbild-Wechsel (F11) und beim Wechsel Vollbild/Fenster (`abort()` in Minecraft, weil 11on12 noch Referenzen auf Back-Buffer hielt, jetzt pro Frame gewrappt und freigegeben), Menü verschluckt `WM_POINTER*`, verschluckte Tasten werden samt Loslassen verschluckt (Esc öffnete Minecrafts Dialog), Server-Erkennung nur nach dauerhaftem Netzwerkverkehr, "im Spiel"-Erkennung berücksichtigt den Fokus, Menü schließt mit Rechts-Shift, Cosmetics-Leiste bricht um, "HUD bearbeiten" passt sich dem Text an.

## 2026-10-01 abends, Signatursuche mit dem Dev-Explorer (nur Dev-Build `-DMOCHI_DEV=ON`, nie im Release)

Belegt im echten Spiel (1.26.52.3), per Disassembly und Heap-Suche:

- `ClientInstance` hat die vtable bei RVA `0e9731b0` (gefunden über die Zeichenkette von `ClientInstance::requestLeaveGame`, Index 15). Vier Instanzen liegen im Heap.
- `ClientInstance::getLocalPlayer` ist vtable-Index `0x540/8` (Funktion RVA `05dc2630`). Aufgerufen mit einer der Instanzen liefert sie einen gültigen `LocalPlayer` (vtable RVA `0e7adab0`, 419 Einträge).
- Die Spieler-Zustandsdaten (`Actor`-State) hängen hinter `LocalPlayer` vtable-Index `0x560/8`; viele Getter lesen daraus (zum Beispiel Offsets `0x368`, `0x3b8`, `0x4ab` bis `0x4c4`, `0x37c`, `0x3e0`).
- Position: Der Heap-Treffer für die angezeigte Position (3, 109, 57) liegt bei `c40aff490`, aber die zwei gefundenen Treffer sind nicht eindeutig zuordenbar (Float-Muster ohne Gegenprobe). Offen.

Nicht erreicht: Es gibt noch keine einzige übernehmbare Signatur in `sigs/`. `locked:` bleibt bei 105 Modulen.

Weitere Fehler und Änderungen heute: Cursor-Freigabe bei offenem Menü (`ClipCursor` wird zurückgenommen), Raw-Input-Klicks gehen ans Menü, gehaltene Tasten werden beim Öffnen losgelassen, der erste Klick bei offenem Menü kommt jetzt vom Raw-Input statt von `WM_*`-Nachrichten.

Offen und nicht geprüft: ob das Menü im Spiel jetzt mit der Maus bedienbar ist (der letzte Test lief nur über Entwickler-Befehle), Rechtsklick links/rechts im Menü, Einstellungen der Extras-Module live, Einfrieren des Spiels im Hintergrund beim Menüwechsel.

## 2026-10-01 spät, ohne Minecraft (Marc spielt)

- Menü öffnet sich jetzt mit sofortiger Eingabesperre: `ui::capturing()` liest den Zustand live (vorher erst im nächsten Frame), beim Öffnen werden gehaltene Tasten und Maustasten sofort losgelassen (`input::releaseHeld` in `gui::setOpen`). Das war der Rest der Spielbewegung beim Öffnen. Im Spiel noch nicht bestätigt, Marc testet.
- Mochi Online: `GET /v1/health` des eigenen Cloudflare-Workers antwortet `{"ok":true,"online":0}`. Der Dienst ist da, die Anbindung im Spiel ist ungeprüft (braucht den Gamertag aus dem Spiel).
- Skin-Vorschau mit dem echten Skin: Es gibt keine Skin-Datei auf der Platte (`custom_skins` ist leer, der Skin kommt aus dem Xbox-Profil beziehungsweise dem Spiel). Die Vorschau muss das Skin-Bild zur Laufzeit aus dem Spiel lesen, hängt also an den Signaturen.
- Testwerkzeug `tools/testhost` mit `TESTHOST_OFFSCREEN`: das Testfenster bekommt auf echtem Windows nur den ersten Frame durch den Present-Hook (danach ruft der Host den Hook nicht mehr auf, Ursache nicht gefunden, in Minecraft tritt es nicht auf). Deshalb keine Oberflächen-Screenshots ohne Minecraft.

## 2026-10-01 spät, Durchsicht ohne Minecraft

- MSVC /W4: nur 7 Warnungen im eigenen Code (Kleinkram). `/analyze`: 14 Meldungen, behoben: URL ohne Host im Online-Modul, Null-Prüfungen bei Zwischenablage, Schrift-Ressource, Fence-Event im DX12-Pfad, `WSAStartup`-Rückgabe.
- Echter Fehler gefunden und behoben: losgelöste Threads (Music, Mice, Regel- und Signatur-Download) konnten beim Entladen (Strg+L) noch laufen, während die DLL freigegeben wurde. Neu: `core/Bg.*` verwaltet sie, `bg::drain` wartet beim Entladen. Alle Thread-Einstiegspunkte laufen jetzt im Crash-Guard, eine Ausnahme (zum Beispiel fehlerhafte `servers.json`) beendet das Spiel nicht mehr.
- Konfiguration gehärtet: kaputte oder falsch typisierte Dateien brechen den Start nicht mehr ab, Profilnamen werden bereinigt (kein `..`).
- `server/bundle.js` konnte bei Windows-Zeilenenden ein kaputtes Bundle erzeugen, behoben (Ergebnis identisch zum committeten `dist/worker.js`). Server-Tests: 17 von 17 grün auf Speicher, D1 und Turso-Attrappe.
- Übersetzungen: `tools/i18n_check.py` prüft englische Texte gegen die deutschen Tabellen. 5 fehlende Einträge ergänzt, der Selbsttest meldet 0 unübersetzte Texte.
- Selbsttest (`MOCHI_SELFTEST=1`, im Testfenster außerhalb des Bildschirms, ohne Minecraft): 177 Module mit Demo-Daten getestet, 0 Fehler, 0 Aussetzer, 0 zu langsam.
- Testfenster `tools/testhost`: lief nur einen Frame, weil es das alte Swapchain-Modell nutzte, jetzt Flip-Modell wie Minecraft. Der Screenshot-Befehl `shot` (neue Stufe `Final`) ist eingebaut, aber nicht fertig geprüft: in einigen Läufen verarbeitet das Testfenster danach keine Befehle mehr, Ursache offen.
- Lua-Sandbox geprüft (kein `io`, `debug`, kein `os.execute`), Eingaben des Clients sind nur Tastatur (kein Klick-Automat).

## 2026-10-02 abends, PC, Stand `claude/onix-ui-input-fixes` (fb196a4) übernommen

- HOME_TODO 0: lokaler `claude/pc-test` war vollständig im Cloud-Branch enthalten, per Fast-Forward übernommen.
- HOME_TODO 1, Build: MSVC baut DLL, Dev-DLL und Launcher. Zwei MSVC-Fehler behoben: `boyer_moore_horspool_searcher` mit `uint8_t`-Zeigern über einen `std::string` (`sig/Image.cpp`), verlustbehaftete wchar-Umwandlung in `core/Guard.cpp`.
- Selbsttest im Testfenster (außerhalb des Bildschirms, ohne Minecraft): 177 Module, 0 Fehler, 0 Aussetzer, 0 zu langsam, 0 unübersetzte Texte. Die Prüfung des Bild-Auflösers wird außerhalb von Minecraft jetzt übersprungen (sie sucht Spielcode). Ausgangszahlen: `usable` 75, `locked` 104.
- Gefundener Fehler mit RTSS (bei Felix über den Watcher immer aktiv, bei vielen Spielern MSI Afterburner): RTSS setzt die ersten Bytes von dxgi `Present` immer wieder auf das Original zurück (von außen mitgelesen, der Sprung wechselt mit den Originalbytes `48 89 5C 24 10`). Unser Inline-Hook fiel dadurch weg, im Testfenster kamen keine oder nur ein Bild an. Behoben: Present, Present1 und ResizeBuffers werden zusätzlich in der Swapchain-vtable umgebogen (`hook/Dx.cpp`, beim Entladen zurückgesetzt). Testfenster 3 von 3 Läufen mit RTSS ok.
- HOME_TODO 2, Server: 19 von 19 Tests auf Speicher, D1 und Turso-Attrappe. `dist/worker.js` ist aktuell. Veröffentlichen bei Cloudflare und `ADMIN_KEY` braucht Felix' Konto, offen.
- HOME_TODO 3, Rauchtest im echten Minecraft 1.26.52.3, DX12: lädt (`frames arrive through the swapchain table`, `dx12 (11on12)`), GameInput-Hooks gesetzt (12 Hooks), neues Menü sichtbar, Kamera bleibt bei offenem Menü still und springt beim Schließen nicht, RTSS kommt nach 60 s dazu und das HUD läuft weiter, F11 zweimal ohne Absturz, Strg+L entlädt (`bye`), erneutes Injizieren mit RTSS schon im Prozess klappt. Log ohne `[error]` und `[warn]`. Screenshots in der Sitzung geprüft.
