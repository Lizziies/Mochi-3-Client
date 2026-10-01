#include "I18n.hpp"

namespace {

const i18n::Entry entries[] = {
    {"Info displays", "Info-Anzeigen"},
    {"Client Settings", "Client-Einstellungen"},
    {"Name tag behind your name in chat, notifications and other global options.", "Namens-Tag hinter deinem Namen im Chat, Benachrichtigungen und weitere globale Optionen."},
    {"Client tag", "Client-Tag"},
    {"Shown behind your own name in Better Chat and in the Tab List. Only you see it, nothing is sent to the server.", "Wird hinter deinem eigenen Namen im Better Chat und in der Tab-Liste angezeigt. Nur du siehst ihn, es wird nichts an den Server gesendet."},
    {"Client tag behind my name", "Client-Tag hinter meinem Namen"},
    {"Tag text", "Tag-Text"},
    {"Tag color", "Tag-Farbe"},
    {"Tag position", "Tag-Position"},
    {"Inside the name brackets", "In den Namens-Klammern"},
    {"Before the message", "Vor der Nachricht"},
    {"Tag in brackets", "Tag in Klammern"},
    {"Also in the Tab List", "Auch in der Tab-Liste"},
    {"Notifications", "Benachrichtigungen"},
};

i18n::Table table(entries);

}
