# Wären wir schon ein Ersatz für Flarial? Ehrliche Lage

Kurzantwort: **Nein, noch nicht.** Auf dem Papier holen wir Flarial mit dem Plan in `docs/PLAN_B.md` ein. Im echten Spiel ist bisher nichts bewiesen. Hier steht, woran man es misst und wo wir stehen.

Stand: 2026-10-01. Die Prozentzahlen sind grobe Schätzungen, keine Messwerte.

## Die sechs Kriterien

| Kriterium | Flarial | Monchi heute | Monchi nach Plan B | Anmerkung |
|---|---|---|---|---|
| **1. Umfang** (Module) | 129 in der Registrierung, Webseite nennt 104 für Windows | 138 registriert, davon viele Extras | ca. 170 mit allen Standard-Modulen | Umfang ist das kleinste Problem |
| **2. Tiefe** (Einstellungen wie das Original) | voll | ca. 55 % bei den Kern-Modulen (Hitbox, Keystrokes, Coordinates, Tab List flach) | ca. 95 % | Plan B Phase 3 und 4 |
| **3. Funktioniert im echten Spiel** | ja, seit Jahren erprobt | **nein, nichts getestet.** 68 von 138 Modulen warten auf Signaturen, darunter fast alle PvP-Kern-Module | erst nach den Tests am PC | **Das ist der entscheidende Punkt** |
| **4. Stabilität** | tausende Nutzer, viele Updates | unbekannt, kein einziger echter Lauf | unbekannt | Crash-Guard und graue Module helfen, ersetzen aber keinen Langzeittest |
| **5. Update-Tempo** nach Minecraft-Updates | DLL-Neubau, oft Tage bis Wochen | Architektur kann schneller sein (Signaturen als Datei), aber **jemand muss sie finden** | gleich, solange das Finden Handarbeit ist | siehe unten |
| **6. Umfeld** | Android-Version, Marketplace, Skripte, Discord-Community, Online-Configs, großer Name | nichts davon | Skripte und Config-Codes ja, Community nein, Android nein | Das lässt sich nicht "bauen", nur aufbauen |

## Was wir wirklich besser machen können (und was nur Behauptung ist)

| Vorteil | Echt? |
|---|---|
| Server-Regeln automatisch (gesperrte Module pro Server) | echt, gebaut, mit echten Servern noch nicht getestet |
| Graue Module statt Absturz bei fehlender Signatur | echt, gebaut |
| Signaturen und Server-Regeln als Datei nachladen, ohne Release | echt, gebaut, wirkt aber erst mit echten Signaturen |
| Latenz-Messung, Lag Analyzer, Network Monitor (WLAN) | echt gebaut. **Ob der Client die Latenz wirklich senkt, ist ungemessen.** Nicht damit werben, bevor es Zahlen gibt |
| Instant Hit | gebaut, wirkt erst mit Hooks im echten Spiel |
| Versionsverwaltung (LeviLauncher) im Launcher | echt gebaut, Download getestet |
| Gratis Skripte (Lua) | geplant, nicht gebaut |
| Zwei Sprachen, Menü-Optik | echt |

## Der größte Haken: Signaturen

Ein Spiel-Modul (Zoom, Armor HUD, Hitbox, Reach, Combo …) braucht Adressen im Spielprogramm. Die findet nur jemand mit der echten Minecraft-Datei, meist per Ghidra/IDA. Bei Flarial und Onix machen das Entwickler-Teams nach jedem Minecraft-Update. Bei uns macht es Felix mit Claude Code am PC. Ohne diesen Schritt ist unser Client ein schönes Menü mit Overlays (FPS, CPS, Keystrokes, Latenz, Netzwerk, Effekte), aber kein Ersatz für Flarial.

Wie wir das Tempo hinbekommen können:
1. Ein Werkzeug `tools/sigcheck`, das alle Signaturen gegen eine neue Minecraft-Version prüft (im Plan, noch nicht gebaut).
2. Signaturen mit Platzhaltern schreiben, damit sie kleine Updates überleben.
3. Beim Start der DLL fehlende Signaturen melden (Log), damit Felix nur die kaputten neu findet.
4. Mit der Zeit eine Muster-Bibliothek anlegen, die Funktionen nach Namen oder Strukturen wiederfindet.

## Wann dürfen wir sagen "Monchi ist ein Ersatz"?

Alle fünf Punkte müssen erfüllt sein:

1. **Mindestens 30 Stufe-1/2-Module laufen im echten Spiel** auf der aktuellen Minecraft-Version, einzeln von Hand geprüft (Liste mit Haken in `docs/STATUS.md`).
2. **Keine Abstürze in 5 Stunden Spielzeit** auf Hive und Zeqa, mit allen Standard-Modulen an.
3. **Ein neues Minecraft-Update ist innerhalb von 48 Stunden unterstützt**, einmal real durchgespielt.
4. **Gemessene Eingabe-Latenz:** vorher/nachher-Zahlen mit dem Latenz-Overlay, die belegen, dass wir mindestens so gut wie Flarial sind.
5. **Zehn externe Tester** (Freunde, Discord) spielen eine Woche und finden nichts Gravierendes.

Bis dahin gilt: Monchi ist **Alpha**, gedacht als Ergänzung, nicht als Ersatz.

## Was ich dir zum Starten empfehle

- Realistisch ist ein **öffentlicher Alpha-Release** mit klarem Hinweis ("Alpha, Spiel-Module sind noch gesperrt, Overlay läuft"). Das bringt Rückmeldungen und das Log-Material, das wir brauchen.
- Den Titel "bester PvP-Client" erst nach den fünf Punkten oben verwenden. Vorher schadet es dem Ruf.
- Der zweite Account arbeitet Plan B ab. Du testest zuhause nach `docs/PC_TEST.md`. Claude Code am PC findet danach die ersten Signaturen.

## Nachtrag Session B (Phasen 0 bis 7 abgearbeitet)

- Gebaut seit der Bewertung oben: Lua-Skripte (Sandbox, `docs/SCRIPTING.md`), Config-Codes, Skript-Liste von GitHub, Discord Rich Presence, Hive Utils, Zeqa Utils, Hive Stats, Crystal-PvP-Geisterliste, gemeinsame HUD-Optik mit echtem Blur, Tab List, Kompassleiste, Debug Menu mit Graph und alle in `PLAN_B.md` genannten fehlenden Module. 171 Module insgesamt.
- Ohne Demo-Daten und ohne Signaturen sind 67 von 171 Modulen nutzbar, 104 grau. Mit den Demo-Daten laufen alle 171 unter Wine ohne Absturz (`tools/cross.sh tour`).
- Unverändert der entscheidende Punkt: Im echten Spiel ist nichts getestet. Die Signaturen und Offsets stehen in `docs/SDK.md`.
