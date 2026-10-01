# Mochi Online: Nutzer erkennen, Tags, Namensfarben, Cosmetics für alle

Ziel: Jeder Spieler, der Mochi benutzt, ist für andere Mochi-Nutzer erkennbar und sichtbar. Rotes Herz vor dem Namen in der Tab-Liste, Tag im Chat, eigene Namensfarbe oder Regenbogen, und später Cosmetics an der Figur. Mockup: `docs/ui_proposals/l_players.png`.

## Warum es einen Server braucht

Der Client kann von außen nicht wissen, ob ein anderer Spieler Mochi benutzt. Dazu müsste er Daten in Minecraft-Pakete schreiben, und das verbietet unsere Regel 2. Deshalb gibt es einen kleinen eigenen Dienst, mit dem alle Mochi-Clients getrennt von Minecraft reden (HTTPS). So machen es Flarial und Onix auch.

Ablauf:

1. Beim Betreten eines Servers meldet der Client: Spielername (Gamertag), Servername. Außerdem Namensstil und ausgerüstete Cosmetics.
2. Alle paar Sekunden fragt der Client: "Welche dieser Spielernamen aus meiner Tab-Liste sind Mochi-Nutzer, und wie sehen sie aus?" Die Antwort enthält Stil und Cosmetics.
3. Der Client zeichnet das Ergebnis in seine eigenen Anzeigen: Tab-Liste (Herz, Farbe), Better Chat (Tag, Farbe), später Spielfiguren (Cosmetics).

Es gehen keine zusätzlichen Pakete an den Minecraft-Server. Nicht-Mochi-Spieler sehen nichts davon.

## Was Nutzer einstellen können

- Namensfarbe: einfarbig, Verlauf, Regenbogen (langsam, mittel, schnell), Puls.
- Tag-Text und Tag-Farbe (mit Filter für Beleidigungen).
- Rotes Herz vor dem Namen an/aus.
- Ausgerüstete Cosmetics (Flügel, Cape, Kopf, Rücken ...).
- Sichtbarkeit: "Für andere Mochi-Nutzer sichtbar" an/aus (Standard: an, steht beim ersten Start gut sichtbar zur Wahl).

## Die Schnittstelle (Entwurf)

Basis-URL ist im Client einstellbar (`online.url`), damit wir den Dienst ohne neue Version umziehen können.

| Aufruf | Zweck |
|---|---|
| `POST /v1/hello` | Anmelden: Gamertag, Client-Version, Stil, Cosmetics. Antwort: Sitzungs-Token |
| `POST /v1/presence` | Server und Welt melden (alle 60 s, kurzer Herzschlag) |
| `POST /v1/lookup` | Liste von Gamertags rein, Stil und Cosmetics der Mochi-Nutzer raus (höchstens 100, kurz zwischengespeichert) |
| `POST /v1/profile` | Eigenen Stil und eigene Cosmetics speichern |
| `POST /v1/bye` | Abmelden |

Antworten sind klein (unter 10 KB), der Client fragt höchstens alle 5 Sekunden und nur bei Änderungen der Spielerliste.

## Technik und Kosten

Cloudflare Workers mit D1 (Datenbank) und KV (Zwischenspeicher). Der kostenlose Tarif reicht für den Start. Kein eigener Server, keine laufenden Kosten, bis viele tausend gleichzeitige Nutzer da sind. Der Code kommt nach `server/` ins Repo (JavaScript, ohne Abhängigkeiten) und lässt sich lokal testen. Ein Konto bei Cloudflare muss Felix anlegen und den Dienst veröffentlichen (ich habe von hier aus keinen Zugang).

## Ehrliche Risiken

- **Namens-Diebstahl:** Ohne Prüfung könnte jemand behaupten, "Luna" zu sein, und deren Stil überschreiben. Gegenmaßnahme erste Version: Der Dienst gibt jedem Gamertag nur einen Sitzungs-Token und akzeptiert nur Änderungen mit dem Token. Zweite Version: Beweis über den Xbox-Login (XSTS-Token), der im Spiel schon existiert. Das muss am PC erforscht werden.
- **Datenschutz:** Der Dienst speichert nur Gamertag, Servername, Stil, Cosmetics, letzten Kontakt. Nichts über Chat, Standort, IP-Dauerspeicherung oder Welten. Vor dem ersten Senden fragt der Client beim ersten Start, löschen geht mit einem Knopf. Eine Datenschutz-Seite muss vor dem Release stehen.
- **Missbrauch:** Tag-Texte filtern, melden, Sperrliste. Die Anzahl der Aufrufe pro Nutzer begrenzen.
- **Server-Regeln:** Manche Server mögen keine Fremd-Clients. Der Dienst meldet keine Serverdaten, wenn die Server-Regeln es für diesen Server sperren (`servers.json`, neues Feld `online: false`).
- **Cosmetics an fremden Figuren:** Wir sehen sie nur, wenn der Client Position und Haltung fremder Spieler lesen kann (Signaturen). Bis dahin zeigen wir Herz, Farbe und Tag in Tab-Liste und Chat.

## Reihenfolge

1. Client-Teil mit Platzhalter-Dienst im Client (Demo-Daten): Tab-Liste mit Herz und Farben, Chat-Tags, Namensstil-Einstellungen. Das ist ohne Server testbar.
2. `server/` bauen und lokal testen.
3. Client mit dem Dienst verbinden (`online.url`), Datenschutz-Hinweis beim ersten Start.
4. Veröffentlichen (Felix), Xbox-Beweis erforschen.
5. Cosmetics an fremden Figuren (braucht Signaturen).
