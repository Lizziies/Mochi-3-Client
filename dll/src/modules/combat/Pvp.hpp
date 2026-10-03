#pragma once

#include "hook/Dx.hpp"
#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"
#include "sig/Sigs.hpp"

#include <windows.h>

#include <algorithm>

class BlockHit : public Module {
public:
    BlockHit()
        : Module("Block Hit", "1.8-style sword blocking pose while you hold right click, with the swing on top.", Category::Pvp,
                 {"cosmetic"}) {
        sub("Hit feedback");
        require(0, {fx::sig(fx::Id::HandMatrix)});
    }

    void onFrame() override {
        bool held = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0 && GetForegroundWindow() == dx::window();
        blend_ += ((held ? 1.f : 0.f) - blend_) * 0.35f;
        if (blend_ < 0.01f) return;
        float k = blend_ * strength_.f;
        fx::transform(fx::Id::HandMatrix, {-0.1f * k, 0.05f * k + (lowered_.b ? -0.1f * k : 0.f), 0.08f * k},
                      {1.f - 0.15f * k, 1.f - 0.15f * k, 1.f - 0.15f * k}, {-18.f * k, 28.f * k, -12.f * k});
    }

private:
    Setting& strength_ = slider("strength", "Pose strength", 1.f, 0.3f, 1.6f, "%.2fx");
    Setting& lowered_ = toggleSetting("lowered", "Hold the item lower", true);
    float blend_ = 0.f;
};

class CrystalOptimizer : public Module {
public:
    CrystalOptimizer()
        : Module("Crystal Optimizer",
                 "A hit end crystal disappears at once instead of waiting for the server. Client side only, sends nothing extra.",
                 Category::Pvp, {"info-others", "timing"}) {
        sub("Crystal PvP");
        markRisky("Some servers count crystal tweaks as an advantage. Only use it where the server rules allow it.");
        require(need::target, need::sigs({"Target", "HideEntities"}));
    }

    void onFrame() override {
        for (auto& e : game::events()) {
            if (e.kind != game::EventKind::Hit || !e.crystal || !e.actor) continue;
            game::hide(e.actor, keep_.f / 1000.f);
            hidden_++;
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::fmt("Crystals hidden by you: {}  ·  hidden right now: {}", hidden_, game::hidden()).c_str());
    }

private:
    Setting& keep_ = slider("keep", "Keep it hidden for (ms)", 500.f, 100.f, 1500.f, "%.0f ms");
    int hidden_ = 0;
};

class KillCleanup : public Module {
public:
    KillCleanup()
        : Module("Kill Cleanup", "Hides a player right after you kill them until the game catches up. Client side only.",
                 Category::Pvp, {"info-others", "timing"}) {
        sub("Hit feedback");
        markRisky("Some servers count this as an advantage. Only use it where the server rules allow it.");
        require(0, {"KillEvents", "HideEntities"});
        words_.visible = [this] { return chat_.b && chatOk(); };
        chat_.visible = [this] { return chatOk(); };
    }

    void onFrame() override {
        auto& st = game::state();
        for (auto& e : game::events()) {
            if (e.kind == game::EventKind::Kill && own_.b && st.combat.lastActor) hide(st.combat.lastActor, e.text);
            if (e.kind == game::EventKind::Chat && chat_.b && chatOk()) deadInChat(e.text, st);
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::fmt("Hidden by you: {}  ·  hidden right now: {}", hidden_, game::hidden()).c_str());
    }

private:
    static bool chatOk() { return game::demo() || (sigs::address("ChatEvents") && sigs::address("ActorList")); }

    void hide(uintptr_t actor, const std::string&) {
        game::hide(actor, keep_.f);
        hidden_++;
    }

    void deadInChat(const std::string& raw, const game::State& st) {
        std::string line = text::lower(text::strip(raw));
        bool died = false;
        for (auto& w : text::split(text::lower(words_.text), ','))
            if (line.find(w) != std::string::npos) died = true;
        if (!died) return;
        for (auto& o : st.others) {
            if (!o.isPlayer || o.name.empty() || o.name == st.player.name) continue;
            if (line.find(text::lower(o.name)) == line.npos) continue;
            if (line.find(text::lower(o.name)) > line.size() / 2) continue;
            hide(o.id, o.name);
        }
    }

    Setting& own_ = toggleSetting("own", "Hide players you kill", true);
    Setting& chat_ = toggleSetting("chat", "Hide players who die according to chat", false);
    Setting& words_ = textSetting("words", "Death words (comma)", "was killed, was slain, was shot, was eliminated, was blown up, died");
    Setting& keep_ = slider("keep", "Keep them hidden for (s)", 2.f, 0.5f, 6.f, "%.1f s");
    int hidden_ = 0;
};
