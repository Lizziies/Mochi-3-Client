# Server-Regeln

Beim Verbinden erkennt der Client den Server und wendet dessen Regeln an. Dabei gibt es drei Stufen:

| Stufe | Verhalten |
|---|---|
| `block` | Modul wird grau und lässt sich auf diesem Server nicht einschalten. Nur für Module, die der Server **ausdrücklich** verbietet und die sicher zum Bann führen. |
| `warn` | Modul bleibt nutzbar, bekommt aber ein gelbes Dreieck mit Hinweis ("Nicht auf der erlaubten Liste von Lifeboat – auf eigenes Risiko"). |
| erlaubt | normal |

Grundsatz: so wenig grau wie möglich. Bei Flarial und anderen Clients sind auf diesen Servern auch alle Features nutzbar, sonst wäre der Client dort unbrauchbar. Hart gesperrt wird nur das, was eindeutig verboten ist.

Beim Verlassen des Servers wird alles wieder freigegeben und der vorherige Zustand wiederhergestellt. Die Regeln liegen als `servers/servers.json` auf GitHub und werden beim Start nachgeladen, Änderungen brauchen also kein neues Release.

## Server-Erkennung

Versionsunabhängig, ohne Spielsignaturen:
- Hooks auf `getaddrinfo` / `GetAddrInfoW` / `GetAddrInfoExW` merken sich Hostname → IP.
- Hooks auf `sendto` / `WSASendTo` sehen die Ziel-IP des RakNet-Verkehrs.
- Erste Ziel-IP mit Port ≠ 53 nach Verbindungsaufbau = aktueller Server, Hostname aus der Zuordnung, Abgleich mit `match` (inkl. Subdomains).
- Kein Verkehr für 5 s → Server verlassen.

## Status im ClickGUI

Oben: "The Hive · 2 gesperrt · 7 Hinweise". Klick zeigt die Liste mit Begründung und Link zur Regelseite.

## Server (Stand Oktober 2026)

### Zeqa
Alles erlaubt, was ein PvP-Client mitbringt. Keine Sperren, keine Hinweise.

### The Hive
Quelle: support.playhive.com/allowed-and-denied-mods
- Ausdrücklich erlaubt: Toggle Sprint/Sneak, Fullbright, FOV Changer / Zoom, Hitbox (nicht durch Wände), Cosmetics, CPS-Anzeige, Armor HUD, Auto GG am Spielende.
- Ausdrücklich verboten: Freelook / 360°-Perspektive, Minimaps/Radar, Macros, Autoclicker, Sprint in alle Richtungen, Chat-Nachricht beim Kill, schnelles Kisten-Looten.
- `block`: Freelook, Auto GG Option "beim Kill"
- `warn`: Entity Counter, Null Movement, Faster Inventory, Item Use Delay Fix, Insta Hurt Animation, Opponent Reach, Hit Ping

### CubeCraft
Quelle: cubecraft.net "Allowed Mods and Clients", Stand 23.09.2026. Onix, Latite und Flarial stehen auf der Liste erlaubter Clients, Freelook ist dort erlaubt.
- Ausdrücklich verboten: FreeCam, Shoulder Surfing, Inventory Tweaks, Mouse Wheelie, Fastbreak, FastPlace, AutoWalk, Better Name Visibility, Skin-Blinker.
- `block`: keins (wir haben keins der verbotenen Features)
- `warn`: Faster Inventory, Java Inventory Hotkeys, Item Use Delay Fix, Null Movement, Insta Hurt Animation
- Nach Release: Freigabe für Mochi im CubeCraft-Forum beantragen.

### Lifeboat
Quelle: lbsg.net/in-game-rules. Erlaubt sind offiziell nur Fullbright, Zoom/FOV, CPS- und Reach-Anzeige, Hitbox, Armor HUD und Mods aus Lunar, Onix und Astral.
- `block`: Null Movement, Faster Inventory, Item Use Delay Fix, Insta Hurt Animation (ändern Timing/Eingabe, klar nicht erlaubt)
- `warn`: alle Module mit Tag `info-others`, `input`, `timing`, `chat`
- Hinweis-Banner: "Mochi ist auf Lifeboat noch nicht offiziell freigegeben."

### Galaxite
Quelle: galaxite.net/rules. Mods mit Vorteil = permanenter Bann, keine Liste.
- `block`: Null Movement, Faster Inventory, Item Use Delay Fix, Insta Hurt Animation
- `warn`: Tags `info-others`, `timing`, `input`

### NetherGames, Mineville, andere
Regeln noch nicht recherchiert. Keine Sperren. `server-rules`-Module bleiben auf unbekannten Servern standardmäßig aus.

## Modul-Tags

| Tag | Bedeutung | Beispiele |
|---|---|---|
| `cosmetic` | rein optisch | Themes, Glint Color, Swing Animations, Custom Crosshair |
| `hud-self` | zeigt eigene Werte | FPS, CPS, Keystrokes, Armor HUD, Coordinates |
| `info-others` | Infos über andere | Opponent Reach, Hitbox, Player Notifier, Entity Counter |
| `camera` | Kamera/Sicht | Zoom, FOV Changer, Freelook, Fullbright |
| `input` | Eingabeverarbeitung | Toggle Sprint/Sneak, Null Movement, Raw Input Buffer |
| `timing` | Spiel-Timing | Faster Inventory, Item Use Delay Fix, Insta Hurt Animation |
| `chat` | sendet Nachrichten | Auto GG, Command/Text Hotkey |

Format: siehe `servers/servers.json`.
