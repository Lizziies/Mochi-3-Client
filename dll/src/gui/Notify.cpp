#include "Notify.hpp"
#include "Theme.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <imgui.h>

#include <algorithm>
#include <deque>
#include <mutex>

namespace notify {

struct Toast {
    std::string title;
    std::string body;
    Kind kind;
    float life;
    float age = 0.f;
    float slide = 0.f;
    float y = -1.f;
};

static std::mutex lock;
static std::deque<Toast> toasts;
static bool muted = false;

void setMuted(bool on) { muted = on; }

void push(std::string title, std::string body, Kind kind, float seconds) {
    if (muted && kind != Kind::Error) return;
    std::scoped_lock g(lock);
    toasts.push_back({std::move(title), std::move(body), kind, seconds});
    while (toasts.size() > 5) toasts.pop_front();
}

static ImVec4 tint(Kind k) {
    auto& t = theme::current();
    switch (k) {
    case Kind::Ok: return t.ok;
    case Kind::Warn: return t.warn;
    case Kind::Error: return {1.f, 0.42f, 0.48f, 1.f};
    default: return t.accent;
    }
}

void draw() {
    std::scoped_lock g(lock);
    if (toasts.empty()) return;

    auto* dl = ImGui::GetForegroundDrawList();
    auto ds = ImGui::GetIO().DisplaySize;
    float s = ui::scale();
    float w = 300.f * s, pad = 12.f * s, gap = 8.f * s;
    float titleSize = 17.f * s, bodySize = 15.f * s;
    auto& t = theme::current();

    float y = 16.f * s;
    for (auto& toast : toasts) {
        toast.age += ui::dt();
        bool leaving = toast.age > toast.life;
        toast.slide = draw::approach(toast.slide, leaving ? 0.f : 1.f, 14.f);
        if (toast.y < 0) toast.y = y;
        toast.y = draw::approach(toast.y, y, 16.f);

        ImVec2 bodySz = fonts::regular()->CalcTextSizeA(bodySize, FLT_MAX, w - pad * 2 - 10 * s, toast.body.c_str());
        float h = pad * 2 + titleSize + (toast.body.empty() ? 0 : bodySz.y + 2 * s);
        float x = 16.f * s - (w + 16.f * s) * (1.f - draw::easeOutCubic(toast.slide));
        ImVec2 min{x, toast.y}, max{x + w, toast.y + h};
        float a = toast.slide;

        dl->AddRectFilled(min + ImVec2(0, 3 * s), max + ImVec2(0, 3 * s), IM_COL32(0, 0, 0, int(60 * a)), t.rounding * s);
        dl->AddRectFilled(min, max, theme::col(t.surface, 0.97f * a), t.rounding * s);
        ImVec4 c = tint(toast.kind);
        dl->AddRectFilled(min, {min.x + 4 * s, max.y}, theme::col(c, a), t.rounding * s, ImDrawFlags_RoundCornersLeft);

        float progress = std::clamp(1.f - toast.age / toast.life, 0.f, 1.f);
        dl->AddRectFilled({min.x + 4 * s, max.y - 2 * s}, {min.x + 4 * s + (w - 4 * s) * progress, max.y},
                          theme::col(c, 0.6f * a));

        draw::heart(dl, {min.x + pad + 8 * s, min.y + pad + titleSize * 0.5f}, 14 * s, theme::col(c, a));
        dl->AddText(fonts::bold(), titleSize, {min.x + pad + 20 * s, min.y + pad}, theme::col(t.text, a),
                    toast.title.c_str());
        if (!toast.body.empty())
            dl->AddText(fonts::regular(), bodySize, {min.x + pad + 10 * s, min.y + pad + titleSize + 2 * s},
                        theme::col(t.textDim, a), toast.body.c_str(), nullptr, w - pad * 2 - 10 * s);

        y += h + gap;
    }

    while (!toasts.empty() && toasts.front().age > toasts.front().life && toasts.front().slide <= 0.01f)
        toasts.pop_front();
}

}
