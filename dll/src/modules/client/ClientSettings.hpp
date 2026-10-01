#pragma once

#include "gui/Notify.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"
#include "sdk/Game.hpp"

#include <cctype>
#include <format>
#include <string>

class ClientSettings : public Module {
public:
    ClientSettings()
        : Module("Client Settings", "Name tag behind your name in chat, notifications and other global options.", Category::Client) {
        sub("Client");
        tagText_.visible = [this] { return tag_.b; };
        tagColor_.visible = [this] { return tag_.b; };
        tagPos_.visible = [this] { return tag_.b; };
        brackets_.visible = [this] { return tag_.b; };
        tabTag_.visible = [this] { return tag_.b; };
    }

    bool alwaysOn() const override { return true; }

    void onFrame() override { notify::setMuted(!notifications_.b); }

    std::string tagged(const std::string& line, bool chat) const {
        if (!tag_.b || (!chat && !tabTag_.b)) return line;
        const std::string& me = game::state().player.name;
        if (me.empty()) return line;
        size_t at = nameEnd(line, me);
        if (at == std::string::npos) return line;
        const auto& c = tagColor_.color;
        std::string body = brackets_.b ? "[" + tagText_.text + "]" : tagText_.text;
        std::string tag = std::format("§#{:02X}{:02X}{:02X};{}§r", int(c.x * 255.f), int(c.y * 255.f), int(c.z * 255.f), body);
        if (chat && tagPos_.i == 1) {
            size_t close = line.find_first_of(">:", at);
            at = close == std::string::npos ? at : close + 1;
        }
        return line.substr(0, at) + " " + tag + line.substr(at);
    }

    std::string tabTag() const {
        if (!tag_.b || !tabTag_.b) return "";
        return brackets_.b ? "[" + tagText_.text + "]" : tagText_.text;
    }

    ImVec4 tagColor() const { return tagColor_.color; }

private:
    static size_t nameEnd(const std::string& line, const std::string& me) {
        for (size_t p = line.find(me); p != std::string::npos; p = line.find(me, p + 1)) {
            size_t end = p + me.size();
            bool left = p == 0 || !std::isalnum((unsigned char)line[p - 1]);
            bool right = end >= line.size() || !std::isalnum((unsigned char)line[end]);
            if (left && right) return end;
        }
        return std::string::npos;
    }

    Setting& tag_ = toggleSetting("tag", "Client tag behind my name", true);
    Setting& tagText_ = textSetting("tagText", "Tag text", "Mochi <3");
    Setting& tagColor_ = colorSetting("tagColor", "Tag color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& tagPos_ = choice("tagPos", "Tag position", {"Inside the name brackets", "Before the message"});
    Setting& brackets_ = toggleSetting("brackets", "Tag in brackets", true);
    Setting& tabTag_ = toggleSetting("tabTag", "Also in the Tab List", true);
    Setting& notifications_ = toggleSetting("notifications", "Notifications", true);
};
