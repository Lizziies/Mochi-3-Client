# Release in ein paar Tagen

Ziel: Monchi als besten kostenlosen Bedrock-PvP-Client veröffentlichen. Diese Datei sagt, womit wir uns abheben, was zum Release wirklich laufen muss und was ehrlich noch offen ist.

## Womit wir uns abheben

Nicht mit der Modulzahl allein (Flarial hat 140+), sondern mit Dingen, die die anderen nicht oder schlecht haben.

| Punkt | Warum es zählt | Stand |
|---|---|---|
| Eingabe-Latenz mit Messung | Latenz-Overlay zeigt Klick → Bild in ms. Wir können Zahlen vorzeigen, andere nur behaupten | Overlay und Low Latency vorhanden, Messung am PC offen |
| Instant Input | Klick wird so früh wie möglich verarbeitet, ohne Extra-Schläge (siehe `FEATURES.md` 1.3) | geplant, Spiel-Hooks nötig |
| WLAN-/Netzwerk-Modul | Misst Jitter und Paketverlust, erklärt, wo die Verzögerung herkommt. Gibt es bei keinem Client | geplant (Session B) |
| Läuft nach Updates weiter | Signaturen als Datei auf GitHub: ein Fix ist ein Commit, kein Release. Fehlende Signatur = Modul grau, kein Absturz | System fertig, echte Signaturen fehlen |
| Server-Regeln automatisch | Verbotene Module werden auf Hive, Lifeboat, CubeCraft usw. gesperrt oder markiert | fertig, Test mit echten Servern offen |
| Launcher mit Auto-Update | Ein Klick, Update mit Prüfsumme, Selbst-Update | gebaut, Wine-Test ok |
| Zwei Sprachen | Englisch und Deutsch, folgt Windows | fertig |
| Kostenlos, offen, Lua | Onix verlangt für Scripting Geld | Lua noch nicht gebaut |
| Optik | Eigener Stil, Themes, Animationen | fertig, Feinschliff offen |

Aussage für die Veröffentlichung nur mit Belegen: "Messbar weniger Eingabe-Latenz" nur, wenn die Zahlen vom PC vorliegen.

## Muss zum Release laufen

Reihenfolge nach Risiko:

1. **Im echten Minecraft laden.** DLL injizieren, DX12-Hook, Menü, HUD, sauber entladen. Bisher nur unter Wine geprüft.
2. **Launcher im echten System.** Spielstart (`ActivateApplication` mit der richtigen AUMID), Versionserkennung, Injection, Selbst-Update. Siehe offene Punkte unten.
3. **Overlay-Module ohne Signaturen.** FPS, CPS, Keystrokes, Latenz, Netzwerk, Post-Effekte, Crosshair. Das ist der sichere Kern für Release 1.
4. **Erste Spiel-Signaturen.** Zoom, Fullbright, No View Bobbing, No Hurt Cam, Toggle Sprint/Sneak, Coordinates, Armor HUD, Hitbox. Jedes Modul, dessen Signatur nicht sicher gefunden wurde, erscheint grau statt kaputt.
5. **GitHub-Release-Pipeline.** Release mit `MonchiLauncher.exe` (Client und Cosmetics eingebettet, eine einzige Exe wie bei Flarial) und `checksums.txt`, damit der Updater funktioniert. Dazu eine Action, die bei einem Tag baut.
6. **README, Screenshots, Name.** Arbeitstitel "Monchi" vor dem Release prüfen und global ersetzen. Logo-Rechte klären.

## Ehrliche Risiken

- **Signaturen sind das Nadelöhr.** Ohne echte Spiel-Signaturen gibt es keine Spiel-Module. Das erfordert Minecraft am PC und Ghidra/IDA. Das kann Tage dauern pro Version.
- **Antivirus.** Injektoren lösen Warnungen aus. Das lässt sich nicht ganz vermeiden. Der Launcher erklärt es. Code-Signing (Zertifikat) wäre besser, kostet Geld.
- **Mojang/Microsoft und Server.** Der Client gibt keine Vorteile, aber auf manchen Servern sind Clients generell nicht erlaubt. Die Server-Regeln helfen, ersetzen aber keine Freigabe. CubeCraft und Lifeboat nach Release um Freigabe bitten.
- **Version-Switcher.** Noch nicht gebaut. Für Release 1 nicht versprechen, nur "kommt".
- **Namen und Marken.** Kein Onix/Flarial-Name, -Logo oder -Design. Kein Code aus Flarial (AGPL).
- **Recht.** Monchi ist nicht mit Mojang oder Microsoft verbunden. Der Hinweis steht im Launcher.

## Offene Prüfpunkte am PC (für Claude Code)

- `GetPackagesByPackageFamily` mit der Familie `Microsoft.MinecraftUWP_8wekyb3d8bbwe`: liefert sie auf GDK-Installationen die Version, und wie wird sie in die angezeigte Version (z. B. 1.26.x) umgerechnet?
- AUMID für `ActivateApplication`: `...!Game` oder `...!App`? Der Launcher probiert beide, danach das Protokoll `minecraft:`.
- Prozess- und Fenstererkennung: Läuft `Minecraft.Windows.exe` direkt, oder startet ein Helfer (`gamelaunchhelper`) zuerst? Muss der Launcher länger warten?
- Injection in den echten Prozess: ACL für `ALL APPLICATION PACKAGES`, Rechte, Zeitpunkt (nach dem ersten Fenster).
- DX12-Pfad im echten Spiel, danach DX11 erzwungen.
- Latenz-Messung: Latenz-Overlay vorher/nachher mit den Optionen aus `INPUT.md`, Zahlen in `docs/STATUS.md` festhalten.

## Zeitplan (grob, abhängig von den Tests am PC)

| Tag | Ziel |
|---|---|
| 1 | Bauen mit MSVC, im echten Spiel laden, Fehler beheben. Overlay-Module testen |
| 2 | Launcher im echten System. Erste Signaturen (Zoom, Fullbright, No Bobbing, No Hurt Cam) |
| 3 | Weitere Signaturen, Server-Regeln testen, Latenz messen |
| 4 | Release-Pipeline, README, Screenshots, Release-Kandidat privat testen |
| 5 | Veröffentlichen |

Wenn die Signaturen länger dauern, geht Release 1 mit dem Overlay-Kern, der Latenz-/Netzwerk-Auswertung und den Spiel-Modulen, die bis dahin laufen. Der Rest kommt per `sigs.json`-Update ohne neues Release nach.
