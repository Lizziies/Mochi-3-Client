#pragma once

#include "core/Build.hpp"
#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "hook/Dx.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "modules/network/Probe.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"
#include "server/Rules.hpp"
#include "sig/Sigs.hpp"

#include <windows.h>
#include <psapi.h>

#include <format>

class StatsHud : public HudModule {
public:
    StatsHud()
        : HudModule("Stats HUD", "One block with selectable lines: FPS, CPS, Ping, position, health, clock, RAM.",
                    {"hud-self"}, {0.01f, 0.36f}) {
        sub("Info displays");
    }

    void onDisable() override {
        if (probeOn_) probe::use(false);
        probeOn_ = false;
    }

    void onFrame() override {
        if (ping_.b != probeOn_) {
            probe::use(ping_.b);
            probeOn_ = ping_.b;
        }
        double ms = dx::frame().frameMs;
        if (ms > 0.0) frameMs_ += (ms - frameMs_) * 0.05;
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& st = game::state();
        struct R {
            std::string k, v;
        };
        std::vector<R> rows;
        if (fps_.b) rows.push_back({"FPS", std::format("{:.0f}", frameMs_ > 0 ? 1000.0 / frameMs_ : 0.0)});
        if (frametime_.b) rows.push_back({"Frametime", std::format("{:.1f} ms", frameMs_)});
        if (cps_.b) rows.push_back({"CPS", std::format("{} | {}", input::cps(MouseButton::Left), input::cps(MouseButton::Right))});
        if (ping_.b) {
            auto p = probe::snapshot();
            rows.push_back({"Ping", p.received ? std::format("{:.0f} ms", p.avg) : "–"});
            if (jitter_.b && p.received) rows.push_back({"Jitter", std::format("{:.1f} ms", p.jitter)});
        }
        if (game::has(game::Domain::Player)) {
            auto& pl = st.player;
            if (coords_.b) rows.push_back({"XYZ", std::format("{:.0f} {:.0f} {:.0f}", pl.pos.x, pl.pos.y, pl.pos.z)});
            if (health_.b) rows.push_back({"Health", std::format("{:.1f}", pl.health)});
            if (hunger_.b) rows.push_back({"Hunger", std::format("{:.0f}", pl.hunger)});
        }
        if (combo_.b && game::has(game::Domain::Combat)) rows.push_back({"Combo", i18n::fmt("{} (best {})", st.combat.combo, st.combat.bestCombo)});
        if (reach_.b && game::has(game::Domain::Combat)) rows.push_back({"Reach", st.combat.reachCount ? text::num(st.combat.lastReach, 2) : "–"});
        if (clock_.b) {
            SYSTEMTIME t;
            GetLocalTime(&t);
            rows.push_back({"Clock", std::format("{:02}:{:02}", t.wHour, t.wMinute)});
        }
        if (ram_.b) {
            PROCESS_MEMORY_COUNTERS pmc{};
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            rows.push_back({"RAM", std::format("{:.0f} MB", pmc.WorkingSetSize / (1024.0 * 1024.0))});
        }
        if (server_.b) {
            auto r = rules::status();
            rows.push_back({"Server", r.server.empty() ? "–" : r.server});
        }

        float kw = 0.f, vw = 0.f, y = 0.f;
        for (auto& r : rows) {
            kw = std::max(kw, textSize(s, i18n::tr(r.k.c_str())).x);
            vw = std::max(vw, textSize(s, r.v).x);
        }
        float gap = 10.f * s;
        for (auto& r : rows) {
            auto a = drawText(dl, o + ImVec2(0, y), s, i18n::tr(r.k.c_str()), accentColor());
            drawText(dl, o + ImVec2(kw + gap, y), s, r.v, textColor());
            y += a.y;
        }
        if (rows.empty()) return drawText(dl, o, s, i18n::tr("No line selected"), textColor());
        return {kw + gap + vw, y};
    }

private:
    Setting& fps_ = toggleSetting("fps", "FPS", true);
    Setting& frametime_ = toggleSetting("frametime", "Frametime", false);
    Setting& cps_ = toggleSetting("cps", "CPS", true);
    Setting& ping_ = toggleSetting("ping", "Ping", true);
    Setting& jitter_ = toggleSetting("jitter", "Jitter", false);
    Setting& coords_ = toggleSetting("coords", "Position", false);
    Setting& health_ = toggleSetting("health", "Health", false);
    Setting& hunger_ = toggleSetting("hunger", "Hunger", false);
    Setting& combo_ = toggleSetting("combo", "Combo", false);
    Setting& reach_ = toggleSetting("reach", "Reach", false);
    Setting& clock_ = toggleSetting("clock", "Time of day", false);
    Setting& ram_ = toggleSetting("ram", "RAM", false);
    Setting& server_ = toggleSetting("server", "Server", false);
    double frameMs_ = 0.0;
    bool probeOn_ = false;
};

