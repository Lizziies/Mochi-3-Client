#pragma once

#include "Probe.hpp"

#include <format>
#include <string>
#include <vector>

namespace quality {

enum class Grade { Unknown, Good, Fair, Poor };

struct Limits {
    float pingFair = 60.f;
    float pingPoor = 150.f;
    float jitterFair = 6.f;
    float jitterPoor = 20.f;
    float lossPoor = 3.f;
};

struct Verdict {
    Grade grade = Grade::Unknown;
    std::string label = "Keine Messung";
    std::string reason;
    std::vector<std::string> tips;
};

inline Verdict judge(const probe::Snapshot& s, const Limits& l) {
    Verdict v;
    if (!s.running || !s.resolved || s.received == 0) {
        v.reason = s.sent > 0 ? "Keine Antwort vom Server. Blockiert die Firewall ICMP?" : "";
        return v;
    }

    bool wifi = s.link.kind == probe::LinkKind::Wifi;
    bool weakWifi = wifi && s.link.signal >= 0 && s.link.signal < 50;
    bool lossy = s.loss >= l.lossPoor;
    bool jittery = s.jitter >= l.jitterPoor;
    bool slow = s.avg >= l.pingPoor;

    if (lossy || jittery || slow) v.grade = Grade::Poor;
    else if (s.avg >= l.pingFair || s.jitter >= l.jitterFair || s.loss >= 0.5f) v.grade = Grade::Fair;
    else v.grade = Grade::Good;
    v.label = v.grade == Grade::Good ? "Stabil" : v.grade == Grade::Fair ? "Wackelig" : "Schlecht";

    if (v.grade != Grade::Good) {
        if (weakWifi) v.reason = std::format("WLAN-Signal schwach ({} %)", s.link.signal);
        else if (lossy && wifi) v.reason = std::format("{:.1f} % Paketverlust, vermutlich Funk-Störung", s.loss);
        else if (lossy) v.reason = std::format("{:.1f} % Paketverlust auf der Strecke", s.loss);
        else if (jittery && wifi) v.reason = "Jitter hoch, vermutlich Funk-Störung";
        else if (jittery) v.reason = "Jitter hoch, die Leitung schwankt";
        else if (slow) v.reason = std::format("Ping hoch ({:.0f} ms), Server weit weg oder Route schlecht", s.avg);
        else v.reason = "Leichte Schwankungen";
    }
    if (s.spikePeriod > 0.f)
        v.reason = std::format("Ping-Spitzen alle {:.0f} s, vermutlich WLAN-Scan im Hintergrund", s.spikePeriod);

    if (wifi) {
        if (s.link.band == "2,4 GHz") v.tips.push_back("Auf das 5-GHz-Band wechseln, 2,4 GHz ist überlaufen");
        if (weakWifi) v.tips.push_back("Näher an den Router oder LAN-Kabel nutzen");
        if (v.grade != Grade::Good) v.tips.push_back("LAN-Kabel ist die sicherste Lösung gegen Jitter");
        if (s.link.powerSaving == 1) v.tips.push_back("Energiesparen des WLAN-Adapters im Geräte-Manager abschalten");
        if (s.spikePeriod > 0.f) v.tips.push_back("Automatische WLAN-Suche des Treibers abschalten");
    } else if (v.grade != Grade::Good) {
        v.tips.push_back("Hintergrund-Downloads und Cloud-Sync pausieren");
        if (lossy) v.tips.push_back("Kabel und Router prüfen, Router-QoS für Spiele aktivieren");
    }
    return v;
}

}
