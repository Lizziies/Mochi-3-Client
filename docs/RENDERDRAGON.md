# RenderDragon, Chunks, Shader und Eingabe-Latenz

Ideen von Felix und was davon geht. Ehrlich getrennt in "gebaut", "geht ohne Spiel-Signaturen" und "braucht Reverse Engineering am PC".

## Shader auf Servern (gebaut)

Modul **Shader Packs** (Visual). Es legt einen eigenen Pixel-Shader über das fertige Bild, wie ein Overlay. Das ist reine Anzeige auf deinem Bildschirm, deshalb funktioniert es auf jedem Server, braucht keinen Raytracing-Pfad und keine Signaturen.

- Vier eingebaute Looks: Soft Glow, Vibrant, Cinematic, Crisp.
- Eigene Shader: `.hlsl`-Dateien in `%LOCALAPPDATA%\Mochi\shaders\`. Eine Funktion `float4 main_image(float2 uv)`, mit `tex(uv)`, `iRes`, `iTime`. `README.txt` und `example.hlsl` legt das Modul beim ersten Start dort ab. Fehler beim Kompilieren stehen in den Einstellungen des Moduls.
- Kosten: ein Vollbild-Durchlauf, 4 bis 32 Texturzugriffe pro Pixel. Auf einer normalen Grafikkarte kaum messbar, auf schwachen Geräten die Stärke oder den Look wechseln.

Grenze: Der Shader sieht nur das fertige Bild. Er kennt weder Tiefe noch Normalen noch Schatten. Echte Shader-Packs mit Schatten, Wasser-Reflexionen und Wolken, wie in Java, brauchen Zugriff auf die Render-Pipeline von RenderDragon (Materialien und Tiefenpuffer). Das ist Phase 2 und braucht Reverse Engineering am PC (siehe unten).

Auch neu: **Blur** (Vollbild-Unschärfe, immer oder nur in Menüs) und die Hintergrund-Unschärfe des Mochi-Menüs (Einstellungen, General, "Menu background blur").

## Schnelleres Laden von Chunks (braucht PC)

Ohne Spieldaten kann man das nicht ehrlich bauen. Mögliche Stellschrauben, die am PC zu suchen und zu messen sind:

1. **Chunk-Mesh-Budget pro Frame:** RenderDragon baut nur eine begrenzte Zahl Unterchunks pro Frame neu. Gibt es eine Konstante oder einen Wert, lässt er sich anheben.
2. **Worker-Threads:** Zahl der Threads, die Chunk-Meshes bauen (Thread-Pool). Auf modernen CPUs sind es oft zu wenige.
3. **Sichtweite und Nachladen:** Wie das Spiel entscheidet, welche Chunks zuerst kommen (Blickrichtung zuerst), und ob der Client Chunks verwirft, die er gleich wieder braucht.
4. **Upload-Zeit:** Mesh-Uploads zur Grafikkarte über mehrere Frames verteilen, damit es keine Ruckler gibt.

Vorgehen: Suche über Strings und Aufrufmuster in Ghidra, Signatur in `sigs/<version>.json`, Wert über einen Effekt-Kanal (`fx::`) setzen, Modul "Chunk Boost" grau, bis die Signaturen da sind. Messung: Zeit bis alle Chunks nach einem Teleport geladen sind, Frametime-Spitzen beim Laufen. Ein Modul gilt erst als fertig, wenn die Messung eine Verbesserung zeigt.

## Eingabe-Verzögerung und "0 Ping bei Eingabe"

Den Ping zum Server kann kein Client senken. Was man senken kann, ist alles zwischen Maus und Bild:

| Maßnahme | Stand |
|---|---|
| Kurze Bildwarteschlange (`SetMaximumFrameLatency(1)`), Tearing statt VSync | gebaut (Low Latency) |
| Präziser Frame Limiter (Warten bis kurz vor dem Bild, damit Eingaben so spät wie möglich gelesen werden) | gebaut (Frame Limiter), Feinschliff offen |
| Timer 1 ms, hohe Priorität, kein Energiesparen | gebaut (System Boost), alles in einem Schalter: Performance Lock |
| Messung Klick bis Bild, Lag Analyzer teilt Eingabe, Bild, Netzwerk, Server | gebaut |
| Raw-Input-Thread mit MMCSS-Klasse "Games" (Eingaben mit höchster Priorität einsammeln) | geplant, geht ohne Signaturen |
| Mausabfrage mit 1000 Hz+ statt an Fenster-Nachrichten gebunden | geplant, geht ohne Signaturen |
| Netzwerk: DSCP/QoS-Markierung der UDP-Pakete (nur IP-Kopf, nicht der Inhalt) | geplant, prüfen ob Router es nutzen |
| Hit-Rückmeldung sofort lokal (Animation, Ton) statt erst nach Server-Antwort | teilweise (Insta Hurt Animation, Hit Sound), braucht Signaturen |

Wichtig: Alles wird gemessen (Klick bis Bild, 1 % Low), bevor wir damit werben.
