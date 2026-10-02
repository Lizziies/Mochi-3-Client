# Todo für Claude Code zuhause

Für Claude Code am PC von Felix (Windows, Minecraft Bedrock GDK). In der Cloud ist alles gebaut, was ohne das echte Spiel geht, und unter Wine im Testfenster geprüft (CI: `.github/workflows/check.yml`, Screenshots auf `ci-results/claude-onix-ui-input-fixes`). Hier steht, was nur am PC geht, in der Reihenfolge, in der es gemacht wird. Den Rahmen (Regeln, Phasen, "kein Modul bleibt grau") gibt `docs/PLAN_HOME.md`, diese Liste ist die konkrete Arbeitsliste dazu.

Zuerst lesen: `CLAUDE.md`, `docs/PLAN_HOME.md`, `docs/STATUS.md`, `docs/PC_TEST.md`, `docs/SDK.md`, `docs/VERSIONING.md`.

## 0. Code holen

Der neueste Stand liegt auf `claude/onix-ui-input-fixes`. Er enthält `main` vollständig (main ist ein direkter Vorfahre), also reicht:

```
git fetch origin
git checkout -B claude/pc-test origin/claude/onix-ui-input-fixes
```

Wenn Felix zustimmt, `main` danach vorspulen: `git push origin claude/onix-ui-input-fixes:main` (reiner Fast-Forward, nichts geht verloren).

Der Branch `handoff/2026-10-02` auf GitHub enthält denselben Stand als ZIP (`Mochi-Handoff-2026-10-02.zip`), dazu die MinGW-Testbauten und die Screenshots. Für die Arbeit trotzdem das Git-Repo nehmen, nicht das ZIP. Releases auf GitHub kann nur Felix bzw. Claude Code zuhause anlegen, die Cloud-Sitzung darf das nicht.

## 1. Bauen und Selbsttest

- [ ] DLL und Launcher mit MSVC bauen (Befehle in `CLAUDE.md`). Die Cloud baut nur mit MinGW; MSVC-Warnungen und -Fehler hier beheben.
- [ ] `MOCHI_SELFTEST=1`: keine Fehler, keine fehlenden Übersetzungen.
- [ ] Log-Zeilen `usable:` und `locked:` notieren (Ausgangszahl für `docs/STATUS.md`).

## 2. Mochi Online aktualisieren (Owner-Abzeichen, Rollen)

Der Dienst unter `https://mochi-online.lisawer008.workers.dev` läuft noch mit dem alten Code ohne Rollen.

- [ ] `cd server && npm test` (19 Tests, drei Speicher), dann `node bundle.js`.
- [ ] Neuen Code veröffentlichen: entweder `npx wrangler deploy` oder im Cloudflare-Dashboard "Edit code" und `server/dist/worker.js` einfügen. Die Spalte `role` legt sich beim ersten Aufruf selbst an.
- [ ] Prüfen, ob das Secret `ADMIN_KEY` gesetzt ist (Settings, Variables and secrets). Sonst setzen (`npx wrangler secret put ADMIN_KEY`) und Felix den Wert sagen, nicht committen.
- [ ] Felix startet Minecraft einmal mit "Mochi Online" an, damit `vlisya` angemeldet ist. Dann:
  ```
  curl -X POST https://mochi-online.lisawer008.workers.dev/v1/admin/role -H "X-Admin-Key: <ADMIN_KEY>" -H "content-type: application/json" -d "{\"name\":\"vlisya\",\"role\":\"owner\"}"
  ```
- [ ] Prüfen: `GET /v1/lookup?names=vlisya` liefert `role: "owner"`. Im Spiel steht in Tab-Liste und Chat `vlisya [Owner]` (blau). Nur er; andere Namen bekommen kein Abzeichen, ein neu geclaimter Name verliert die Rolle.

## 3. Erster Start im echten Spiel

Ablauf und Erwartungen in `docs/PC_TEST.md`, Abschnitte "Branch claude/onix-ui-input-fixes" und "Live-Einstellungen und Owner". Besonders:

