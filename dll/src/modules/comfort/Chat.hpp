#pragma once

#include "core/Paths.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "gui/Theme.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "modules/server/ServerChat.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"
#include "sdk/Inject.hpp"

#include <windows.h>

#include <algorithm>
#include <ctime>
#include <format>
#include <fstream>
#include <random>
#include <set>
#include <sstream>

inline std::vector<std::string> splitList(const std::string& in, char sep) {
    std::vector<std::string> out;
    std::stringstream ss(in);
    std::string part;
    while (std::getline(ss, part, sep)) {
        auto a = part.find_first_not_of(' '), b = part.find_last_not_of(' ');
        if (a != std::string::npos) out.push_back(part.substr(a, b - a + 1));
    }
    return out;
}

class AutoGG : public Module {
public:
    AutoGG()
        : Module("Auto GG", "Automatically writes gg at the end of a game. On a kill it is banned on some servers.",
                 Category::Comfort, {"chat"}) {
        sub("Chat");
        require(need::chat, need::sigs({"ChatEvents"}));
        triggers_.visible = [this] { return preset_.i == 1; };
        killMessage_.visible = [this] { return onKill_.b; };
    }

    void onFrame() override {
        double now = ui::time();
        if (pending_ && now >= due_) {
            pending_ = false;
            inject::say(message_, chatKey_.i);
        }
        for (auto& e : game::events()) {
            if (e.kind == game::EventKind::Chat && onEnd_.b && endMatches(e.text)) schedule(pick(messages_.text), delay_.f);
            if (e.kind == game::EventKind::Kill && onKill_.b && !optionBlocked("onKill")) schedule(pick(killMessage_.text), 0.4f);
        }
    }

    void drawSettings() override {
        if (optionBlocked("onKill")) ImGui::TextColored(theme::current().warn, i18n::tr("On this server the message on a kill is banned and disabled."));
    }

private:
    bool endMatches(const std::string& raw) const {
        std::string line = text::lower(text::strip(raw));
        std::vector<std::string> list = preset_.i == 1 ? splitList(text::lower(triggers_.text), ',') : srv::endWords(game::state().server);
        for (auto& t : list)
            if (line.find(t) != std::string::npos) return true;
        return false;
    }

    std::string pick(const std::string& pool) {
        auto items = splitList(pool, '|');
        if (items.empty()) return "gg";
        return items[std::uniform_int_distribution<size_t>(0, items.size() - 1)(rng_)];
    }

    void schedule(const std::string& msg, float delay) {
        double now = ui::time();
        if (now - last_ < cooldown_.f) return;
        last_ = now;
        message_ = msg;
        due_ = now + delay;
        pending_ = true;
    }

    Setting& onEnd_ = toggleSetting("onEnd", "At the end of a game", true);
    Setting& onKill_ = toggleSetting("onKill", "After a kill", false);
    Setting& preset_ = choice("preset", "Detect game end", {"Automatic by server", "Own words"});
    Setting& triggers_ = textSetting("triggers", "Words in chat (comma)", "game over, victory");
    Setting& messages_ = textSetting("messages", "Messages (random, separate with |)", "gg|gg wp|good game");
    Setting& killMessage_ = textSetting("killMessage", "Message after kill", "gg");
    Setting& delay_ = slider("delay", "Delay (s)", 1.5f, 0.3f, 8.f, "%.1f s");
    Setting& cooldown_ = slider("cooldown", "Minimum gap (s)", 12.f, 3.f, 60.f, "%.0f s");
    Setting& chatKey_ = keySetting("chatKey", "Chat key in game", 'T');
    std::mt19937 rng_{std::random_device{}()};
    std::string message_;
    double due_ = 0.0;
    double last_ = -100.0;
    bool pending_ = false;
};

class MessageLogger : public Module {
public:
    MessageLogger()
        : Module("Message Logger", "Saves the chat to a text file per day, with timestamps and filter.", Category::Comfort, {"hud-self"}) {
        sub("Chat");
        require(need::chat, need::sigs({"ChatEvents"}));
    }

