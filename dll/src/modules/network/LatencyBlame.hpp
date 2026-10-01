#pragma once

#include "Probe.hpp"
#include "gui/Theme.hpp"
#include "hook/Dx.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "render/Fonts.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <format>

class LatencyBlame : public HudModule {
public:
    LatencyBlame()
        : HudModule("Latency Blame",
                    "Teilt die Verzögerung in Eingabe, Bild, Netzwerk und Server-Tick auf.",
                    {"hud-self"}, {0.35f, 0.02f}) {
        sub("Netzwerk");
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        qpf_ = double(f.QuadPart);
        tick_.visible = [this] { return serverTick_.b; };
    }

    void onEnable() override { probe::use(true); }
    void onDisable() override { probe::use(false); }

    void onFrame() override {
        auto& fi = dx::frame();
        int64_t click = input::lastClickQpc();
        if (!click || click == seen_ || fi.presentQpc <= click) return;
        seen_ = click;
        double ms = double(fi.presentQpc - click) * 1000.0 / qpf_;
        if (ms >= 250) return;
        ring_[head_] = (float)ms;
        head_ = (head_ + 1) % ring_.size();
        count_ = std::min(count_ + 1, ring_.size());
    }

protected:
    struct Part {
        const char* name;
        float ms;
        ImVec4 color;
    };

    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& t = theme::current();
        auto snap = probe::snapshot();
        auto& fi = dx::frame();

        float sample = 0.f;
        for (size_t i = 0; i < count_; i++) sample += ring_[i];
        sample = count_ ? sample / count_ : 0.f;

        float frame = (float)fi.frameMs;
        float input = std::max(0.f, sample - frame);
        float render = std::min(sample, frame);
        float network = (snap.received > 0 ? snap.avg : 0.f) * (pingMode_.i == 0 ? 1.f : 0.5f);
        float server = serverTick_.b ? tick_.f * 0.5f : 0.f;

        std::array<Part, 4> parts{{
            {"Eingabe", input, inputColor_.color},
            {"Bild", render, renderColor_.color},
            {"Netzwerk", network, netColor_.color},
            {"Server", server, serverColor_.color},
        }};
        float total = 0.f;
        for (auto& p : parts) total += p.ms;

        float lineH = fonts::hudSize() * s * 1.1f;
        float width = 230 * s, y = 0;

        auto head = count_ ? std::format("Gesamt ca. {:.0f} ms bis zum Treffer", total) : "Klick mal, dann messe ich";
        y += drawText(dl, o, s, head, textColor()).y;

        float barH = 10 * s;
        ImVec2 b0 = o + ImVec2(0, y + 3 * s);
        dl->AddRectFilled(b0, b0 + ImVec2(width, barH), IM_COL32(0, 0, 0, 70), barH * 0.5f);
        if (total > 0.f) {
            float x = 0.f;
            for (auto& p : parts) {
                float w = width * p.ms / total;
                if (w < 0.5f) continue;
                dl->AddRectFilled(b0 + ImVec2(x, 0), b0 + ImVec2(x + w, barH), ImGui::GetColorU32(p.color), x == 0.f ? barH * 0.5f : 0.f);
                x += w;
            }
        }
        y += barH + 8 * s;

        float biggest = 0.f;
        const Part* worst = nullptr;
        for (auto& p : parts) {
            if (p.ms <= 0.f) continue;
            if (p.ms > biggest) {
                biggest = p.ms;
                worst = &p;
            }
            dl->AddCircleFilled(o + ImVec2(5 * s, y + lineH * 0.5f), 4 * s, ImGui::GetColorU32(p.color));
            drawText(dl, o + ImVec2(14 * s, y), s, std::format("{}  {:.1f} ms", p.name, p.ms), textColor());
            y += lineH;
        }
        if (verdict_.b && worst && total > 0.f)
            y += drawText(dl, o + ImVec2(0, y + 2 * s), s, std::format("Größter Anteil: {} ({:.0f} %)", worst->name, 100.f * biggest / total),
                          ImGui::GetColorU32(t.textDim)).y;
        return {width, y};
    }

private:
    Setting& pingMode_ = choice("pingMode", "Netzwerk-Anteil", {"Volle Rundlaufzeit", "Halbe Rundlaufzeit (nur Hinweg)"}, 0);
    Setting& serverTick_ = toggleSetting("serverTick", "Server-Tick einrechnen", true);
    Setting& tick_ = slider("tick", "Tick-Raster (ms)", 50.f, 25.f, 100.f, "%.0f");
    Setting& verdict_ = toggleSetting("verdict", "Größten Anteil nennen", true);
    Setting& inputColor_ = colorSetting("inputColor", "Farbe Eingabe", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& renderColor_ = colorSetting("renderColor", "Farbe Bild", {1.f, 0.49f, 0.71f, 1.f});
    Setting& netColor_ = colorSetting("netColor", "Farbe Netzwerk", {0.71f, 0.61f, 1.f, 1.f});
    Setting& serverColor_ = colorSetting("serverColor", "Farbe Server", {1.f, 0.82f, 0.49f, 1.f});
    std::array<float, 20> ring_{};
    size_t head_ = 0;
    size_t count_ = 0;
    int64_t seen_ = 0;
    double qpf_ = 1;
};
