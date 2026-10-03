# Recherche und Wünsche

## Flarial-Quellbasis 2026-10-03

Auf ausdrücklichen Nutzerwunsch wurde der Modulcode aus `flarialmc/dll-oss` kopiert;
Commit und AGPL-3.0-Lizenz liegen in `vendor/flarial`. Der Nutzer möchte Monchis UI und
eigene Erweiterungen erhalten und die nativen Modul-Anbindungen aus Flarial adaptieren.

- NametagModifier öffnet eine sechs Byte lange Self-Tag-Branch. Für Monchis Standardmodus
  genügt die native Freigabe: kein eigener Text und keine globalen Farb-Overrides. Damit
  bleibt die Server-Formatierung in Minecrafts Renderer. Ein eigener Overlay-Stil ist optional.
- Freelook nutzt einen Drei-Argument-UpdatePlayer-Hook plus Yaw-Store-Patches. Der neue
  1.26-Head-Store enthält einen REX-Präfix und ist fünf Bytes lang. Der Port berücksichtigt
  diese Länge und rollt bei Teilfehlern zurück. Die Kamerarückgabe sichert zusätzlich die Winkel.
- Flarials RawInputBuffer ist in diesem Quellstand ein Platzhalter. Der Quellbestand allein
  ist kein Nachweis eines besseren Inputs oder vollständig funktionierender Version-Hooks.
- Flarials PingCounter fragt SDK::getLastPing/getServerPing ab; Monchis Ping Counter nutzt
  den separaten Probe-Dienst. Unterschiedliche Messquellen erklären unterschiedliche Werte;
  die Spiel-Ping-Anbindung bleibt eine spätere Aufgabe nach den Modulports.

Vollständiger Quellen-/Abhängigkeitsvergleich und offene Arbeiten: `docs/FLARIAL_PORT.md`.

Hier landen alle Infos zu anderen Clients und Wünsche von Felix, damit nichts verloren geht.

## Wünsche von Felix (UI)

- Scroll-Animationen und die Anordnung wie beim Onix Client: das wirkt frischer. Konkret nachbauen: weiches Scrollen mit Trägheit, Karten, die beim Scrollen leicht einblenden/gleiten, Hover-Lift, sanfte Panel-Übergänge.
- Girly UI, Pink, Herz-Logo, alles per Farbe anpassbar.
- Bestes Input, beste Animationen, beste Module. Alles muss maximal ausgebaut sein, die jetzigen 20 Module sind nur der Start.
- Alles Wichtige, was bei der Arbeit rauskommt, gehört in die Projektdateien (hier, STATUS.md, PLAN.md).

## Zahlen anderer Clients (Stand Okt 2026, Quellen: onixclient.com, flarial.xyz Vergleichsseite)

- Onix: über 90 eingebaute Module plus über 125 Community-Module (Scripting), Theme-Editor, FPS/Rendering-Optionen. Bezahlt/Patreon.
- Flarial: kostenlos, 140+ Module, Windows und Android, AGPL-3.0.
- Latite: kostenlos, Open Source, JS/TS-Plugin-System, nur Windows.
- Konsequenz für uns: Zielzahl 135 eigene Module reicht nur mit Tiefe. Ein Scripting-System (Phase 9) ist wichtig, weil Onix darüber 125 Community-Module bekommt.

## Nicht geschafft

- YouTube-Videos kann ich hier nicht ansehen und keine Bilder von Onix laden. Für den Design-Abgleich braucht es Screenshots oder kurze Beschreibungen von Felix, oder Claude Code am PC öffnet die Videos im Browser. Bis dahin wird nach den obigen Wünschen gebaut.

## Offene Design-Aufgaben nach Onix-Vorbild

- Smooth Scroll mit Trägheit im Kartenraster und in Panels.
- Karten: gestaffeltes Einblenden beim Öffnen und beim Kategoriewechsel.
- Panel-Übergang: Kreuzblende statt hartem Wechsel, Seiten gleiten seitlich.
- Toggle mit Federbewegung, Slider mit Wert-Bubble.
- Tooltip-Animationen.

## Weitere Wünsche (Session 2)

- No View Bobbing als eigenes Modul (Flarial hat nur Minimal und Java View Bobbing).
- Instant Hit/Input, so wenig Verzögerung wie möglich. Server-Ping ist nicht änderbar, nur lokal optimierbar und messbar.
- WLAN-Modul für Spieler im WLAN.
- Vorbild: Lunar Client (Java), nur für Bedrock.
- Details: `FEATURES.md`, Vergleich: `CLIENTS.md`.