    void onFrame() override {
        for (auto& e : game::events()) {
            if (e.kind != game::EventKind::Chat) continue;
            std::string line = colors_.b ? text::strip(e.text) : e.text;
            if (!filter_.text.empty() && text::lower(line).find(text::lower(filter_.text)) == std::string::npos) continue;
            SYSTEMTIME t;
            GetLocalTime(&t);
            auto dir = paths::logs() / L"chat";
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            std::ofstream out(dir / std::filesystem::path(std::format("chat-{:04}-{:02}-{:02}.txt", t.wYear, t.wMonth, t.wDay)), std::ios::app);
            if (stamp_.b) out << std::format("[{:02}:{:02}:{:02}] ", t.wHour, t.wMinute, t.wSecond);
            if (server_.b && !game::state().server.empty()) out << "[" << game::state().server << "] ";
            out << line << "\n";
        }
    }

private:
    Setting& colors_ = toggleSetting("strip", "Remove color codes", true);
    Setting& stamp_ = toggleSetting("stamp", "Timestamp", true);
    Setting& server_ = toggleSetting("server", "Prefix the server name", false);
    Setting& filter_ = textSetting("filter", "Only lines containing (empty = all)", "");
};

class DeathLogger : public GameList {
public:
    DeathLogger()
        : GameList("Death Logger", "Remembers where you died and shows the last death points with coordinates.", need::player,
                   need::sigs({"LocalPlayer"}), {"hud-self"}, {0.01f, 0.20f}) {
        sub("Chat");
    }

    void onKey(KeyEvent& ev) override {
        if (ev.down && !ev.repeat && ev.vk == copyKey_.i && copyKey_.i && !list_.empty()) {
            auto& d = list_.back();
            ImGui::SetClipboardText(std::format("{} {} {}", int(d.x), int(d.y), int(d.z)).c_str());
            notify::push(i18n::tr("Copied"), i18n::tr("Death point is on the clipboard."), notify::Kind::Ok);
        }
    }

    void onFrame() override {
        for (auto& e : game::events()) {
            if (e.kind != game::EventKind::Death) continue;
            auto& p = game::state().player;
            list_.push_back({p.pos.x, p.pos.y, p.pos.z, p.dimension, ui::time()});
            while ((int)list_.size() > keep_.i) list_.erase(list_.begin());
            if (toast_.b) notify::push(i18n::tr("You died"), std::format("{} {} {}", int(p.pos.x), int(p.pos.y), int(p.pos.z)), notify::Kind::Info);
        }
    }

    void onRender(ImDrawList* dl) override {
        if (list_.empty() && !gui::editingHud()) return;
        GameList::onRender(dl);
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        float y = 0.f, w = 0.f;
        if (list_.empty()) return drawText(dl, o, s, i18n::tr("No death yet"), textColor());
        int shown = 0;
        for (auto it = list_.rbegin(); it != list_.rend() && shown < show_.i; ++it, ++shown) {
            std::string t = std::format("{} {} {}", int(it->x), int(it->y), int(it->z));
            if (age_.b) t += "  ·  " + text::clock(float(ui::time() - it->at));
            auto sz = drawText(dl, o + ImVec2(0, y), s, t, shown == 0 ? accentColor() : textColor());
            w = std::max(w, sz.x);
            y += sz.y;
        }
        return {w, y};
    }

private:
    struct Death {
        float x, y, z;
        int dim;
        double at;
    };

    Setting& keep_ = intSlider("keep", "Remembered death points", 5, 1, 20);
    Setting& show_ = intSlider("show", "Shown death points", 3, 1, 10);
    Setting& age_ = toggleSetting("age", "Time since death", true);
    Setting& toast_ = toggleSetting("toast", "Notice on death", true);
    Setting& copyKey_ = keySetting("copyKey", "Copy last point", 0);
    std::vector<Death> list_;
};

class ChatPlus : public HudModule {
public:
    ChatPlus()
        : HudModule("Better Chat", "Your own movable chat with timestamps, filter and highlighting.",
                    {"hud-self"}, {0.01f, 0.55f}) {
        sub("Chat");
        require(need::chat, need::sigs({"ChatEvents"}));
        background_.b = false;
        highlightWords_.visible = [this] { return highlight_.b; };
        highlightColor_.visible = [this] { return highlight_.b; };
    }

    bool defaultEnabled() const override { return false; }

