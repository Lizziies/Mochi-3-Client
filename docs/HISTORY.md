# Verlauf

Kurze Chronik, damit jede neue Session weiß, was schon passiert ist.

## Session 1 (Hauptaccount, Claude.ai Chat)

- Flarial und Onix recherchiert, Plan und Docs geschrieben.
- DLL-Kern, Renderer (DX11/DX12), ClickGUI, HUD-Editor, Themes, Input-Grundlagen, Server-Regeln und 20 Overlay-Module gebaut. Getestet nur unter Wine mit dem Testhost.
- Danach sollten 8 parallele Helfer die restlichen ca. 115 Module schreiben (Kampf-Info, Welt-Info, Kamera, Welt-Visuals, Oberfläche und Chat, Steuerung und Server, Overlay-Tools und Spiele, Post-Effekte) und das Onix-Layout einbauen. Die Session lief ins Limit, deren Ergebnisse sind nicht in der ZIP.
- Ergebnis: `mochi-client.zip`, Stand ca. 15 %.

## Session 2 (zweiter Account, Claude Code Cloud)

- ZIP in dieses Repo importiert, `docs/MASTER.md` und diese Datei angelegt.
- Fehlende Module werden hier in Blöcken nachgebaut (siehe `docs/STATUS.md`).
- `docs/CLIENTS.md` (Wettbewerber) und `docs/FEATURES.md` (Feature-Spezifikation) geschrieben. Einige Seiten (flarial.xyz, onixclient.com, latite.net) waren vom Netzwerk gesperrt, Angaben sind in CLIENTS.md mit ✔/~/? gekennzeichnet.
- Um 11 Uhr bekommt diese Session die Zusammenfassung des Chatverlaufs vom Hauptaccount, damit mehrere Sessions parallel arbeiten können.

## Session B (Module, Branch claude/modules-b)

- Block 1: Netzwerk-Messung (ICMP und RakNet-Ping, WLAN-Daten über wlanapi/iphlpapi, nur Lesen), Frame Limiter, Instant Input, Latency Blame, Tuning-Verteiler.
- Block 2: Post-Effekt-Pass (HLSL, ein Shader für alle Bildfilter), Screenshot-Capture, Crosshair-Editor, Block Game, Pomodoro. MinGW und glslang liegen auf der Cloud-Maschine (apt), damit der Code vor dem Commit syntaxgeprüft werden kann.
- Block 3 bis 5: SDK (`dll/src/sdk`) mit Demo-Daten, Live-Leser, Effekt-Kanälen und Tasten-Injection; danach alle weiteren Module bis 135. `docs/SDK.md` und der Abschnitt "Umsetzungsstand" in `docs/MODULES.md` entstehen teils aus dem Quellcode (Skript lag nur im Scratchpad, die Tabellen sind von Hand pflegbar).
- Phase 0 (Teil 1): Module-Texte auf Englisch umgestellt, deutsche Tabelle in `Lang_B.cpp`, `Module::ruleNote()` übersetzt den Standardhinweis zur Laufzeit.
- Phase 0 (Teil 2): Counter-Module angeglichen (Combo, Reach, Hit Ping, Opponent Reach, Potion HUD, Arrow/Totem/Pot Counter, Insta Hurt Animation). `HudModule` kann jetzt nach oben wachsen (`growsUp`), `Module::wants` leiht Daten, ohne das Modul bei fehlenden Daten grau zu machen.
- Phase 1 (Teil 1): Hive Utils und Zeqa Utils, gemeinsame Chat-Hilfen in `modules/server/ServerChat.hpp`, Demo-Server mit Skript-Chat, `tools/testhost` kann per `TESTHOST_SCRIPT` Tasten und Klicks ausführen.
- Phase 1 (Teil 2): Hive Stats, Hive Leaderboard (`modules/server/HiveApi.*`), Auto-GG-Endwörter pro Server, NetherGames und Mineville in `servers.json`, Pivot für HUD-Module.
- Phase 2: Crystal Optimizer mit Geisterliste, Kill Cleanup, Crit-Partikel im Particle Multiplier, `Hit`-Ereignisse tragen Actor-Zeiger und Crystal-Flag.
- Phase 3 (Teil 1): Hitbox-Kanäle und -Einstellungen, gemeinsame HUD-Optik mit Blur-Pass, Keystrokes und Coordinates in die Tiefe, Platzhalter.