class Watermark : public HudModule {
public:
    Watermark()
        : HudModule("Watermark", "Client logo with heart and version as text, pill or gradient.", {"cosmetic"},
                    {0.005f, 0.005f}) {
        sub("Info displays");
        background_.b = false;
        text_.visible = [this] { return custom_.b; };
    }

    void onFrame() override {
        double ms = dx::frame().frameMs;
        if (ms > 0.0) frameMs_ += (ms - frameMs_) * 0.05;
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& t = theme::current();
        std::string name = custom_.b ? text_.text : std::string(build::name);
        float h = fonts::hudSize() * s * 1.5f;
        float x = 0.f;
        if (heart_.b) {
            draw::heart(dl, o + ImVec2(h * 0.5f, h * 0.55f), h * 0.9f, ImGui::GetColorU32(t.accent));
            x = h + 4 * s;
        }
        ImFont* f = fonts::bold();
        float size = h * 0.9f;
        ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0.f, name.c_str());
        if (style_.i == 1) {
            ImVec2 a = o + ImVec2(x - 6 * s, 0), b = o + ImVec2(x + ts.x + 8 * s, h);
            draw::pill(dl, a, b, theme::col(t.surface, 0.8f));
        }
        if (style_.i == 2) {
            int start = dl->VtxBuffer.Size;
            dl->AddText(f, size, o + ImVec2(x, (h - ts.y) * 0.5f), IM_COL32_WHITE, name.c_str());
            for (int i = start; i < dl->VtxBuffer.Size; i++) {
                auto& v = dl->VtxBuffer[i];
                float k = std::clamp((v.pos.x - (o.x + x)) / std::max(ts.x, 1.f), 0.f, 1.f);
                v.col = theme::col(theme::mix(t.accent, t.accent2, k));
            }
        } else {
            dl->AddText(f, size, o + ImVec2(x, (h - ts.y) * 0.5f), ImGui::GetColorU32(textColor_.color), name.c_str());
        }
        float w = x + ts.x;
        std::string extra;
        if (version_.b) extra += std::string(build::version);
        if (fps_.b) extra += std::format("{}{:.0f} FPS", extra.empty() ? "" : "  ·  ", frameMs_ > 0 ? 1000.0 / frameMs_ : 0.0);
        if (!extra.empty()) {
            float small = size * 0.55f;
            dl->AddText(fonts::hud(), small, o + ImVec2(x + ts.x + 8 * s, (h - small) * 0.5f), theme::col(t.textDim), extra.c_str());
            w += 8 * s + fonts::hud()->CalcTextSizeA(small, FLT_MAX, 0.f, extra.c_str()).x;
        }
        return {w, h};
    }

private:
    Setting& style_ = choice("style", "Style", {"Text", "Pill", "Gradient"}, 2);
    Setting& heart_ = toggleSetting("heart", "Heart", true);
    Setting& custom_ = toggleSetting("custom", "Custom text", false);
    Setting& text_ = textSetting("text", "Text", "Mochi");
    Setting& version_ = toggleSetting("version", "Version", true);
    Setting& fps_ = toggleSetting("fps", "FPS behind it", false);
    double frameMs_ = 0.0;
};

class DebugMenu : public Module {
public:
    DebugMenu()
        : Module("Debug Menu", "Java-style debug display: FPS, position, chunk, biome, memory, renderer.",
                 Category::Hud, {"hud-self"}) {
        sub("Info displays");
    }

    void onKey(KeyEvent& ev) override {
        if (ev.down && !ev.repeat && key_.i && ev.vk == key_.i) visible_ = !visible_;
    }