    void onFrame() override {
        if (hideVanilla_.b) fx::skip(fx::Id::HideChat);
    }

    void onRender(ImDrawList* dl) override {
        if (!game::state().inWorld && !gui::editingHud()) return;
        HudModule::onRender(dl);
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& chat = game::state().chat;
        bool open = game::state().screen == game::Screen::Chat || gui::editingHud();
        double now = ui::time();
        float w = width_.f * s, lineH = fonts::hudSize() * s * 1.15f;
        std::vector<std::pair<const game::ChatLine*, int>> lines;
        std::vector<std::string> words = splitList(text::lower(highlightWords_.text), ',');
        std::string filter = text::lower(filter_.text);

        for (auto& l : chat) {
            std::string plain = text::lower(text::strip(l.text));
            if (!filter.empty() && plain.find(filter) == std::string::npos) continue;
            if (compact_.b && !lines.empty() && lines.back().first->text == l.text) {
                lines.back().second++;
                lines.back().first = &l;
                continue;
            }
            lines.push_back({&l, 1});
        }
        int total = int(lines.size());
        int from = std::max(0, total - lines_.i);
        float y = 0.f;
        if (background_.b && !lines.empty()) dl->AddRectFilled(o - ImVec2(4 * s, 2 * s), o + ImVec2(w + 4 * s, lineH * std::min(lines_.i, total) + 2 * s), ImGui::GetColorU32(bgColor_.color), rounding_.f * s);

        for (int i = from; i < total; i++) {
            auto& [l, count] = lines[size_t(i)];
            float age = float(now - l->time);
            float a = open ? 1.f : std::clamp((fade_.f - age) / 1.f, 0.f, 1.f);
            if (a <= 0.f) {
                y += 0.f;
                continue;
            }
            ImU32 base = ImGui::GetColorU32(withAlpha(textColor_.color, a));
            float x = 0.f;
            if (stamp_.b) {
                std::time_t t = std::time(nullptr) - std::time_t(age);
                std::tm tm{};
                localtime_s(&tm, &t);
                std::string st = std::format("[{:02}:{:02}] ", tm.tm_hour, tm.tm_min);
                x += drawText(dl, o + ImVec2(0, y), s, st, ImGui::GetColorU32(withAlpha(theme::current().textDim, a))).x;
            }
            std::string plain = text::lower(text::strip(l->text));
            bool hit = false;
            if (highlight_.b)
                for (auto& wd : words)
                    if (plain.find(wd) != std::string::npos) hit = true;
            if (hit) dl->AddRectFilled(o + ImVec2(0, y), o + ImVec2(w, y + lineH), ImGui::GetColorU32(withAlpha(highlightColor_.color, 0.25f * a)), 3 * s);

            dl->PushClipRect(o + ImVec2(0, y), o + ImVec2(w, y + lineH + 2), true);
            if (colors_.b) {
                for (auto& seg : text::colored(l->text, base)) {
                    ImVec4 c = ImGui::ColorConvertU32ToFloat4(seg.color);
                    c.w *= a;
                    x += drawText(dl, o + ImVec2(x, y), s, seg.text, ImGui::GetColorU32(c)).x;
                }
            } else {
                x += drawText(dl, o + ImVec2(x, y), s, text::strip(l->text), base).x;
            }
            if (count > 1) drawText(dl, o + ImVec2(x + 4 * s, y), s, std::format("x{}", count), ImGui::GetColorU32(withAlpha(theme::current().accent, a)));
            dl->PopClipRect();
            y += lineH;
        }
        return {w, std::max(y, lineH)};
    }

private:
    Setting& width_ = slider("width", "Width", 420.f, 200.f, 900.f, "%.0f");
    Setting& lines_ = intSlider("lines", "Visible lines", 10, 3, 30);
    Setting& fade_ = slider("fade", "Visible for (s)", 10.f, 3.f, 60.f, "%.0f s");
    Setting& stamp_ = toggleSetting("stamp", "Timestamp", false);
    Setting& compact_ = toggleSetting("compact", "Merge identical lines", true);
    Setting& colors_ = toggleSetting("colors", "Show color codes", true);
    Setting& highlight_ = toggleSetting("highlight", "Highlight words", false);
    Setting& highlightWords_ = textSetting("highlightWords", "Words (comma)", "");
    Setting& highlightColor_ = colorSetting("highlightColor", "Highlight", {1.f, 0.82f, 0.49f, 1.f});
    Setting& filter_ = textSetting("filter", "Only lines containing (empty = all)", "");
    Setting& hideVanilla_ = toggleSetting("hideVanilla", "Hide the original chat", true);
};

