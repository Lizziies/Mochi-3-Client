# Updates, neue Minecraft-Versionen und Downgrade

Wie Mochi aktuell bleibt, was bei einem Minecraft-Update passiert und wie Versionswechsel gedacht sind. Ehrlich getrennt in "gebaut", "geplant" und "noch zu klären".

## Wer baut was

| Teil | Zuständig | Stand |
|---|---|---|
| Launcher-Updater (DLL + Launcher selbst) | Session A (Launcher) | gebaut, unter Wine getestet bis zum Download |
| Release-Pipeline (GitHub Action, Prüfsummen) | Session A | geschrieben (`.github/workflows/release.yml`), noch nie gelaufen |
| Signaturen nachladen (DLL lädt `sigs/` von GitHub) | war schon in der DLL | vorhanden, aber ohne echte Signaturen |
| Server-Regeln nachladen (`servers.json`) | war schon in der DLL | vorhanden |
| Version-Switcher | Session A, nach dem Release | nicht gebaut |
| Signaturen für neue Minecraft-Versionen finden | Felix + Claude Code am PC | offen, das ist die eigentliche Arbeit |

Session B (Module) baut keine Update-Logik.

## Die vier Ebenen

Es gibt vier getrennte Dinge, die sich aktualisieren, mit unterschiedlichem Aufwand.

| Ebene | Was | Wie | Braucht neues Release? |
|---|---|---|---|
| 1. Signaturen und Server-Regeln | Daten (`sigs/<version>.json`, `servers/servers.json`) | DLL lädt sie beim Start von `raw.githubusercontent.com`, mit Cache und eingebauter Kopie | nein, ein Commit reicht |
| 2. Client (`Mochi.dll`) | Programmcode | Launcher holt das neueste GitHub-Release, prüft SHA-256, ersetzt die DLL | ja |
| 3. Launcher (`MochiLauncher.exe`) | Programmcode | Launcher lädt die neue exe, benennt die laufende um, startet die neue, löscht die alte | ja |
| 4. Minecraft selbst | Spiel | Microsoft Store / Xbox-App | nicht unsere Sache |

## Ablauf beim Klick auf "Spielen"

1. Launcher fragt `api.github.com/repos/<owner>/<repo>/releases/latest` (bei Beta-Kanal die Liste der Releases).
2. Ist der Tag neuer als die installierte Version, lädt er `Mochi.dll` und `checksums.txt`, prüft die Prüfsumme und ersetzt `%LOCALAPPDATA%\Mochi\bin\Mochi.dll`. Stimmt die Prüfsumme nicht, wird nichts installiert.
3. Läuft Minecraft schon, wird der Prozess verwendet, sonst gestartet (Aktivierung über die App-ID, Rückfall auf `minecraft:`).
4. Launcher wartet auf Fenster und DirectX-Module, wartet 2 Sekunden und injiziert per `LoadLibraryW`.
5. Die DLL liest ihre Minecraft-Version, lädt die passende `sigs/<version>.json` (GitHub → Cache → eingebaut) und schaltet Module ohne gefundene Signatur grau.

"Selbst-Update": Button "Aktualisieren" auf der Startseite. Er lädt zusätzlich `MochiLauncher.exe`, ersetzt sich selbst und startet neu.

Voraussetzung: Ein Release mit den drei Dateien `Mochi.dll`, `MochiLauncher.exe`, `checksums.txt`. Die Action erzeugt sie, wenn du einen Tag `v0.1.0` pushst. Auf GitHub muss außerdem ein Standardbranch (`main`) existieren, weil Signaturen von dort geladen werden.

## Was passiert, wenn Minecraft sich aktualisiert

Microsoft aktualisiert Minecraft automatisch. Dann kann es sein, dass die Signaturen nicht mehr passen.

1. Start: DLL erkennt die neue Version, `sigs/<neue-version>.json` gibt es noch nicht.
2. Spiel-Module ohne Signatur werden grau mit "not available on this version". Overlay-Module (FPS, CPS, Keystrokes, Latenz, Netzwerk, Crosshair) laufen weiter. **Das Spiel stürzt nicht ab.**
3. Der Launcher zeigt "Untested" statt "Supported" (Abgleich mit `sigs/index.json`).
4. Wir (Felix + Claude Code) finden die kaputten Signaturen, schreiben `sigs/<version>.json`, committen. Beim nächsten Start laden alle Spieler sie automatisch. **Kein neues Release nötig.**
5. Nur wenn sich Klassen oder Strukturen im Spiel ändern (neue Offsets, nicht nur neue Adressen), ist auch ein DLL-Release nötig.

