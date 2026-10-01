# Versionen, Updates, Downgrades

## Problem

Jedes Minecraft-Update verschiebt Funktionen in der Exe. Signaturen (Byte-Muster) und Offsets (Feldpositionen in Klassen) brechen. Flarial pflegt dafür eine `SigInit::initXXXX()`-Funktion pro Version im Code. Jedes Update braucht einen neuen DLL-Build.

## Unsere Lösung

### 1. Versionserkennung
Beim Start Version aus der Exe-Ressource (`VS_FIXEDFILEINFO`) lesen, zusätzlich SHA256 der Exe. Beides ins Log und in den Sig-Status.

### 2. sigs.json statt Code
```json
{
  "version": "1.26.30",
  "inherits": "1.26.20",
  "sigs": {
    "ClientInstance::update": { "pattern": "48 89 5C 24 ? 57 48 83 EC 20 ...", "rel": "call" },
    "Options::getFov": { "pattern": "...", "rel": "none" }
  },
  "offsets": {
    "Player::gamemode": 3112
  }
}
```
- Eine Datei pro Version in `sigs/`. Neue Versionen erben von der vorigen und überschreiben nur, was sich geändert hat.
- `rel`: wie die Adresse aufgelöst wird (`none`, `call`, `lea`, `mov` → RIP-relativ).
- Mehrere Patterns pro Eintrag erlaubt (Fallbacks), das erste mit genau einem Treffer gewinnt.

### 3. Laden
1. Mitgelieferte `sigs/` im DLL-Ordner.
2. Cache `%LOCALAPPDATA%\Mochi\cache\sigs\`.
3. GitHub raw (`https://raw.githubusercontent.com/<user>/<repo>/main/sigs/<version>.json`), wenn neuer, mit SHA256-Prüfung gegen eine signierte Indexdatei.

Ein Sig-Fix ist damit ein Commit in `sigs/`. Alle Spieler bekommen ihn beim nächsten Start, ohne neues Release.

### 4. Unbekannte Version
- Kein exakter Eintrag: die neueste ältere Version nehmen und **jede Sig scannen**. Was genau einmal trifft, wird verwendet.
- Module, deren Sigs fehlen, werden grau ("nicht verfügbar auf 1.26.40"). Overlay-Module laufen immer.
- Gescannte Adressen pro Exe-Hash cachen, damit der Start schnell bleibt.

### 5. Module deklarieren Abhängigkeiten
```cpp
Zoom() : Module("Zoom", {"Options::getFov", "LevelRenderer::renderLevel"}) {}
```
Der Modul-Manager prüft beim Start und sperrt das Modul, wenn etwas fehlt.

### 6. sigcheck-Tool
`tools/sigcheck <Minecraft.Windows.exe> sigs/1.26.30.json`. Gibt pro Sig OK / 0 Treffer / mehrere Treffer aus. Ablauf bei neuem MC-Update:
1. Exe kopieren, `sigcheck` laufen lassen.
2. Kaputte Sigs mit Claude Code + Ghidra headless neu finden.
3. Neue `sigs/<version>.json` committen. Fertig.

### 7. Downgrades
Weil alle alten Versionen in `sigs/` bleiben und sich vererben, funktioniert der Client auf jeder Version, für die je eine Datei existiert. Ziel: Unterstützung ab 1.21.0 bis aktuell.

## Version-Switcher (Launcher, Phase 8)

- Bedrock für Windows läuft seit 1.21.120 als GDK-Build, nicht mehr als UWP. Alte Tools wie MCLauncher sind dafür nicht mehr geeignet. Aetopia/MCBE.GDK.Switcher ist eingestellt und verweist auf LeviLauncher.
- Pfade unterscheiden sich: UWP-Daten unter `%LocalAppData%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\LocalState\games\com.mojang`, GDK unter `%AppData%\Minecraft Bedrock\Users\<id>\games\com.mojang`, Spieldateien z. B. `C:\XboxGames\Minecraft for Windows\Content`. Module, die `options.txt` lesen, müssen beide Pfade kennen.
- Vor dem Bau: LeviLauncher (LiteLDev) anschauen, wie Versionen bezogen und installiert werden, und die Lizenz prüfen. Eigene Implementierung schreiben.
- Anforderungen:
  - Nur mit eigener Spiellizenz (Microsoft-Login/Xbox-App). Keine Spieldateien auf GitHub oder eigenen Servern.
  - Jede Version in eigenem Ordner, optional getrennte Welten/Configs (Isolation).
  - Gepinnte Versionen vor Zwangs-Updates schützen.
  - Release und Preview.
  - Im Launcher pro Version anzeigen, ob Mochi sie unterstützt (gibt es `sigs/<version>.json`?).

## Auto-Update des Clients

- Launcher fragt beim Start `GET https://api.github.com/repos/<user>/<repo>/releases/latest`.
- Neuer Tag → Assets `Mochi.dll`, `MochiLauncher.exe`, `checksums.txt` laden, SHA256 prüfen.
- DLL ersetzen. Launcher: neue Exe als `MochiLauncher.new.exe` speichern, neu starten, die alte im neuen Prozess löschen.
- Kanal-Wahl: Stable / Beta (Pre-Releases).
- Die DLL prüft zusätzlich nur `sigs/` (siehe oben), lädt aber nie selbst Code nach.