class PlayerNotifier : public Module {
public:
    PlayerNotifier()
        : Module("Player Notifier", "Tells you when a player from your list joins the server, for example friends.", Category::Comfort, {"info-others"}) {
        sub("Chat");
        require(need::tab, need::sigs({"TabListData"}));
    }

    void onFrame() override {
        double now = ui::time();
        if (now - last_ < 1.0) return;
        last_ = now;
        auto names = splitList(text::lower(list_.text), ',');
        std::set<std::string> present;
        for (auto& e : game::state().tab) {
            std::string n = text::lower(e.name);
            present.insert(n);
            if (std::find(names.begin(), names.end(), n) != names.end() && !seen_.count(n) && primed_) {
                notify::push(i18n::tr("Player online"), e.name, notify::Kind::Info, 6.f);
                if (sound_.b) MessageBeep(MB_ICONASTERISK);
            }
        }
        if (leave_.b)
            for (auto& n : seen_)
                if (!present.count(n) && std::find(names.begin(), names.end(), n) != names.end()) notify::push(i18n::tr("Player left"), n, notify::Kind::Info, 4.f);
        seen_ = present;
        primed_ = true;
    }

    void onServer(const ServerEvent&) override {
        seen_.clear();
        primed_ = false;
    }

private:
    Setting& list_ = textSetting("list", "Names (comma)", "");
    Setting& sound_ = toggleSetting("sound", "Sound", true);
    Setting& leave_ = toggleSetting("leave", "Also report when they leave", false);
    std::set<std::string> seen_;
    bool primed_ = false;
    double last_ = 0.0;
};

class ScoreboardPlus : public HudModule {
public:
    ScoreboardPlus()
        : HudModule("Scoreboard", "Your own movable scoreboard without red numbers.",
                    {"hud-self"}, {0.84f, 0.30f}) {
        sub("HUD parts");
        require(need::board, need::sigs({"ScoreboardData"}));
    }

    void onFrame() override {
        if (hideVanilla_.b) fx::skip(fx::Id::HideScoreboard);
    }

    void onRender(ImDrawList* dl) override {
        if (!game::state().inWorld && !gui::editingHud()) return;
        HudModule::onRender(dl);
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& sb = game::state().scoreboard;
        float w = 0.f, y = 0.f;
        std::vector<std::pair<std::string, std::string>> rows;
        int n = 0;
        for (auto& [text, score] : sb.lines) {
            if (n++ >= max_.i) break;
            rows.push_back({text, numbers_.b ? std::to_string(score) : ""});
        }
        std::string title = text::strip(sb.title);
        if (titleOn_.b && !title.empty()) w = textSize(s, title).x;
        for (auto& r : rows) w = std::max(w, textSize(s, text::strip(r.first)).x + (r.second.empty() ? 0.f : textSize(s, r.second).x + 16 * s));
        if (titleOn_.b && !title.empty()) {
            drawText(dl, o + ImVec2((w - textSize(s, title).x) * 0.5f, 0), s, title, accentColor());
            y += fonts::hudSize() * s * 1.3f;
        }
        for (auto& r : rows) {
            drawText(dl, o + ImVec2(0, y), s, text::strip(r.first), textColor());
            if (!r.second.empty()) drawText(dl, o + ImVec2(w - textSize(s, r.second).x, y), s, r.second, ImGui::GetColorU32(numberColor_.color));
            y += fonts::hudSize() * s * 1.12f;
        }
        return {std::max(w, 80.f * s), std::max(y, 14.f * s)};
    }

private:
    Setting& titleOn_ = toggleSetting("title", "Title", true);
    Setting& numbers_ = toggleSetting("numbers", "Numbers on the right", false);
    Setting& numberColor_ = colorSetting("numberColor", "Number color", {1.f, 0.4f, 0.45f, 1.f});
    Setting& max_ = intSlider("max", "Maximum lines", 15, 3, 20);
    Setting& hideVanilla_ = toggleSetting("hideVanilla", "Hide the original scoreboard", true);
};

