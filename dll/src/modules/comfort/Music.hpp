#pragma once

#include "I18n.hpp"
#include "modules/HudModule.hpp"
#include "system/Music.hpp"

#include <algorithm>
#include <cmath>

class MusicControl : public HudModule {
public:
    MusicControl()
        : HudModule("Music", "Shows what Spotify, Apple Music, Deezer or Amazon Music plays and lets you control it with keys, without leaving the game.",
                    {"hud-self"}, {0.01f, 0.86f}) {
        sub("Music");
    }

    void onEnable() override { music::use(true); }
    void onDisable() override { music::use(false); }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || !ev.vk) return;
        struct Bind {
            Setting& key;
            music::Action action;
        };
        for (Bind b : {Bind{playKey_, music::Action::PlayPause}, Bind{nextKey_, music::Action::Next}, Bind{prevKey_, music::Action::Previous},
                       Bind{upKey_, music::Action::VolumeUp}, Bind{downKey_, music::Action::VolumeDown}, Bind{muteKey_, music::Action::Mute}}) {
            if (b.key.i != ev.vk) continue;
            music::send(b.action);
            ev.cancel = true;
            return;
        }
    }

    void drawSettings() override {
        auto t = music::now();
        ImGui::Spacing();
        if (!t.found) ImGui::TextDisabled("%s", i18n::tr("No music app found. Start Spotify, Apple Music, Deezer or Amazon Music."));
        else ImGui::TextDisabled(i18n::tr("Found: %s"), t.player.c_str());
        ImGui::TextDisabled("%s", i18n::tr("Keys control whichever player Windows treats as active. The volume keys change the volume of the music app only, not of Minecraft."));
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto t = music::now();
        std::string line = !t.found ? std::string(i18n::tr("No music")) : t.title.empty() ? t.player : t.title;
        if (t.found && !t.title.empty() && !t.playing) line = t.title;
        float maxW = maxWidth_.f * s;
        ImVec2 full = textSize(s, line);
        float x = 0.f;
        if (full.x > maxW) {
            float over = full.x - maxW;
            x = -std::fmod(float(ImGui::GetTime()) * 28.f * s, over + 60.f * s);
            x = std::max(x, -over);
        }
        float icon = 14.f * s;
        dl->PushClipRect(o + ImVec2(icon + 6 * s, 0), o + ImVec2(icon + 6 * s + maxW, full.y + 2), true);
        drawText(dl, o + ImVec2(icon + 6 * s + x, 0), s, line, textColor());
        dl->PopClipRect();
        drawNote(dl, o + ImVec2(0, full.y * 0.5f), icon * 0.5f, t.playing);
        return {icon + 6 * s + std::min(full.x, maxW), full.y};
    }

private:
    void drawNote(ImDrawList* dl, ImVec2 c, float r, bool playing) {
        ImU32 col = playing ? accentColor() : textColor();
        if (playing) {
            dl->AddTriangleFilled({c.x - r * 0.5f, c.y - r}, {c.x - r * 0.5f, c.y + r}, {c.x + r, c.y}, col);
        } else {
            dl->AddRectFilled({c.x - r * 0.7f, c.y - r}, {c.x - r * 0.15f, c.y + r}, col);
            dl->AddRectFilled({c.x + r * 0.15f, c.y - r}, {c.x + r * 0.7f, c.y + r}, col);
        }
    }

    Setting& maxWidth_ = slider("maxWidth", "Width", 260.f, 120.f, 600.f, "%.0f");
    Setting& playKey_ = keySetting("playKey", "Play / pause", 0);
    Setting& nextKey_ = keySetting("nextKey", "Next song", 0);
    Setting& prevKey_ = keySetting("prevKey", "Previous song", 0);
    Setting& upKey_ = keySetting("upKey", "Music volume up", 0);
    Setting& downKey_ = keySetting("downKey", "Music volume down", 0);
    Setting& muteKey_ = keySetting("muteKey", "Mute music", 0);
};
