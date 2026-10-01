#include "Manager.hpp"
#include "core/Guard.hpp"
#include "core/Log.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "gui/Widgets.hpp"
#include "hook/Input.hpp"
#include "server/Rules.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"
#include "sdk/Inject.hpp"
#include "sig/Sigs.hpp"

#include "camera/Camera.hpp"
#include "client/ClickGui.hpp"
#include "client/SigStatus.hpp"
#include "combat/Counters.hpp"
#include "combat/Feedback.hpp"
#include "combat/Target.hpp"
#include "combat/Tweaks.hpp"
#include "comfort/Chat.hpp"
#include "comfort/Link.hpp"
#include "comfort/Screenshot.hpp"
#include "comfort/Streamer.hpp"
#include "comfort/Toggles.hpp"
#include "fun/BlockGame.hpp"
#include "fun/DvdScreen.hpp"
#include "fun/EyeBreak.hpp"
#include "fun/Flappy.hpp"
#include "fun/Pets.hpp"
#include "fun/Snake.hpp"
#include "hud/Clock.hpp"
#include "hud/Cps.hpp"
#include "hud/Compose.hpp"
#include "hud/Fps.hpp"
#include "hud/GameInfo.hpp"
#include "hud/Inventory.hpp"
#include "hud/Keystrokes.hpp"
#include "hud/Latency.hpp"
#include "hud/Memory.hpp"
#include "hud/MouseStrokes.hpp"
#include "hud/Pomodoro.hpp"
#include "hud/ServerInfo.hpp"
#include "hud/SessionTimer.hpp"
#include "hud/Stopwatch.hpp"
#include "input/CpsLimiter.hpp"
#include "input/InstantInput.hpp"
#include "input/NoScroll.hpp"
#include "network/LatencyBlame.hpp"
#include "network/Network.hpp"
#include "network/PingCounter.hpp"
#include "network/Probe.hpp"
#include "perf/Auto.hpp"
#include "perf/FrameLimiter.hpp"
#include "perf/LowLatency.hpp"
#include "perf/Tuning.hpp"
#include "post/Capture.hpp"
#include "post/Effects.hpp"
#include "post/FunEffects.hpp"
#include "post/PostFx.hpp"
#include "perf/RenderOptions.hpp"
#include "perf/SystemBoost.hpp"
#include "server/MatchSummary.hpp"
#include "server/ServerProfiles.hpp"
#include "visual/Crosshair.hpp"
#include "world/Waypoints.hpp"
#include "world/World.hpp"

#include <windows.h>

