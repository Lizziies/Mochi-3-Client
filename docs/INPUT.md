# Input-Latenz

Ziel: Zeit zwischen Klick/Tastendruck und sichtbarem Frame minimieren. Ping zum Server lässt sich vom Client aus nicht ändern, alles hier betrifft nur die lokale Kette Maus → Spiel → GPU → Bildschirm.

Regel: Jede Maßnahme wird mit dem Latenz-Overlay vorher/nachher gemessen. Was nichts bringt, fliegt raus.

## 1. Messen zuerst

Latenz-Overlay (Modul, Phase 5 zuerst bauen):
- Zeitstempel beim Raw-Input-Event (QPC), Zeitstempel beim nächsten `Present`, Differenz = Input→Present.
- Frametime-Graph, 1%-Low, Anzahl Frames in der Queue.
- Optional: Flash-Test. Bei Klick ein weißes Quadrat zeichnen, mit Handy-Slowmo oder Messgerät nachmessen.

## 2. Frame-Queue verkürzen (größter Hebel)

- Im Present-Hook die Swapchain abfragen: `IDXGISwapChain2::SetMaximumFrameLatency(1)` falls Waitable-Swapchain, sonst `IDXGIDevice1::SetMaximumFrameLatency(1)`.
- Prüfen, ob das Spiel eine Waitable-Swapchain nutzt. Wenn ja, vor dem Input-Poll auf das Waitable-Object warten (Reflex-artig: erst warten, dann Input lesen, dann rendern).
- Option "Tearing erlauben": `SyncInterval 0` + `DXGI_PRESENT_ALLOW_TEARING`, falls die Swapchain mit dem Flag erstellt wurde. Sonst Hook auf Swapchain-Erstellung (`CreateSwapChainForCoreWindow/ForHwnd`), Flag hinzufügen. Nur aktivieren, wenn Ingame-VSync aus ist.

## 3. Präziser Frame-Limiter statt VSync/Cap

- Spiel-FPS-Cap umgehen (Option), dann eigener Limiter im Present-Hook.
- Limiter wartet **vor** dem Input-Sampling, nicht nach dem Present. So ist der Input bei der Darstellung frischer.
- Wartezeit: `CreateWaitableTimerExW` mit `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` plus kurzes Spin am Ende.

## 4. Raw Input sauber verarbeiten

- Bedrock liest Maus-Input pro Tick/Frame. Mehrere Pakete zwischen zwei Frames können zusammengefasst werden oder verloren gehen. Bei Flarial ist der "Raw Input Buffer" im öffentlichen Code nur ein Platzhalter.
- Eigener Raw-Input-Pfad: `WM_INPUT` im WndProc-Hook abgreifen, alle Pakete per `GetRawInputBuffer` einsammeln, Klicks mit QPC-Zeitstempel in eine Queue legen und in `MouseDevice::feed` / `InputHandler::tick` in richtiger Reihenfolge einspeisen. Keine Klicks erfinden, nur keine verlieren.
- Dieselben Daten treiben CPS und Keystrokes, damit die Anzeigen exakt sind.

## 5. System-Tweaks (nur solange das Spiel läuft, beim Entladen zurücksetzen)

- `timeBeginPeriod(1)`.
- Prozesspriorität "Über normal" (nicht "Echtzeit").
- Power-Throttling für den Prozess aus: `SetProcessInformation(ProcessPowerThrottling)`.
- Render-Thread-Priorität leicht anheben.
- Hinweis im Launcher, falls Windows-Game-Mode aus ist oder HAGS deaktiviert ist (nur Hinweis, nichts selbst umstellen).

## 6. Weniger rendern = mehr FPS

Render Options: Partikel, Himmel, Wolken, Block-Entities, Entity-Schatten, Wetter einzeln abschaltbar. Preset "PvP Max FPS".

## 7. Eigenes Overlay billig halten

- ImGui-Draw nur, wenn sich etwas ändert, sonst gecachte Vertex-Buffer.
- Blur im Menü nur bei offenem Menü, nie im HUD.
- Ziel: < 0,3 ms Overlay-Kosten pro Frame bei offenem HUD, im Latenz-Overlay anzeigen.

## Ehrliche Erwartung

Auf einem starken PC bringt das wenige Millisekunden, auf schwachen PCs und bei VSync deutlich mehr. Werbung damit nur mit gemessenen Zahlen.
