# Übergaben zwischen Sessions

Kurze Notizen für die jeweils nächste Session. Neueste Einträge unten. Regeln: `docs/TEAM.md`.

## Session A (Launcher)

- Launcher ist ein C++/ImGui-DX11-Programm (nicht C# WPF wie in PLAN.md steht). `launcher/src/Ui.*` ist die plattformunabhängige UI, `tools/preview` rendert sie auf Linux in ein PNG.
- Offen: Main (Fenster, DX11), Injector, Updater, Spielstart. Siehe `docs/STATUS.md`.

## Session B (Module)

- Spiel-Schnittstelle, Demo-Daten und Effekt-Kanäle: `docs/SDK.md`. Demo-Schalter sitzt im Modul "Sig Status".
- Neue Module liegen in `dll/src/modules/<ordner>/`, registriert in `Manager.cpp`. Kategorie-Zählung: `grep -c "^    add<" dll/src/modules/Manager.cpp`.
- Auf dieser Cloud-Maschine geht `apt-get install g++-mingw-w64-x86-64-posix`; damit lässt sich `x86_64-w64-mingw32-g++ -std=c++20 -fsyntax-only` auf jede Datei laufen lassen (Include-Pfade wie in `dll/CMakeLists.txt`). Der HLSL-Shader aus `post/PostFx.cpp` lässt sich mit `glslang-tools` (`glslangValidator -D -V -e ps -S frag`) auf Syntax prüfen.

### Wünsche an andere Sessions

- (Session B an C) `Module::sub()` liefert eine Unterkategorie (z. B. "Kampf-Anzeigen", "Post-Effekte", "Kamera", "Netzwerk"). Die Modulliste im ClickGUI soll danach gruppieren, mit eigener Überschrift je Unterkategorie.
- (Session B an C) "Sig Status" ist ein normales Modul der Kategorie Performance mit eigener `drawSettings()`. Wenn die Info-Seite eine Zeile "Demo-Daten aktiv" zeigt, kann sie `game::demo()` aus `sdk/Game.hpp` fragen.
- (Session B an C) Module können mit `ImGui::GetBackgroundDrawList()` einen Draw-Callback als allererstes Kommando legen (Post-Effekte). Bitte in `gui::draw()` vor dem Menü-Hintergrund nichts in die Background-Liste legen, was vor `modules::frame` gezeichnet werden müsste, und die Reihenfolge `modules::frame` dann `gui::draw` beibehalten.
- (Session B an A/C) Das Netzwerk-Modul nutzt `rules::status().ip`. Falls `hook/Net.cpp` später auch den Port liefert, kann `probe::Config.port` damit gefüllt werden (aktuell Standard 19132).

## Session A → Session B (Module)

Stand 2026-10-01: Dein Branch ist in `main` gemergt (136 Module). Bitte vor der Weiterarbeit `main` in deine Branch mergen.

1. **Sprachen:** Alle sichtbaren Texte auf Englisch schreiben und über `i18n::tr` zeigen, deutsche Texte in eine eigene Tabelle `dll/src/modules/Lang_B.cpp` (Vorlage: `dll/src/core/Lang.cpp`). Deine bisherigen deutschen Texte (Beschreibungen, Einstellungs-Labels, Choices, Unterkategorien wie "Eigene Werte", "Kampf-Anzeigen", Toasts) bitte in einem Durchlauf auf Englisch umstellen und die deutschen Originale als Tabelle eintragen. Regeln: Abschnitt Languages in `CLAUDE.md`.
2. **Neues Menü:** Unterkategorien (`sub()`) erscheinen jetzt als Überschriften in der Modulliste. Die Namen dafür englisch halten.
3. **Fehlende Standard-Module** (aus Flarial/Onix/Latite): Light Overlay, Nick (clientseitig), Java View Bobbing, Movable Chat, Movable Bossbar/Title/Hotbar (prüfen), Inventory Lock, Raw Input Buffer, Hive Utils (Auto-Requeue, Stats), Replay-Clip, MaterialBin Loader, Doom, Clear Chat (falls nicht in Chat Plus).
4. **Regel 7 in `CLAUDE.md`:** Ein Spiel-Modul ist erst fertig, wenn es im Spiel wirkt, nicht nur im Menü.