    void onRender(ImDrawList* dl) override {
        if (!visible_ && !gui::editingHud()) return;
        float s = ui::scale() * scale_.f;
        auto ds = ImGui::GetIO().DisplaySize;
        std::vector<std::string> left, right;
        auto& st = game::state();
        auto& pl = st.player;

        double ms = dx::frame().frameMs;
        smooth_ += (ms - smooth_) * 0.05;
        left.push_back(std::format("{} {}", build::name, build::version));
        left.push_back(std::format("{:.0f} fps ({:.2f} ms)", smooth_ > 0 ? 1000.0 / smooth_ : 0.0, smooth_));
        if (game::has(game::Domain::Player)) {
            left.push_back("");
            left.push_back(std::format("XYZ: {:.3f} / {:.5f} / {:.3f}", pl.pos.x, pl.pos.y, pl.pos.z));
            left.push_back(i18n::fmt("Block: {} {} {}", int(std::floor(pl.pos.x)), int(std::floor(pl.pos.y)), int(std::floor(pl.pos.z))));
            left.push_back(i18n::fmt("Chunk: {} {} in {} {}", int(std::floor(pl.pos.x)) & 15, int(std::floor(pl.pos.z)) & 15, int(std::floor(pl.pos.x / 16.f)), int(std::floor(pl.pos.z / 16.f))));
            left.push_back(i18n::fmt("Facing: yaw {:.1f} pitch {:.1f}", pl.yaw, pl.pitch));
            left.push_back(i18n::fmt("Speed: {:.2f} b/s", std::sqrt(pl.vel.x * pl.vel.x + pl.vel.z * pl.vel.z)));
            left.push_back(i18n::fmt("Health {:.1f}  Hunger {:.0f}  Level {}", pl.health, pl.hunger, pl.level));
        }
        if (game::has(game::Domain::World)) {
            auto& w = st.world;
            left.push_back(i18n::fmt("Biome: {}", w.biome));
            left.push_back(i18n::fmt("Time: {} (day {})  {}", w.time, w.day, w.raining ? i18n::tr("Rain") : i18n::tr("clear")));
            left.push_back(i18n::fmt("Entities: {}  Players: {}", w.entities, w.players));
        }

        PROCESS_MEMORY_COUNTERS pmc{};
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        auto stats = sigs::stats();
        right.push_back(std::format("Renderer: {}", dx::api() == dx::Api::Dx12 ? "DirectX 12" : dx::api() == dx::Api::Dx11 ? "DirectX 11" : "–"));
        right.push_back(i18n::fmt("Window: {:.0f}x{:.0f}", ds.x, ds.y));
        right.push_back(std::format("RAM: {:.0f} MB", pmc.WorkingSetSize / (1024.0 * 1024.0)));
        right.push_back(std::format("Minecraft {}", stats.gameVersion));
        right.push_back(i18n::fmt("Signatures: {}/{} ({})", stats.found, stats.total, stats.source));
        auto r = rules::status();
        right.push_back(std::format("Server: {}", r.server.empty() ? "–" : r.server));
        if (game::demo()) right.push_back(i18n::tr("Demo data on"));
        if (channels_.b) {
            right.push_back("");
            right.push_back(i18n::tr("Effect channels:"));
            for (int i = 0; i < int(fx::Id::Count); i++) {
                auto rep = fx::report(fx::Id(i));
                if (rep.requested) right.push_back(std::format("  {} {}", fx::info(fx::Id(i)).sig, rep.installed ? i18n::tr("active") : i18n::tr("requested")));
            }
        }

        float size = fonts::hudSize() * s, lineH = size * 1.25f;
        ImU32 tc = ImGui::GetColorU32(textColor_.color);
        auto column = [&](const std::vector<std::string>& lines, bool rightSide) {
            float y = 6 * s;
            for (auto& l : lines) {
                if (l.empty()) {
                    y += lineH * 0.5f;
                    continue;
                }
                ImVec2 ts = fonts::hud()->CalcTextSizeA(size, FLT_MAX, 0.f, l.c_str());
                float x = rightSide ? ds.x - ts.x - 8 * s : 6 * s;
                if (bg_.b) dl->AddRectFilled({x - 3 * s, y - 1}, {x + ts.x + 3 * s, y + lineH - 1}, ImGui::GetColorU32(bgColor_.color));
                dl->AddText(fonts::hud(), size, {x, y}, tc, l.c_str());
                y += lineH;
            }
        };
        if (left_.b) column(left, false);
        if (right_.b) column(right, true);
    }

private:
    Setting& key_ = keySetting("toggle", "On/off key", VK_F3);
    Setting& left_ = toggleSetting("left", "Left column", true);
    Setting& right_ = toggleSetting("right", "Right column", true);
    Setting& channels_ = toggleSetting("channels", "Show active effect channels", true);
    Setting& scale_ = slider("scale", "Size", 1.f, 0.6f, 2.f, "%.2fx");
    Setting& bg_ = toggleSetting("bg", "Line background", true);
    Setting& bgColor_ = colorSetting("bgColor", "Background color", {0.f, 0.f, 0.f, 0.4f});
    Setting& textColor_ = colorSetting("textColor", "Text color", {1.f, 1.f, 1.f, 1.f});
    bool visible_ = false;
    double smooth_ = 0.0;
};
