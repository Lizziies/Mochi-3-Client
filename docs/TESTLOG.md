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

- Start im echten Spiel: `LoadLibrary` in `Minecraft.Windows.exe` schlug zuerst mit Fehler 126 fehl. Ursache war der Testaufbau, nicht der Client: meine Shell läuft in einer Sandbox, die `%LOCALAPPDATA%` umleitet, und Minecraft sah die DLL dort nicht. Lösung: `tools/inject.ps1` kopiert die DLL nach `dev-data\bin` im Repo (gitignored) und legt dort `Mochi.root` ab. Der Client nimmt dann `dev-data\data` als Datenordner (`core/Paths.cpp`), so lässt sich auch das Log lesen. Der Injektor meldet jetzt den echten Fehlercode von `LoadLibraryW`.
- Ergebnis: Client lädt, `renderer: dx12 (11on12)`, `ui ready`, `ready`, Hooks auf Present, ResizeBuffers, Present1, ExecuteCommandLists, sendto, WSASendTo, getaddrinfo, ClipCursor, SetCursorPos. Version im Log: `minecraft 1.26.52.3`. 179 Module registriert, 107 gesperrt (`locked:`), `sigs: 0/0`. Screenshot: HUD (FPS, CPS, Keystrokes) und Hub-Menü mit Blur nach Rechts-Shift erscheinen im Spiel.
- Beobachtung: RTSS (`RTSSHooks64.dll`) war nach dem Start ebenfalls im Prozess (Watcher), Mochi hat trotzdem sauber gehookt.
