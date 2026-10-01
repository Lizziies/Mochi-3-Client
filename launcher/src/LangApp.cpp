#include "I18n.hpp"

namespace {

const i18n::Entry entries[] = {
    {"Checking for updates", "Suche nach Updates"},
    {"Downloading {}", "Lade {} herunter"},
    {"Connecting the client", "Verbinde den Client"},
    {"Waiting for the game to load", "Warte, bis das Spiel geladen ist"},
    {"Mochi is connected. Have fun!", "Mochi ist verbunden. Viel Spaß!"},
    {"Updating the launcher", "Aktualisiere den Launcher"},
    {"The release is incomplete", "Das Release ist unvollständig"},
    {"Download failed", "Download fehlgeschlagen"},
    {"Checksum does not match", "Prüfsumme stimmt nicht überein"},
    {"Close Minecraft to update the client", "Schließe Minecraft, um den Client zu aktualisieren"},
    {"Could not replace the launcher", "Der Launcher konnte nicht ersetzt werden"},
    {"The client file is missing. Put Mochi.dll next to the launcher.", "Die Client-Datei fehlt. Lege Mochi.dll neben den Launcher."},
    {"Could not open the Minecraft process", "Der Minecraft-Prozess konnte nicht geöffnet werden"},
    {"Minecraft refused the client", "Minecraft hat den Client abgelehnt"},
    {"Minecraft could not be started", "Minecraft konnte nicht gestartet werden"},
    {"Minecraft did not start", "Minecraft wurde nicht gestartet"},
    {"Minecraft closed before it was ready", "Minecraft wurde geschlossen, bevor es bereit war"},
    {"No release notes yet.", "Noch keine Release-Notizen."},
    {"Downloading LeviLauncher", "Lade LeviLauncher herunter"},
    {"The downloaded file is not valid", "Die heruntergeladene Datei ist ungültig"},
    {"Could not save LeviLauncher", "LeviLauncher konnte nicht gespeichert werden"},
};

i18n::Table table(entries);

}
