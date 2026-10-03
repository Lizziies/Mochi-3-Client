# Test zuhause (so wenig wie möglich)

Alles, was in der Cloud geht, ist erledigt: gebaut, unter Wine getestet, Release-Pipeline vorbereitet. Zuhause bleibt nur der Test im echten Minecraft. Dafür brauchst du kein Visual Studio und kein Terminal.

## Schritte

1. Von GitHub (Releases) `MonchiLauncher.exe` herunterladen. Das ist eine einzige Datei, Client und Cosmetics stecken darin. Windows-Defender warnt eventuell, das ist bei Injektoren normal. Für den Test eine Ausnahme für den Ordner anlegen.
2. `MonchiLauncher.exe` starten und auf "Play" klicken. Der Launcher startet Minecraft, wartet und verbindet den Client.
3. Im Spiel Rechts-Shift drücken. Das Menü sollte erscheinen. FPS, CPS und Keystrokes sollten oben links stehen.
4. Ein paar Module an- und ausschalten, das HUD verschieben (Edit HUD), die Sprache umstellen (Settings).
5. Strg+L drücken. Der Client entlädt sich, das Spiel läuft normal weiter.

## Versionswechsel testen

1. Launcher, Seite Versions: LeviLauncher installieren (Knopf), dort eine ältere Minecraft-Version laden.
2. Zurück im Monchi-Launcher auf "Rescan". Die neue Version sollte in der Liste stehen. Steht sie nicht da, mit "Add folder" den Ordner wählen, in dem LeviLauncher die Version abgelegt hat.
3. "Use this one" wählen und auf Play klicken. Monchi startet genau diese Version (die Exe direkt) und verbindet sich.
4. Launcher schließen und neu öffnen: die gewählte Version steht weiter auf "In use".
5. Wenn die Version sofort wieder schließt: die Meldung im Launcher und `latest.log` schicken. Direkter Start einer Exe außerhalb des Stores ist am PC noch nicht geprüft.

## Was du mir schickst

- Die Datei `%LOCALAPPDATA%\Monchi\logs\latest.log` (Launcher: Settings → Open logs).
- Einen Screenshot vom Menü und, wenn etwas schiefging, vom Fehler.
- Kurz: Version von Minecraft (Einstellungen im Spiel) und ob DX12 oder DX11.

Mehr ist nicht nötig. Mit dem Log kann ich aus der Cloud die Fehler beheben.

## Wenn etwas nicht klappt

| Problem | Was tun |
|---|---|
| Launcher startet Minecraft nicht | Minecraft einmal selbst starten, dann im Launcher "Play" (der Auto-Inject erkennt das laufende Spiel) |
| Spiel startet, kein Menü | Log schicken. Meist ein Hook auf DX12, den ich dann anpasse |
| Alle Spiel-Module sind grau | Erwartet: es gibt noch keine Signaturen für deine Version. Siehe unten |
| Antivirus löscht die DLL | Ausnahme für den Ordner anlegen |

## Was nur am PC geht: Signaturen

Spiel-Module (Zoom, Fullbright, Hitbox, Armor HUD …) brauchen Adressen aus der Minecraft-Exe. Die Exe darf nicht ins Repo, deshalb kann nur der PC sie lesen. Das ist der einzige Teil, der Terminal-Arbeit bleibt. Ablauf dafür:

1. Claude Code am PC starten, `docs/SDK.md` und `docs/VERSIONING.md` lesen lassen.
2. Es findet die Signaturen für deine Minecraft-Version und schreibt `sigs/<version>.json`.
3. Commit, Push. Danach lädt jeder Client die Signaturen automatisch, ohne neues Release.

Die Demo-Daten im Modul "Game Support" (Einschalter) lassen alle Spiel-Module mit simulierten Werten laufen, damit man sie ohne Signaturen im Menü ausprobieren kann. Im echten Spiel greifen sie erst mit Signaturen.

## Branch claude/onix-ui-input-fixes: was zuhause zu prüfen ist

1. Spiel starten, Client verbinden, **im Hauptmenü Rechts-Shift drücken: es darf nichts passieren.** In eine Welt gehen, Rechts-Shift: das Menü öffnet sich.
2. Im Log nach `gameinput:` suchen. Erwartet: `gameinput: readings hooked (v…) with … hooks, menu now blocks keyboard and mouse`. Steht dort `runtime not loaded`, liest diese Version die Eingabe anders, dann das Log schicken.
3. Menü offen lassen, WASD drücken, Maus bewegen, klicken: **die Figur bleibt stehen, die Kamera bewegt sich nicht, es wird nicht angegriffen.** Menü mit Rechts-Shift oder Esc schließen: kein Kamerasprung, kein Pause-Menü von Minecraft.
4. Toggle Sprint an, Strg einmal drücken, loslaufen: die Figur sprintet ohne gehaltene Taste. Die ersten Zeilen `gameinput: key scan=… vk=…` im Log zeigen, welche Tastencodes das Spiel sieht, die bitte mitschicken.
5. Zoom an, C halten: das Bild wird vergrößert, die Maus ist langsamer, das Mausrad ändert die Stufe und blättert nicht durch die Hotbar.
6. Sens Multiplier an, Basiswert auf 0,5: die Maus ist nur halb so schnell.
7. Menü ansehen und mit den Onix-Screenshots vergleichen: drei Panels (Suche, Liste, Einstellungen), Blur hinter den Panels, Text scharf. Einen Screenshot in 1080p schicken.
8. Im Menü einfach ein Wort tippen (z. B. "reach"): es landet in der Suche, auch der erste Buchstabe. Esc leert die Suche, nochmal Esc schließt das Menü.
9. Global Settings → Appearance: eine andere Akzentfarbe anklicken. Schalter, Slider und aktive Zeilen wechseln sofort die Farbe, auch nach Neustart des Spiels. Der Launcher übernimmt die Farbe beim nächsten Start, solange dort keine eigene gewählt ist.

## Live-Einstellungen und Owner

Jede Einstellung muss sofort im Spiel wirken, ohne das Modul aus- und anzuschalten und ohne Neustart.

1. Zoom an, C halten, im Menü den Zoom-Regler verschieben: die Vergrößerung ändert sich beim nächsten Halten sofort.
2. FPS: "Show" auf "Frame time", dann "Both". "Update interval" auf 2 s: die Zahl springt nur noch alle 2 s.
3. Keystrokes: "Glow when pressed" auf eine andere Farbe stellen und eine Taste drücken. Ping Counter: die Zahl ist grün, gelb oder rot je nach Ping.
4. Ein paar HUD-Module mit Farben und Größe durchgehen (Coordinates, CPS, Armor HUD): jede Änderung ist sofort zu sehen.
5. Minecraft-Einstellungen öffnen und wieder schließen, Alt-Tab, Vollbild umschalten, Server wechseln: alle Module bleiben an, das HUD ist sofort wieder da, im Log steht kein neuer Start der Module.
6. GUI Scale an, Knopf "2" drücken: die Minecraft-Oberfläche hat die Größe 2 wie bei Flarial. Geht erst, wenn der Hook gebaut ist (`HOME_TODO.md` Punkt 5); bis dahin ist das Modul grau.
7. Item Tracker an, etwas aufheben und etwas wegwerfen: rechts erscheint kurz "+3 …" grün und "−1 …" rot.
8. Owner (erst nach `HOME_TODO.md` Punkt 2): Monchi Online an, auf einen Server. In der Tab-Liste und im Chat steht `vlisya [Owner]`. Ein Freund mit Monchi sieht das genauso, bei sich selbst steht kein Abzeichen.