class TabList : public HudModule {
public:
    TabList()
        : HudModule("Tab List", "Java-style player list with columns, ping and sorting.",
                    {"info-others"}, {0.30f, 0.05f}) {
        sub("HUD parts");
        require(need::tab, need::sigs({"TabListData"}));
        rows_.visible = [this] { return columns_.i == 0; };
    }

    void onRender(ImDrawList* dl) override {
        if (!game::state().inWorld && !gui::editingHud()) return;
        if (onHold_.b && !input::down(VK_TAB) && !gui::editingHud()) return;
        HudModule::onRender(dl);
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        std::vector<game::TabEntry> list = game::state().tab;
        if (sort_.i == 0) std::sort(list.begin(), list.end(), [](auto& a, auto& b) { return text::lower(a.name) < text::lower(b.name); });
        else if (sort_.i == 1) std::sort(list.begin(), list.end(), [](auto& a, auto& b) { return a.ping < b.ping; });
        int per = columns_.i > 0 ? int((list.size() + size_t(columns_.i) - 1) / size_t(columns_.i)) : rows_.i;
        per = std::max(per, 1);
        int cols = int((list.size() + size_t(per) - 1) / size_t(per));
        float rowH = fonts::hudSize() * s * 1.15f, x = 0.f, total = 0.f;
        for (int c = 0; c < cols; c++) {
            float colW = 60.f * s;
            for (int r = 0; r < per; r++) {
                size_t i = size_t(c * per + r);
                if (i >= list.size()) break;
                colW = std::max(colW, textSize(s, list[i].name).x + (ping_.b ? 56.f * s : 8.f * s));
            }
            for (int r = 0; r < per; r++) {
                size_t i = size_t(c * per + r);
                if (i >= list.size()) break;
                auto& e = list[i];
                bool me = e.name == game::state().player.name;
                drawText(dl, o + ImVec2(x, r * rowH), s, e.name, me ? accentColor() : textColor());
                if (ping_.b) {
                    std::string t = pingBars_.b ? "" : std::format("{}", e.ping);
                    ImVec4 c4 = rampColor(float(e.ping), 40.f, 200.f, good_.color, mid_.color, bad_.color);
                    if (pingBars_.b) {
                        int lit = e.ping < 60 ? 4 : e.ping < 110 ? 3 : e.ping < 180 ? 2 : 1;
                        for (int b = 0; b < 4; b++) {
                            float bh = (3 + b * 2) * s;
                            ImVec2 base{o.x + x + colW - 26 * s + b * 5 * s, o.y + r * rowH + rowH - 3 * s};
                            dl->AddRectFilled(base - ImVec2(0, bh), base + ImVec2(3 * s, 0), b < lit ? ImGui::GetColorU32(c4) : IM_COL32(255, 255, 255, 40));
                        }
                    } else {
                        drawText(dl, o + ImVec2(x + colW - textSize(s, t).x - 6 * s, r * rowH), s, t, ImGui::GetColorU32(c4));
                    }
                }
            }
            x += colW + 12 * s;
            total = x;
        }
        return {std::max(total - 12 * s, 60.f * s), per * rowH};
    }

private:
    Setting& columns_ = intSlider("columns", "Columns (0 = by rows)", 0, 0, 6);
    Setting& rows_ = intSlider("rows", "Rows per column", 20, 5, 40);
    Setting& sort_ = choice("sort", "Sorting", {"Name", "Ping", "As sent by the server"}, 2);
    Setting& ping_ = toggleSetting("ping", "Show ping", true);
    Setting& pingBars_ = toggleSetting("pingBars", "Ping as bars", true);
    Setting& onHold_ = toggleSetting("onHold", "Only while Tab is held", true);
    Setting& good_ = colorSetting("good", "Ping good", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& mid_ = colorSetting("mid", "Ping medium", {1.f, 0.82f, 0.49f, 1.f});
    Setting& bad_ = colorSetting("bad", "Ping bad", {1.f, 0.4f, 0.45f, 1.f});
};
