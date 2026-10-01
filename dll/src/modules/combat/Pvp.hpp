#pragma once

#include "hook/Dx.hpp"
#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"

#include <windows.h>

#include <algorithm>

class BlockHit : public Module {
public:
    BlockHit()
        : Module("Block Hit", "1.8-style sword blocking pose while you hold right click, with the swing on top.", Category::Pvp,
                 {"cosmetic"}) {
        sub("Animations");
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
                 "Makes end crystals easier to see and hit: no spin and bobbing, no base, optionally hidden the moment you hit them. Client side only, sends nothing extra.",
                 Category::Pvp, {"info-others", "timing"}) {
        sub("Crystal PvP");
        markRisky("Some servers count crystal tweaks as an advantage. Only use it where the server rules allow it.");
        require(0, {fx::sig(fx::Id::CrystalSimple)});
    }

    void onFrame() override {
        if (still_.b) fx::force(fx::Id::CrystalSimple, true);
        if (noBase_.b) fx::force(fx::Id::CrystalNoBase, true);
        if (hideOnHit_.b) fx::force(fx::Id::CrystalHide, true);
    }

private:
    Setting& still_ = toggleSetting("still", "No spin and bobbing", true);
    Setting& noBase_ = toggleSetting("noBase", "Hide the bedrock base", true);
    Setting& hideOnHit_ = toggleSetting("hideOnHit", "Hide when hit", false);
};