Das spart die Tage Wartezeit, die Flarial und Onix nach Updates haben, ersetzt aber nicht die Arbeit, die Signaturen zu finden.

## Downgrade und "neueste Version laden"

Hier muss man ehrlich sein, weil es das schwierigste Stück ist.

**Fakten:**
- Seit 1.21.120 läuft Bedrock für Windows als GDK-Build. Die Versionen sind an das Microsoft-Konto gebunden. Einen offiziellen Downgrade gibt es nicht.
- LeviLauncher (Open Source, GPL-3.0-only, von LiteLDev, geschrieben in Go/Wails) kann mehrere Release- und Preview-Versionen nebeneinander installieren, jede mit eigenem Ordner und eigenen Welten. Man braucht dafür eine eigene gültige Minecraft-Lizenz.
- Wie genau LeviLauncher die Spieldateien bezieht, haben wir noch nicht im Detail geprüft (Webseite war gesperrt). Das muss vor dem Bau nachgesehen werden.

**Was Mochi tut, in Stufen:**

| Stufe | Inhalt | Wann |
|---|---|---|
| 1 (gebaut) | Im Launcher unter "Versions" gibt es die Versionsverwaltung: ein Klick lädt die offizielle `LeviLauncher.exe` vom GitHub-Release von LiteLDev nach `%LOCALAPPDATA%\Mochi\tools` und startet sie. LeviLauncher installiert und wechselt Versionen (auch alte, mit eigener Lizenz). Mochi läuft als getrenntes Programm daneben (kein GPL-Code in Mochi) und verbindet sich automatisch mit der Version, die LeviLauncher startet. Mochi arbeitet mit **jeder** Minecraft-Version, die gerade läuft. Der Auto-Inject-Wächter im Launcher injiziert auch, wenn das Spiel von einem anderen Launcher gestartet wurde. Wer eine alte Version will, installiert sie mit LeviLauncher, Mochi hängt sich dran. Für Versionen mit `sigs/<version>.json` laufen die Spiel-Module. | Release 1 |
| 2 | Versionen-Seite zeigt die installierten Versionen (auch die von LeviLauncher, wenn erkennbar), mit Badge "Mochi compatible". Start einer ausgewählten Version aus dem Launcher heraus. | nach Release 1 |
| 3 | Eigener Download neuer und alter Versionen, nur über die Microsoft-Berechtigung des Nutzers. Wird nur gebaut, wenn das technisch und rechtlich sauber geht. Wir hosten keine Spieldateien und übernehmen keinen GPL-Code in Mochi. | offen |

"Die neueste Version laden": Die neueste Minecraft-Version kommt über den Microsoft Store bzw. die Xbox-App. Der Launcher zeigt, wenn eine neuere Version installiert ist, als die von Mochi geprüfte, und bietet einen Link zum Store an. Mochi selbst lädt kein Minecraft.

Auto-Update von Minecraft für eine gepinnte Version zu verhindern, ist Teil von Stufe 2 oder 3 und noch nicht untersucht.

## Was noch zu klären ist (am PC)

- Ob `GetPackagesByPackageFamily` auf GDK die Version liefert und wie sie sich in die angezeigte Version umrechnet.
- Wie LeviLauncher Versionen bezieht und wo es sie ablegt. Davon hängt Stufe 2 und 3 ab.
- Wie sich der Spielstart bei einer von LeviLauncher installierten Version verhält (andere Pfade, andere App-ID).
- Ob der Updater hinter Firmen- oder Schul-Proxys funktioniert (WinHTTP nutzt die Systemeinstellung).

## Sicherheit

- Prüfsumme für jede heruntergeladene Datei. Fehlt `checksums.txt` oder stimmt sie nicht, wird nichts installiert.
- Die DLL lädt nur Daten nach (`sigs/`, `servers.json`), niemals Code.
- Code-Signing der exe wäre besser gegen Antivirus-Warnungen, kostet aber ein Zertifikat.
