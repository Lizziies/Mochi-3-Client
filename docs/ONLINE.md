# Mochi Online: Nutzer erkennen, Tags, Namensfarben, Cosmetics für alle

Ziel: Jeder Spieler, der Mochi benutzt, ist für andere Mochi-Nutzer erkennbar und sichtbar. Mochi-Logo hinter dem Namen in der Tab-Liste und im Chat, eigene Namensfarbe oder Regenbogen, und später Cosmetics an der Figur. Mockup: `docs/ui_proposals/l_players.png`.

## Warum es einen Server braucht

Der Client kann von außen nicht wissen, ob ein anderer Spieler Mochi benutzt. Dazu müsste er Daten in Minecraft-Pakete schreiben, und das verbietet unsere Regel 2. Deshalb gibt es einen kleinen eigenen Dienst, mit dem alle Mochi-Clients getrennt von Minecraft reden (HTTPS). So machen es Flarial und Onix auch.

Ablauf:

1. Beim Betreten eines Servers meldet der Client: Spielername (Gamertag), Servername. Außerdem Namensstil und ausgerüstete Cosmetics.
2. Alle paar Sekunden fragt der Client: "Welche dieser Spielernamen aus meiner Tab-Liste sind Mochi-Nutzer, und wie sehen sie aus?" Die Antwort enthält Stil und Cosmetics.
3. Der Client zeichnet das Ergebnis in seine eigenen Anzeigen: Tab-Liste (Herz, Farbe), Better Chat (Herz, Farbe), später Spielfiguren (Cosmetics).

Es gehen keine zusätzlichen Pakete an den Minecraft-Server. Nicht-Mochi-Spieler sehen nichts davon.

## Was Nutzer einstellen können

- Namensfarbe: nur der echte Xbox-Name wird eingefärbt (einfarbig, Verlauf, Regenbogen, Puls). Man kann keinen anderen Namen dazuschreiben, es gibt keinen freien Text.
- Das Mochi-Logo (Herz) steht fest hinter dem Xbox-Namen. Es lässt sich nur ein- und ausschalten und in der Farbe wählen.
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

Cloudflare Workers mit D1 (Datenbank). Der kostenlose Tarif reicht für den Start. Kein eigener Server, keine laufenden Kosten, bis viele tausend gleichzeitige Nutzer da sind. Der Code liegt in `server/` (JavaScript, ohne Abhängigkeiten) und lässt sich lokal testen, siehe `server/README.md`. Ein Konto bei Cloudflare muss Felix anlegen und den Dienst veröffentlichen (ich habe von hier aus keinen Zugang).

## Ehrliche Risiken

- **Namens-Diebstahl:** Ohne Prüfung könnte jemand behaupten, "Luna" zu sein, und deren Stil überschreiben. Gegenmaßnahme erste Version: Der Dienst gibt jedem Gamertag nur einen Sitzungs-Token und akzeptiert nur Änderungen mit dem Token. Zweite Version: Beweis über den Xbox-Login (XSTS-Token), der im Spiel schon existiert. Das muss am PC erforscht werden.
- **Datenschutz:** Der Dienst speichert nur Gamertag, Servername, Stil, Cosmetics, letzten Kontakt. Nichts über Chat, Standort, IP-Dauerspeicherung oder Welten. Vor dem ersten Senden fragt der Client beim ersten Start, löschen geht mit einem Knopf. Eine Datenschutz-Seite muss vor dem Release stehen.
- **Missbrauch:** Es gibt keine freien Texte, nur Farben und feste Logos, deshalb kein Filter nötig. Aufrufe pro Nutzer begrenzen, Sperrliste für Namens-Diebstahl.
- **Server-Regeln:** Manche Server mögen keine Fremd-Clients. Der Dienst meldet keine Serverdaten, wenn die Server-Regeln es für diesen Server sperren: dafür gibt es keinen neuen Mechanismus, das Modul "Mochi Online" kommt einfach in die `block`-Liste des Servers in `servers.json`. Dann lässt es sich dort nicht einschalten.
- **Cosmetics an fremden Figuren:** Wir sehen sie nur, wenn der Client Position und Haltung fremder Spieler lesen kann (Signaturen). Bis dahin zeigen wir Herz und Farbe in Tab-Liste und Chat.

## Stand

| Schritt | Stand |
|---|---|
| 1. Client mit Demo-Daten | gebaut: Modul "Mochi Online" (`dll/src/modules/online/`), Herz hinter dem Namen (Farbe wählbar) und Namensfarbe (einfarbig, Verlauf, Regenbogen, Puls) in Tab-Liste und Better Chat, ausgerüstete Cosmetics gehen als `worn` mit, Einstellungen für alles, erfundene Nutzer ohne Netzwerk |
| 2. `server/` bauen und lokal testen | gebaut: Worker mit D1, 17 Tests laufen gegen den Speicher und gegen die echte SQL-Datenbank (`node:sqlite`) |
| 3. Client mit dem Dienst verbinden | gebaut und unter Wine gegen den lokalen Server geprüft: hello, presence, lookup, profile, bye, forget. Hinweis beim ersten Einschalten, Knopf "Meine Daten im Dienst löschen" |
| 4. Veröffentlichen | **offen, Felix:** Cloudflare-Konto, `wrangler deploy`, Adresse in den Client eintragen. Datenschutz-Seite schreiben. Xbox-Beweis erforschen |
| 5. Cosmetics an fremden Figuren | offen, braucht Signaturen. Das Profil trägt die ausgerüsteten Cosmetics schon (`worn`), siehe `docs/COSMETICS.md` |

Entscheidungen beim Bauen:

- **Namens-Diebstahl, erste Version:** Jede Installation erzeugt einmal ein zufälliges Geheimnis (`online.key`). Wer einen Gamertag zuerst benutzt, besitzt ihn. Andere bekommen 403, bis er 30 Tage still war. Das ist nur ein Anfang, der Xbox-Beweis ersetzt es.
- **Einwilligung:** Das Modul ist standardmäßig aus. Wer es einschaltet, stimmt zu und bekommt dabei eine Meldung, was gesendet wird. "Für andere sichtbar" ist getrennt einstellbar: wer es ausschaltet, sieht andere, wird aber selbst nicht aufgelistet.
- **Ohne Adresse:** Im Demo-Modus zeigt der Client erfundene Nutzer. Ohne Demo und ohne Adresse passiert nichts und das Modul sagt es.
- **Kein freier Text:** Der Dienst kennt nur Modus, Farben, Tempo, Herz an/aus und Herzfarbe. Alles andere im Profil wird verworfen.

## Verbrauch (Gratis-Tarif)

Der Client fragt sparsam: Herzschlag alle 2 Minuten, Nachschlagen nur für Namen, die er noch nicht kennt (Mochi-Nutzer werden alle 2 Minuten, Nicht-Nutzer alle 5 Minuten neu gefragt, höchstens alle 15 Sekunden). Pro Spieler und Stunde sind das grob 30 bis 100 Aufrufe. Mit 100 Spielern, die je 2 Stunden am Tag spielen, sind das etwa 6.000 bis 20.000 Aufrufe und 12.000 geschriebene Zeilen pro Tag, das Gratis-Limit von Cloudflare (100.000 Aufrufe, 100.000 Schreibzugriffe) reicht grob bis 500 Spieler.
