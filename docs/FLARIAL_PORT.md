# Flarial-Modulbasis, 2026-10-03

Auf Wunsch des Nutzers liegen alle 117 Flarial-Modulordner unter `vendor/flarial/src/Client/Module/Modules`.
Events, Hooks, SDK und Signaturdefinitionen wurden ebenfalls übernommen. Original-Lizenz (AGPL-3.0)
und der feste Upstream-Commit stehen in `vendor/flarial`. Mochis bestehende UI, Launcher und
zusätzliche Module bleiben erhalten; das Worktree `Desktop/mochi-ui` wurde nicht verändert.

Die importierten Dateien sind die Quellbasis, kein bereits komplett integrierter zweiter Client.
Flarial verwendet ein anderes SDK, eigene Events, Rendering und Konfiguration. Jeder Port muss
an Mochis Schnittstellen angeschlossen werden. Ein vorhandener Menüschalter beweist keine Spielwirkung.

## Erster angeschlossener Bereich

| Modul | Bisher | Neue Anbindung | Prüfung |
|---|---|---|---|
| Freelook | Versionsbit in der entt-Registry verändert; Wirkung unbestätigt | Flarials nativer Kamera-/Spieler-Update-Hook mit drei Argumenten und Body-/Head-Yaw-Patches, über Mochis Einstellungen gesteuert; Kamera-Winkel vor Rückgabe wiederhergestellt | Lokale Hook-/Patch-Lifecycle-Tests bestanden; Signaturen und sichtbare Wirkung im Spiel noch offen |
| Third Person Nametag | Eigenes Poppins-Overlay, Formatierung entfernt | Flarials native Self-Tag-Sperre; keine Text-/Farbersetzung. Original-Overlay bleibt optional | Lokale Patch-Lifecycle-Tests bestanden; Signatur und sichtbare Wirkung im Spiel noch offen |

Die beiden Module benötigen echte Signaturen (`CameraUpdatePlayer`, `CameraYaw`, `CameraHeadYaw`, `OwnNametagGate`).
Die alten Pseudo-Freigaben ersetzen diese Anbindung nicht. Fehlende Signaturen sperren das Modul.
Die eingefügten Muster stammen aus Flarials Signaturdefinitionen; sie sind noch keine bestätigten
Mochi-Signaturen für 1.26.52.3.
Der 1.26-Head-Yaw-Store ist fünf Bytes lang; der Port ersetzt und restauriert die vollständige
Instruktion, nicht nur vier Bytes. Bei einem unpassenden zweiten Patch wird der erste zurückgenommen.

## Bestand und weitere Ports

Der vorherige Stand registriert 153 Module. Am 3. Oktober ergab ein Runtime-Audit in einer Welt
mit deaktivierten Demo-Daten 153 verfügbare Schalter. Das ist eine Verfügbarkeitsprüfung, kein
Funktionsnachweis. Der ältere Commit hatte 26 nicht angebundene Module entfernt; „0 grau“
beschreibt deshalb nicht den vollständigen gewünschten Funktionsumfang.

`tools/compare_modules.py` vergleicht alle registrierten Klassen mit der Flarial-Quellbasis und
erfasst native Effekt-Anfragen, Game-Aufrufe, Events, Signaturen und Optionsnamen. Nicht direkt
zugeordnete Ordner sind keine bestätigten fehlenden Funktionen: Gruppierung und Namen unterscheiden sich.

Die entfernten Render-Funktionen müssen wieder aufgenommen und einzeln angebunden werden:
Nametag Modifier, Item Physics, Hand-/Swing-Animationen, Block Hit, Left Hand, Smooth Sneak,
Minimal View Bobbing, Time/Weather/Environment Changer, Fog/Water/Hurt/Glint Color, Low Fire,
Particle Multiplier, GUI Scale, Movable Hotbar/Title/Bossbar, Light Overlay, Subtitles,
Skin Stealer und die serverabhängigen Timing-/Hurt-Funktionen. Bestehende Server-Regeln gelten weiter.

Nicht jeden Flarial-Ordner ungeprüft aktivieren: `RawInputBuffer` ist im heruntergeladenen
Stand ein Platzhalter; `NoHurtCam` nutzt teilweise Signaturen, die für 1.26 bereits als veraltet
markiert sind. Quellcode kopiert bedeutet nicht, dass jeder Hook auf dieser Version funktioniert.

## Weitere bereits korrigierte Stellen

- HUD-Blur nimmt seinen Hintergrund vor Mochis HUD und Menü auf; die Blur-Fläche folgt der HUD-Ausrichtung.
- Text-HUDs behalten Bedrock-Farben, Fett- und Kursivformatierung. System-Symbole ergänzen die HUD-Schrift;
  Resource-Pack-Zeichen brauchen weiterhin den nativen Minecraft-Renderer.
- Heap-Suche läuft mit niedrigerer Priorität, kurzen Pausen und weniger Wiederholungen.
- Frame-Limiter wartet nur begrenzt auf seinen Timer.
- Signaturen werden im DLL-Build eingebettet; ohne passende Einträge bleiben Funktionen gesperrt.
- Unbekannte Spielversionen verwenden keine älteren oder neueren Offsets als Ersatz.

Mochi wurde aus dem Spiel entladen. Die abschließende Modulliste enthielt nur Flarial.
Seit der Untersagung wurde kein neuer Mochi-Build injiziert. Weitere Sichttests werden erst
in einer später ausdrücklich freigegebenen Mochi-Testsession durchgeführt.