- [ ] DX11 und DX12: Menü, Blur, Cosmetics-Vorschau, Fenstergröße ändern, Vollbild, Alt-Tab. `hook/Dx.cpp` übernimmt bei neuer Swapchain auf demselben Gerät (`adopt`); prüfen, dass nach Vollbild-Umschaltung nichts neu lädt.
- [ ] GameInput-Hooks (`hook/GameInput.cpp`): Menü offen = Spiel sieht keine Eingabe; kein Kamerasprung beim Schließen.
- [ ] Zurück aus den Minecraft-Einstellungen, Serverwechsel, Alt-Tab: Module bleiben an, HUD ist sofort da, nichts lädt neu (`sdk/Live.cpp` wertet eine Lücke unter 8 s als Serverwechsel).
- [ ] Jede Einstellung wirkt sofort am Modul (Felix' Fehler von zuhause: "die Extra-Einstellungen haben nichts verstellt"). In der Cloud wurden alle Module auf tote Einstellungen durchgesehen und repariert (siehe `docs/STATUS.md`, Abschnitt "abends"); im Spiel mit der Liste in `PC_TEST.md` gegenprüfen.
- [ ] Strg+L entlädt, neu injizieren geht.

## 4. Signaturen: kein Modul bleibt grau

Hauptarbeit. Phasen 1 und 2 aus `docs/PLAN_HOME.md` (Speicherabbild, `tools/sigcheck`, Wellen nach Nutzen). Nach jeder Welle Zahl der grauen Module in `docs/STATUS.md`.

Zusätzlich zu den dort genannten Signaturen fehlen in `dll/src/sdk/Live.cpp` diese Spielerfelder (die Demo-Daten haben sie, der Live-Leser noch nicht): `inWater`, `flying`, `gliding`, `swimming`, `usingItem`, `useProgress`, `blocking`, `effects`, `view` (Perspektive), `mode` (Spielmodus), `team`. Module wie Toggle Sprint (Schwimmen/Fliegen), Item Use, Potion HUD, Perspektive und Teams hängen daran.

## 5. GUI Scale wie bei Flarial

Das Modul "GUI Scale" (`modules/comfort/Link.hpp`) hat absolute Stufen 1 bis 6, "nur ganze Stufen" und Knöpfe 1 / 1.5 / 2 / 2.5 / 3 / 4. Es fehlt der Hook:

- [ ] `ClientInstance::_updateScreenSizeVariables` finden (Signatur selbst, nicht aus Flarial). Eigener Detour, der den Skalierungswert setzt.
- [ ] Beim Ändern des Reglers sofort neu layouten (die Funktion einmal mit den aktuellen Bildschirmwerten aufrufen), kein Neustart, kein Fensterwechsel nötig.
- [ ] Beim Ausschalten den Wert des Spiels zurücksetzen.
- [ ] Prüfen: Wert 2 sieht aus wie bei Flarial mit 2 (Screenshot nebeneinander).

## 6. Eingabe schneller als Flarial

In der Cloud schon gemacht: Fensternachrichten werden auf dem Render-Thread abgearbeitet (kein Lock im Eingabe-Thread), Mausbewegung ohne offenes Menü wird gar nicht erst eingereiht, Limiter wartet nach Present, Speicher-Schreibrechte pro Frame zwischengespeichert, Shader kompilieren im Hintergrund. Was am PC bleibt:

- [ ] DX12: Frame-Latency-Waitable-Swapchain bzw. Fence, damit höchstens ein Bild in der Warteschlange liegt (`modules/perf/LowLatency`).
- [ ] FPS-Limiter: auf welchem Thread er wartet, mit Thread-IDs messen (Render- vs. Spiel-Thread), dann den Platz festlegen.
- [ ] Logging asynchron machen (Schreiben auf die Platte im Hintergrund-Thread), Raw-Input-Pfad ohne zusätzliche Systemaufrufe.
- [ ] Messen wie in Phase 6 von `PLAN_HOME.md`: Klick bis Bild, Vanilla / Mochi / Flarial / Onix, gleiche Szene. Zahlen in `docs/PARITY.md`. Ziel: Mochi gleich oder besser als Flarial.

## 7. Cosmetics und Namen im Spiel

Mochi Online schickt schon, wer was trägt (`worn`) und die Namensfarbe; in Tab-Liste und Chat wird beides gezeichnet. Für die Figur selbst:

- [ ] Phase 5 aus `PLAN_HOME.md`: Teile über die Skin-Geometrie oder den Render-Aufruf der Figur, nie als Overlay (Wände!).
- [ ] Namensfarbe und Abzeichen auch über dem Kopf (Nametag-Hook, `fx.selfNametag` und der Weg für andere Spieler).
- [ ] Mit zwei Konten prüfen: beide sehen Cosmetics und Namen des anderen.

## 8. Rest

- [ ] Echte Item-Symbole (Armor HUD, Item Tracker, Inventory Viewer) aus den Texturen des Spiels statt Text.
- [ ] Automatische CPS-Grenze pro Server (CPS Limiter liest die Grenze aus `servers/servers.json`).
- [ ] Schwache PCs: Profil "Low" (Auto-Leistung) auf einem schwachen Rechner oder mit gedrosselter GPU prüfen. Low schaltet Blur, Glow und Extras ab, der Frame Limiter bleibt an.
- [ ] Parität mit Flarial und Onix Punkt für Punkt (`docs/PARITY.md`, `docs/FLARIAL_REAL.md`).

## 9. Exe bauen und veröffentlichen (ganz zum Schluss)

Erst wenn 1 bis 8 erledigt sind und `locked:` im Log leer ist (Ausnahme: Sperren durch Server-Regeln).

- [ ] Einzelne `MochiLauncher.exe` mit `-DMOCHI_DLL` und `-DMOCHI_COSMETICS` (Befehle in `CLAUDE.md`), allein in einem leeren Ordner testen.
- [ ] Tag `v0.1.0-alpha.1` pushen, `release.yml` baut dieselbe Exe. Herunterladen und wie ein Nutzer testen, Selbst-Update auf ein zweites Pre-Release prüfen.
- [ ] Vor dem öffentlichen Release: Name "Mochi" ersetzen, README, Datenschutz-Seite.

Nach jedem Schritt: Zeile in `docs/STATUS.md` und `docs/TESTLOG.md`, kleiner Commit.
