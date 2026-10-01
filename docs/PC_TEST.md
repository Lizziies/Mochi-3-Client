# Test zuhause (so wenig wie möglich)

Alles, was in der Cloud geht, ist erledigt: gebaut, unter Wine getestet, Release-Pipeline vorbereitet. Zuhause bleibt nur der Test im echten Minecraft. Dafür brauchst du kein Visual Studio und kein Terminal.

## Schritte

1. Von GitHub (Releases) `MochiLauncher.exe` und `Mochi.dll` herunterladen und in denselben Ordner legen. Windows-Defender warnt eventuell, das ist bei Injektoren normal. Für den Test eine Ausnahme für den Ordner anlegen.
2. `MochiLauncher.exe` starten und auf "Play" klicken. Der Launcher startet Minecraft, wartet und verbindet den Client.
3. Im Spiel Rechts-Shift drücken. Das Menü sollte erscheinen. FPS, CPS und Keystrokes sollten oben links stehen.
4. Ein paar Module an- und ausschalten, das HUD verschieben (Edit HUD), die Sprache umstellen (Settings).
5. Strg+L drücken. Der Client entlädt sich, das Spiel läuft normal weiter.

## Was du mir schickst

- Die Datei `%LOCALAPPDATA%\Mochi\logs\latest.log` (Launcher: Settings → Open logs).
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
