#pragma once

#include "core/Build.hpp"
#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "hook/Dx.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/SysInfo.hpp"
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

#include <algorithm>
#include <array>
#include <ctime>
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
        : Module("Debug Menu", "Java-style debug display (F3) with performance, position, world, target, system, server and time blocks and a frametime graph.",
                 Category::Hud, {"hud-self"}) {
        sub("Info displays");
        graphWidth_.visible = [this] { return graph_.b; };
        graphHeight_.visible = [this] { return graph_.b; };
        graphAnchor_.visible = [this] { return graph_.b; };
        graphLines_.visible = [this] { return graph_.b; };
        graphFrames_.visible = [this] { return graph_.b; };
        channels_.visible = [this] { return mochi_.b; };
    }

    void onKey(KeyEvent& ev) override {
        if (ev.down && !ev.repeat && key_.i && ev.vk == key_.i) visible_ = !visible_;
    }

    void onFrame() override {
        double ms = dx::frame().frameMs;
        if (ms <= 0.0) return;
        smooth_ += (ms - smooth_) * 0.05;
        history_[head_ % history_.size()] = float(ms);
        head_++;
    }

    void onRender(ImDrawList* dl) override {
        if (!visible_ && !gui::editingHud()) return;
        float s = ui::scale() * scale_.f;
        auto ds = ImGui::GetIO().DisplaySize;
        std::vector<std::string> left, right;
        auto& st = game::state();
        auto& pl = st.player;
        float cpu = cpu_.percent();

        auto gap = [](std::vector<std::string>& v) {
            if (!v.empty() && !v.back().empty()) v.push_back("");
        };
        auto head = [&](std::vector<std::string>& v, const char* title) { v.push_back(std::string("#") + i18n::tr(title)); };

        left.push_back(std::format("{} {}", build::name, build::version));
        if (perf_.b) {
            gap(left);
            head(left, "Performance");
            left.push_back(std::format("{:.0f} fps ({:.2f} ms)", smooth_ > 0 ? 1000.0 / smooth_ : 0.0, smooth_));
            if (head_ > 30) {
                size_t n = std::min<size_t>(head_, history_.size());
                std::vector<float> copy(history_.begin(), history_.begin() + long(n));
                std::sort(copy.begin(), copy.end());
                float worst = copy[std::min(n - 1, size_t(float(n) * 0.99f))];
                left.push_back(i18n::fmt("1% low: {:.0f} fps  ·  worst frame {:.1f} ms", 1000.f / std::max(worst, 0.1f), copy.back()));
            }
        }
        if (game::has(game::Domain::Player) && position_.b) {
            gap(left);
            head(left, "Position");
            left.push_back(std::format("XYZ: {:.3f} / {:.5f} / {:.3f}", pl.pos.x, pl.pos.y, pl.pos.z));
            left.push_back(i18n::fmt("Block: {} {} {}", int(std::floor(pl.pos.x)), int(std::floor(pl.pos.y)), int(std::floor(pl.pos.z))));
            left.push_back(i18n::fmt("Chunk: {} {} in {} {}", int(std::floor(pl.pos.x)) & 15, int(std::floor(pl.pos.z)) & 15, int(std::floor(pl.pos.x / 16.f)), int(std::floor(pl.pos.z / 16.f))));
            left.push_back(i18n::fmt("Facing: yaw {:.1f} pitch {:.1f}", pl.yaw, pl.pitch));
            left.push_back(i18n::fmt("Speed: {:.2f} b/s  ·  velocity {:.2f} {:.2f} {:.2f}", std::sqrt(pl.vel.x * pl.vel.x + pl.vel.z * pl.vel.z), pl.vel.x, pl.vel.y, pl.vel.z));
        }
        if (game::has(game::Domain::Player) && player_.b) {
            gap(left);
            head(left, "Player");
            left.push_back(i18n::fmt("Health {:.1f}  Hunger {:.0f}  Level {}", pl.health, pl.hunger, pl.level));
            if (st.combat.hits || st.combat.kills) left.push_back(i18n::fmt("Hits {}  Kills {}  Deaths {}", st.combat.hits, st.combat.kills, st.combat.deaths));
        }
        if (game::has(game::Domain::World) && world_.b) {
            auto& w = st.world;
            gap(left);
            head(left, "World");
            if (!w.name.empty()) left.push_back(i18n::fmt("World: {}", w.name));
            left.push_back(i18n::fmt("Biome: {}", w.biome));
            left.push_back(i18n::fmt("Dimension: {}", pl.dimension == 1 ? "Nether" : pl.dimension == 2 ? "The End" : "Overworld"));
            left.push_back(i18n::fmt("Time: {} (day {})  {}", w.time, w.day, w.thundering ? i18n::tr("Thunder") : w.raining ? i18n::tr("Rain") : i18n::tr("clear")));
            left.push_back(i18n::fmt("Entities: {}  Players: {}", w.entities, w.players));
        }
        if (game::has(game::Domain::Target) && target_.b) {
            auto& t = st.target;
            if (t.kind != game::Target::Kind::None) {
                gap(left);
                head(left, "Target");
                if (t.kind == game::Target::Kind::Block) {
                    left.push_back(i18n::fmt("Block: {}", t.name));
                    left.push_back(std::format("{} {} {}", t.blockX, t.blockY, t.blockZ));
                    if (t.breakProgress > 0.f) left.push_back(i18n::fmt("Breaking: {:.0f}%", t.breakProgress * 100.f));
                } else {
                    left.push_back(i18n::fmt("Entity: {}  ·  {:.1f} m", t.name, t.distance));
                    if (t.health > 0.f) left.push_back(i18n::fmt("Health {:.1f} / {:.0f}", t.health, t.maxHealth));
                }
            }
        }

        if (system_.b) {
            PROCESS_MEMORY_COUNTERS pmc{};
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            head(right, "System");
            right.push_back(std::format("Renderer: {}", dx::api() == dx::Api::Dx12 ? "DirectX 12" : dx::api() == dx::Api::Dx11 ? "DirectX 11" : "–"));
            right.push_back(std::format("GPU: {}", sysinfo::gpuName()));
            right.push_back(i18n::fmt("Window: {:.0f}x{:.0f}", ds.x, ds.y));
            right.push_back(i18n::fmt("RAM: {:.0f} MB  ·  system {:.0f}%", pmc.WorkingSetSize / (1024.0 * 1024.0), sysinfo::systemRamPercent()));
            right.push_back(i18n::fmt("CPU: {:.0f}% of {} threads", cpu, sysinfo::cores()));
        }
        if (server_.b) {
            auto r = rules::status();
            gap(right);
            head(right, "Server");
            right.push_back(std::format("{}", r.server.empty() ? "–" : r.server));
            if (!r.ip.empty()) right.push_back(std::format("IP: {}", r.ip));
            if (game::has(game::Domain::World)) {
                right.push_back(i18n::fmt("Ping: {} ms", st.world.ping));
                if (st.world.tps > 0.f) right.push_back(std::format("TPS: {:.1f}", st.world.tps));
            }
        }
        if (times_.b) {
            std::time_t now = std::time(nullptr);
            std::tm tm{};
            localtime_s(&tm, &now);
            gap(right);
            head(right, "Time");
            right.push_back(std::format("{:02}:{:02}:{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec));
            right.push_back(i18n::fmt("Game running for {}", text::clock(float(sysinfo::processUptimeSeconds()))));
        }
        if (mochi_.b) {
            auto stats = sigs::stats();
            gap(right);
            head(right, "Mochi");
            right.push_back(std::format("Minecraft {}", stats.gameVersion));
            right.push_back(i18n::fmt("Signatures: {}/{} ({})", stats.found, stats.total, stats.source));
            if (game::demo()) right.push_back(i18n::tr("Demo data on"));
            if (channels_.b) {
                right.push_back(i18n::tr("Effect channels:"));
                for (int i = 0; i < int(fx::Id::Count); i++) {
                    auto rep = fx::report(fx::Id(i));
                    if (rep.requested) right.push_back(std::format("  {} {}", fx::info(fx::Id(i)).sig, rep.installed ? i18n::tr("active") : i18n::tr("requested")));
                }
            }
        }

        float size = fonts::hudSize() * s, lineH = size * 1.25f;
        ImU32 tc = ImGui::GetColorU32(textColor_.color), hc = ImGui::GetColorU32(titleColor_.color);
        auto column = [&](const std::vector<std::string>& lines, bool rightSide) {
            float y = 6 * s;
            for (auto& l : lines) {
                if (l.empty()) {
                    y += lineH * 0.5f;
                    continue;
                }
                bool title = l[0] == '#';
                std::string shown = title ? l.substr(1) : l;
                ImVec2 ts = fonts::hud()->CalcTextSizeA(size, FLT_MAX, 0.f, shown.c_str());
                float x = rightSide ? ds.x - ts.x - 8 * s : 6 * s;
                if (bg_.b) dl->AddRectFilled({x - 3 * s, y - 1}, {x + ts.x + 3 * s, y + lineH - 1}, ImGui::GetColorU32(bgColor_.color));
                dl->AddText(fonts::hud(), size, {x, y}, title ? hc : tc, shown.c_str());
                y += lineH;
            }
            return y;
        };
        float leftEnd = left_.b ? column(left, false) : 6 * s;
        if (right_.b) column(right, true);
        if (graph_.b) graph(dl, s, ds, leftEnd);
    }

private:
    void graph(ImDrawList* dl, float s, ImVec2 ds, float top) {
        float w = graphWidth_.f * s, h = graphHeight_.f * s;
        float x = graphAnchor_.i == 0 ? 6 * s : graphAnchor_.i == 1 ? (ds.x - w) * 0.5f : ds.x - w - 8 * s;
        float y = graphAnchor_.i == 1 ? ds.y - h - 10 * s : std::max(top + 6 * s, ds.y - h - 10 * s);
        if (graphAnchor_.i == 0) y = std::min(y, ds.y - h - 10 * s);
        ImVec2 a{x, y}, b{x + w, y + h};
        dl->AddRectFilled(a, b, ImGui::GetColorU32(bgColor_.color));
        size_t n = std::min<size_t>(size_t(graphFrames_.i), std::min<size_t>(head_, history_.size()));
        if (n < 2) return;
        float scaleMs = 50.f;
        float barW = w / float(graphFrames_.i);
        auto yAt = [&](float ms) { return b.y - std::min(ms, scaleMs) / scaleMs * h; };
        if (graphLines_.b) {
            for (float ms : {1000.f / 60.f, 1000.f / 30.f}) {
                dl->AddLine({a.x, yAt(ms)}, {b.x, yAt(ms)}, IM_COL32(255, 255, 255, 70), 1.f);
                dl->AddText(fonts::hud(), 10.f * s, {a.x + 2, yAt(ms) - 11 * s}, IM_COL32(255, 255, 255, 120), ms < 20.f ? "60" : "30");
            }
        }
        for (size_t i = 0; i < n; i++) {
            float ms = history_[(head_ - n + i) % history_.size()];
            float bx = b.x - float(n - i) * barW;
            ImVec4 c = ms < 1000.f / 58.f ? ImVec4(0.55f, 0.91f, 0.69f, 1.f) : ms < 1000.f / 30.f ? ImVec4(1.f, 0.82f, 0.49f, 1.f) : ImVec4(1.f, 0.4f, 0.45f, 1.f);
            dl->AddRectFilled({bx, yAt(ms)}, {bx + std::max(barW - 0.5f, 1.f), b.y}, ImGui::GetColorU32(c));
        }
    }

    Setting& key_ = keySetting("toggle", "On/off key", VK_F3);
    Setting& left_ = toggleSetting("left", "Left column", true);
    Setting& right_ = toggleSetting("right", "Right column", true);
    Setting& perf_ = toggleSetting("perf", "Performance block", true);
    Setting& position_ = toggleSetting("position", "Position block", true);
    Setting& player_ = toggleSetting("player", "Player block", true);
    Setting& world_ = toggleSetting("world", "World block", true);
    Setting& target_ = toggleSetting("target", "Target block", true);
    Setting& system_ = toggleSetting("system", "System block", true);
    Setting& server_ = toggleSetting("serverBlock", "Server block", true);
    Setting& times_ = toggleSetting("times", "Time block", true);
    Setting& mochi_ = toggleSetting("mochi", "Mochi block", true);
    Setting& channels_ = toggleSetting("channels", "Show active effect channels", true);
    Setting& graph_ = toggleSetting("graph", "Frametime graph", true);
    Setting& graphWidth_ = slider("graphWidth", "Graph width", 240.f, 100.f, 600.f, "%.0f");
    Setting& graphHeight_ = slider("graphHeight", "Graph height", 60.f, 30.f, 200.f, "%.0f");
    Setting& graphFrames_ = intSlider("graphFrames", "Frames in the graph", 240, 60, 360);
    Setting& graphAnchor_ = choice("graphAnchor", "Graph position", {"Bottom left", "Bottom center", "Bottom right"});
    Setting& graphLines_ = toggleSetting("graphLines", "30 and 60 FPS lines", true);
    Setting& scale_ = slider("scale", "Size", 1.f, 0.6f, 2.f, "%.2fx");
    Setting& bg_ = toggleSetting("bg", "Line background", true);
    Setting& bgColor_ = colorSetting("bgColor", "Background color", {0.f, 0.f, 0.f, 0.4f});
    Setting& textColor_ = colorSetting("textColor", "Text color", {1.f, 1.f, 1.f, 1.f});
    Setting& titleColor_ = colorSetting("titleColor", "Block titles", {1.f, 0.49f, 0.71f, 1.f});
    bool visible_ = false;
    double smooth_ = 0.0;
    std::array<float, 360> history_{};
    size_t head_ = 0;
    sysinfo::Cpu cpu_;
};