namespace modules {

static std::vector<std::unique_ptr<Module>> list;
static bool hudHidden = false;
static float cost = 0.f;
static Motion motion;

template <class T>
static void add() {
    list.push_back(std::make_unique<T>());
}

void init() {
    game::init();
    add<ClickGui>();

    add<Fps>();
    add<Cps>();
    add<Keystrokes>();
    add<MouseStrokes>();
    add<Clock>();
    add<Stopwatch>();
    add<Pomodoro>();
    add<SessionTimer>();
    add<Memory>();
    add<LatencyHud>();
    add<ServerInfo>();
    add<IpDisplay>();
    add<Coordinates>();
    add<DirectionHud>();
    add<SpeedDisplay>();
    add<LookAngles>();
    add<HealthDisplay>();
    add<ExperienceInfo>();
    add<DayCounter>();
    add<PackDisplay>();
    add<HeldItem>();
    add<StatsHud>();
    add<Watermark>();
    add<DebugMenu>();
    add<ArmorHud>();
    add<PotionHud>();
    add<PotCounter>();
    add<ArrowCounter>();
    add<TotemCounter>();
    add<ItemCounter>();
    add<DurabilityWarning>();
    add<LowHealth>();
    add<BetterHunger>();
    add<PingCounter>();
    add<Network>();
    add<LatencyBlame>();

    add<SaturationHue>();
    add<BrightnessContrast>();
    add<ScreenTint>();
    add<Sharpen>();
    add<DepthOfField>();
    add<ColorFilter>();
    add<NightShift>();
    add<MotionBlur>();

    add<FovChanger>();
    add<JavaDynamicFov>();
    add<Zoom>();
    add<Freelook>();
    add<NoViewBobbing>();
    add<MinimalViewBobbing>();
    add<NoHurtCam>();
    add<SmoothSneak>();
    add<AutoPerspective>();
    add<Fullbright>();
    add<BlockOutline>();
    add<TimeChanger>();
    add<WeatherChanger>();
    add<EnvironmentChanger>();
    add<FogColor>();
    add<WaterColor>();
    add<ChunkBorder>();
    add<Waypoints>();
    add<HideHand>();
    add<ViewModel>();
    add<Animations>();

    add<BreakProgress>();
    add<Crosshair>();

    add<ReachCounter>();
    add<OpponentReach>();
    add<ComboCounter>();
    add<HitCounter>();
    add<HitPing>();
    add<SessionStats>();
    add<HitInfo>();
    add<EntityCounter>();
    add<TargetHud>();
    add<Waila>();
    add<BowCharge>();
    add<CooldownIndicator>();
    add<DamageIndicator>();
    add<HitMarker>();
    add<HitEffects>();
    add<KillEffects>();
    add<HitSound>();
    add<TotemPop>();
    add<Hitbox>();
    add<HurtColor>();
    add<GlintColor>();
    add<LowFire>();
    add<ParticleMultiplier>();
    add<SensMultiplier>();
    add<BowSensitivity>();
    add<SnapLook>();
    add<NullMovement>();
    add<ItemUseDelayFix>();
    add<FasterInventory>();
    add<InstaHurtAnimation>();
    add<CpsLimiter>();
    add<NoScroll>();
    add<ToggleSprint>();
    add<ToggleSneak>();
    add<CommandHotkey>();
    add<TextHotkey>();
    add<ProfileHotkeys>();
    add<AutoGG>();
    add<MessageLogger>();
    add<ChatPlus>();
    add<DeathLogger>();
    add<PlayerNotifier>();
    add<ScoreboardPlus>();
    add<TabList>();
    add<MumbleLink>();
    add<GuiScale>();
    add<StreamerMode>();

    add<ServerProfiles>();
    add<MatchSummary>();
    add<InstantInput>();

    add<Screenshot>();

    add<LowLatency>();
    add<FrameLimiter>();
    add<RenderOptions>();
    add<AutoProfile>();
    add<BackgroundLoad>();
    add<SystemBoost>();
    add<SigStatus>();

    add<Snake>();
    add<Flappy>();
    add<DvdScreen>();
    add<EyeBreak>();
    add<BlockGame>();
    add<Pet>();
    add<Petals>();
    add<Deepfry>();
    add<UpsideDown>();

    refreshSigs();
    for (auto& m : list)
        if (m->alwaysOn()) m->setEnabled(true);

    logger::info("{} modules registered", list.size());
}

void shutdown() {
    for (auto& m : list) {
        if (m->enabled()) guard::call(m->name().c_str(), [&] { m->onDisable(); });
    }
    fx::shutdown();
    inject::shutdown();
    probe::shutdown();
    post::shutdown();
    capture::shutdown();
    game::shutdown();
}

const std::vector<std::unique_ptr<Module>>& all() { return list; }

Module* find(const std::string& name) {
    for (auto& m : list)
        if (m->name() == name) return m.get();
    return nullptr;
}

static void fault(Module& m) {
    m.setEnabled(false);
    notify::push("Modul abgeschaltet", m.name() + " hatte einen Fehler. Details im Log.", notify::Kind::Error);
}

void frame(ImDrawList* hud) {
    if (sigs::takeChanged()) refreshSigs();
    rules::tick();

    LARGE_INTEGER t0, t1, qpf;
    QueryPerformanceCounter(&t0);
    perf::begin();
    post::begin();
    fx::begin();
    guard::call("sdk", [] { game::update(); });
    input::consumeMotion(motion.x, motion.y);

    bool editing = gui::editingHud();
    for (auto& m : list) {
        if (!m->enabled()) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onFrame(); })) fault(*m);
    }

    post::submit(hud);
    capture::submit(hud, capture::Stage::Game);
    for (auto& m : list) {
        if (!m->enabled()) continue;
        if (m->isHud() && (hudHidden && !editing)) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onRender(hud); })) fault(*m);
    }

    capture::submit(hud, capture::Stage::Overlay);
    perf::apply();
    guard::call("effects", [] { fx::apply(); });
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&qpf);
    cost += (float(double(t1.QuadPart - t0.QuadPart) * 1000.0 / double(qpf.QuadPart)) - cost) * 0.05f;
}

float costMs() { return cost; }

Motion mouseDelta() { return motion; }

void dispatchKey(KeyEvent& ev) {
    if (widgets::capturingKey()) return;
    if (ev.down && !ev.repeat && ev.vk == VK_F1) hudHidden = !hudHidden;

    bool captured = gui::capturesKeyboard();
    if (ev.down && !ev.repeat && !captured) {
        for (auto& m : list) {
            int key = m->keybind().i;
            if (!key || key != ev.vk || m->alwaysOn()) continue;
            m->toggle();
            if (m->rule() == RuleLevel::Block)
                notify::push(m->name(), "Auf diesem Server nicht erlaubt.", notify::Kind::Warn);
        }
    }

    for (auto& m : list) {
        if (!m->enabled() || (captured && !m->alwaysOn())) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onKey(ev); })) fault(*m);
        if (ev.cancel) return;
    }
}

void dispatchMouse(MouseEvent& ev) {
    for (auto& m : list) {
        if (!m->enabled()) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onMouse(ev); })) fault(*m);
        if (ev.cancel) return;
    }
}

void dispatchServer(const ServerEvent& ev) {
    for (auto& m : list) {
        if (!m->enabled()) continue;
        guard::call(m->name().c_str(), [&] { m->onServer(ev); });
    }
}

void refreshSigs() {
    for (auto& m : list) {
        m->checkSigs();
        m->setEnabled(m->userEnabled());
    }
}

}
