# Zusammenarbeit mehrerer Sessions

Mehrere Claude-Sessions (verschiedene Accounts) arbeiten parallel am selben Repo. Damit niemand dem anderen Dateien überschreibt, gelten diese Regeln.

## Grundregeln

1. Jede Session arbeitet auf **ihrer eigenen Branch** und pusht nur dorthin. Nie auf die Branch einer anderen Session, nie direkt auf `main`.
2. Jede Session bleibt in **ihrem Bereich** (Tabelle unten). Dateien außerhalb des eigenen Bereichs werden nicht angefasst. Gibt es dort etwas zu ändern, schreibt die Session es in `docs/HANDOFF.md` unter "Wünsche an andere Sessions".
3. Zusammengeführt wird nur von Felix (oder auf seine Ansage). Bei Konflikten merged man `main` bzw. die andere Branch in die eigene, löst auf und pusht. Nie History umschreiben.
4. Vor jeder Arbeit `docs/MASTER.md`, `CLAUDE.md`, `docs/STATUS.md` lesen. Nach jedem Block `docs/STATUS.md` und `docs/HISTORY.md` ergänzen (nur eigene Zeilen anhängen, nichts löschen).
5. Bauen und Testen geht doch: `tools/cross.sh setup` installiert MinGW, Wine, Xvfb und ImageMagick, `tools/cross.sh build` baut DLL, Launcher und Testhost, `tools/cross.sh shots` startet die DLL im Testhost unter Wine und macht Screenshots (echte Menü-Bilder). Nach jedem Block bauen, Fehler beheben. Den Test im echten Minecraft macht danach Felix mit Claude Code am PC.
6. Sprachen: Alle sichtbaren Texte im Code auf Englisch schreiben, über `i18n::tr(...)` anzeigen und einen deutschen Eintrag in der Tabelle des eigenen Bereichs anlegen (Details im Abschnitt Languages in `CLAUDE.md`). Neue Module: eigene Tabelle `dll/src/modules/Lang_<Name>.cpp` anlegen, damit niemand in dieselbe Datei schreibt.
7. Code-Stil und harte Regeln aus `CLAUDE.md` gelten für alle (kaum Kommentare, kurze Namen, keine Cheats).

## Bereiche

| Session | Bereich | Branch |
|---|---|---|
| A (Launcher) | `launcher/`, `tools/preview/`, `docs/PLAN.md` (nur Phase 7 und 8), `docs/UI.md` (nur Abschnitt Launcher) | `claude/flarial-client-chat-access-h80yqs` |
| B (Module) | `dll/src/modules/`, `dll/src/sdk/` (neu), `servers/`, `docs/MODULES.md`, `docs/FEATURES.md` | eigene Branch, Vorschlag `claude/modules-b` |
| A übernimmt zusätzlich | `dll/src/gui/`, `dll/src/render/`, `common/`, `.github/` (Menü-UI, Animationen, Sprachen, Release-Pipeline), solange der Hauptaccount nicht aktiv ist | siehe oben |

Gemeinsam beschreibbar, aber nur anhängen: `docs/STATUS.md`, `docs/HISTORY.md`, `docs/RESEARCH.md`, `docs/HANDOFF.md`.

Konfliktstelle: `dll/src/modules/Manager.cpp` (Modul-Liste). Nur Session B ändert sie.

## Aufgabe für Session B (Module)

Ziel: die fehlenden Module bauen. Quellen: `docs/FEATURES.md` (voller Plan), `docs/MODULES.md` (Tiers), `docs/CLIENTS.md` (was die anderen haben).

Reihenfolge:

1. Latenz-Overlay und Netzwerk-/WLAN-Modul (`FEATURES.md` 1.2 bis 1.4), Frame Limiter, Render Options.
2. Overlay-Module ohne Signaturen: Post-Effekte (Motion Blur, Saturation/Hue, Brightness, Sharpen, Screen Tint, Deepfry), Custom Crosshair Editor, Hit-Marker, Screenshot+, Block Game, Pomodoro.
3. Spiel-Schnittstelle (`dll/src/sdk/`) mit simulierten Demo-Daten, damit alle Spiel-Module testbar sind und beim echten Test nur Signaturen fehlen. Signaturen selbst gibt es erst am PC.
4. Spiel-Module nach Priorität 1 in `FEATURES.md`: Kampf-Anzeigen (Reach, Combo, Hit Counter, Cooldowns), Armor/Potion/Pot HUD, Coordinates, Direction HUD, Toggle Sprint/Sneak, Zoom, FOV Changer, Fullbright, Freelook, **No View Bobbing**, No Hurt Cam, Hitbox, Hurt Color, Block Outline, Scoreboard-Tools, Tab List.
5. Danach Priorität 2, dann 3.

Pro Modul: Einstellungen im ClickGUI (viele Optionen), richtige Tags, Server-Regel-Eintrag in `servers/servers.json`, Eintrag in `Manager.cpp`. Fehlt eine Signatur, wird das Modul grau (nie ein Absturz).

Zählstand: aktuell 20 Module registriert. Ziel 135.

## So startet Felix die zweite Session

1. Auf GitHub im Repo `Lizziies/Mochi-3-Client`: Settings → Collaborators → "Add people" und den zweiten GitHub-Account einladen (oder das Repo bei dem Account forken). Der Account nimmt die Einladung an.
2. Auf dem zweiten Account unter claude.ai GitHub verbinden (`claude.ai/connect-github`) und die Claude GitHub App für das Repo zulassen.
3. Dort Claude Code im Web starten, Repo `Lizziies/Mochi-3-Client`, neue Branch.
4. Diesen Prompt einfügen:

> Du bist Session B im Projekt Mochi, einem kostenlosen Minecraft-Bedrock-PvP-Client. Lege als Erstes mit `git checkout -b claude/modules-b` eine neue Branch an und pushe nie auf eine andere. Lies dann `docs/TEAM.md`, dann `docs/MASTER.md`, `CLAUDE.md`, `docs/FEATURES.md`, `docs/CLIENTS.md`, `docs/MODULES.md`, `docs/STATUS.md`. Arbeite nur in deinem Bereich (Module), auf deiner eigenen Branch `claude/modules-b`, und pushe nur dorthin. Baue die fehlenden Module in der Reihenfolge aus `docs/TEAM.md`. Texte immer Englisch plus deutscher Eintrag (siehe Abschnitt Languages in CLAUDE.md). Es gibt hier kein Windows, also schreibe sauberen Code im Stil der vorhandenen Module (`dll/src/modules/hud/Fps.hpp` als Vorlage). Nach jedem Block `docs/STATUS.md` und `docs/HISTORY.md` ergänzen und committen. Arbeite selbstständig weiter, bis du nicht mehr sinnvoll weiterkommst. Stelle nur Rückfragen, wenn wirklich nötig.

5. Danach nicht mehr stören: die Session arbeitet allein weiter und schreibt Fortschritt in `docs/STATUS.md`.

## Wünsche an andere Sessions

Hier anhängen, was man von einem anderen Bereich braucht (Datum, Session, Wunsch).

- (Session A → B) Das Netzwerk-Modul braucht Server-Erkennung aus `dll/src/hook/Net.cpp`. Die Schnittstelle dort nutzen, nicht neu bauen.

- (Session A → alle) Das Client-Menü hat jetzt 4 Seiten (Modules, Appearance, Profiles, Settings) und Kategorie-Pillen. HUD-Module zeigen ihre Stil-Optionen (Hintergrund, Farben, Abstand, Größe) eingeklappt. `theme::setFade()` blendet alles Gezeichnete aus. Neue Module brauchen dafür nichts zu tun.
- (Session A → Felix) Auf GitHub fehlt ein Standardbranch `main`. Updater und Signatur-Loader lesen von dort. Vor dem Release die Arbeit nach `main` mergen.

- (Session A → B) **Verbindlicher Arbeitsplan:** `docs/PLAN_B.md`. Ehrliche Lage und Kriterien: `docs/PARITY.md`. Was Flarial wirklich kann: `docs/FLARIAL_REAL.md`.
