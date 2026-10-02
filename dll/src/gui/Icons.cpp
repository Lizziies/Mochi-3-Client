#include "Icons.hpp"

#include "render/Draw.hpp"

#include <algorithm>
#include <cmath>

namespace gui {

namespace {

constexpr float pi = 3.14159265f;

struct Pen {
    ImDrawList* dl;
    ImVec2 c;
    float r;
    ImU32 col;
    float w;

    ImVec2 at(float x, float y) const { return {c.x + x * r, c.y + y * r}; }
    void line(float x0, float y0, float x1, float y1, float k = 1.f) const { dl->AddLine(at(x0, y0), at(x1, y1), col, w * k); }
    void circle(float x, float y, float rr, float k = 1.f) const { dl->AddCircle(at(x, y), rr * r, col, 0, w * k); }
    void dot(float x, float y, float rr) const { dl->AddCircleFilled(at(x, y), rr * r, col); }
    void rect(float x0, float y0, float x1, float y1, float round = 0.f) const { dl->AddRect(at(x0, y0), at(x1, y1), col, round * r, 0, w); }
    void fill(float x0, float y0, float x1, float y1, float round = 0.f) const { dl->AddRectFilled(at(x0, y0), at(x1, y1), col, round * r); }
    void arc(float x, float y, float rr, float a0, float a1) const {
        dl->PathArcTo(at(x, y), rr * r, a0, a1, 20);
        dl->PathStroke(col, 0, w);
    }
    void poly(std::initializer_list<ImVec2> pts, bool closed = true) const {
        for (auto& p : pts) dl->PathLineTo(at(p.x, p.y));
        dl->PathStroke(col, closed ? ImDrawFlags_Closed : 0, w);
    }
    void solid(std::initializer_list<ImVec2> pts) const {
        ImVec2 buf[12];
        int n = 0;
        for (auto& p : pts)
            if (n < 12) buf[n++] = at(p.x, p.y);
        dl->AddConvexPolyFilled(buf, n, col);
    }
};

}

void icon(ImDrawList* dl, Icon kind, ImVec2 center, float radius, ImU32 color) {
    Pen p{dl, center, radius, color, std::max(1.3f, radius * 0.13f)};
    switch (kind) {
    case Icon::Sword:
        p.line(-0.45f, 0.45f, 0.82f, -0.82f, 1.2f);
        p.line(0.82f, -0.82f, 0.5f, -0.78f);
        p.line(0.82f, -0.82f, 0.78f, -0.5f);
        p.line(-0.72f, 0.12f, -0.12f, 0.72f, 1.2f);
        p.line(-0.42f, 0.42f, -0.85f, 0.85f, 1.4f);
        break;
    case Icon::Crystal:
        p.poly({{0.f, -1.f}, {0.78f, -0.15f}, {0.f, 1.f}, {-0.78f, -0.15f}});
        p.line(-0.78f, -0.15f, 0.78f, -0.15f);
        p.line(-0.32f, -0.15f, 0.f, -1.f);
        p.line(0.32f, -0.15f, 0.f, -1.f);
        p.line(-0.32f, -0.15f, 0.f, 1.f);
        p.line(0.32f, -0.15f, 0.f, 1.f);
        break;
    case Icon::Shield:
        p.poly({{-0.78f, -0.82f}, {0.78f, -0.82f}, {0.78f, 0.05f}, {0.f, 0.95f}, {-0.78f, 0.05f}});
        p.line(0.f, -0.82f, 0.f, 0.95f);
        break;
    case Icon::Bolt: {
        ImVec2 pts[6] = {p.at(0.2f, -1.f), p.at(-0.62f, 0.12f), p.at(-0.04f, 0.12f), p.at(-0.2f, 1.f), p.at(0.64f, -0.2f), p.at(0.06f, -0.2f)};
        dl->AddConcavePolyFilled(pts, 6, color);
        break;
    }
    case Icon::Run:
        p.poly({{-0.85f, -0.7f}, {-0.15f, 0.f}, {-0.85f, 0.7f}}, false);
        p.poly({{0.05f, -0.7f}, {0.75f, 0.f}, {0.05f, 0.7f}}, false);
        break;
    case Icon::Box:
        p.rect(-0.85f, -0.35f, 0.35f, 0.85f);
        p.poly({{-0.85f, -0.35f}, {-0.35f, -0.85f}, {0.85f, -0.85f}, {0.35f, -0.35f}}, false);
        p.poly({{0.85f, -0.85f}, {0.85f, 0.35f}, {0.35f, 0.85f}}, false);
        break;
    case Icon::Drop:
        p.arc(0.f, 0.28f, 0.62f, -0.35f, pi + 0.35f);
        p.line(-0.58f, 0.06f, 0.f, -0.95f);
        p.line(0.58f, 0.06f, 0.f, -0.95f);
        break;
    case Icon::Swing:
        p.arc(0.f, 0.25f, 0.85f, pi * 1.05f, pi * 1.85f);
        p.line(0.6f, -0.35f, 0.78f, 0.1f);
        p.line(0.6f, -0.35f, 0.15f, -0.25f);
        p.line(-0.5f, 0.9f, 0.1f, 0.3f, 1.2f);
        break;
    case Icon::Target:
        p.circle(0.f, 0.f, 0.9f);
        p.circle(0.f, 0.f, 0.48f);
        p.dot(0.f, 0.f, 0.16f);
        break;
    case Icon::Potion:
        p.arc(0.f, 0.35f, 0.62f, -pi * 0.33f, pi * 1.33f);
        p.line(-0.3f, -0.18f, -0.3f, -0.7f);
        p.line(0.3f, -0.18f, 0.3f, -0.7f);
        p.line(-0.45f, -0.75f, 0.45f, -0.75f, 1.3f);
        p.line(-0.4f, 0.35f, 0.4f, 0.35f);
        break;
    case Icon::Heart: draw::heart(dl, center, radius * 1.8f, color); break;
    case Icon::Spark:
        for (int i = 0; i < 4; i++) {
            float a = pi * 0.25f + i * pi * 0.5f;
            p.line(std::cos(a) * 0.35f, std::sin(a) * 0.35f, std::cos(a) * 0.95f, std::sin(a) * 0.95f, 1.2f);
        }
        p.dot(0.f, 0.f, 0.12f);
        break;
    case Icon::Mouse:
        p.rect(-0.58f, -0.92f, 0.58f, 0.92f, 0.56f);
        p.line(0.f, -0.92f, 0.f, -0.28f);
        p.line(-0.58f, -0.28f, 0.58f, -0.28f);
        break;
    case Icon::Timer:
        p.circle(0.f, 0.15f, 0.78f);
        p.line(0.f, 0.15f, 0.f, -0.3f);
        p.line(0.f, 0.15f, 0.32f, 0.32f);
        p.line(-0.25f, -0.88f, 0.25f, -0.88f, 1.3f);
        break;
    case Icon::Keyboard:
        p.rect(-0.98f, -0.6f, 0.98f, 0.6f, 0.18f);
        for (int row = 0; row < 2; row++)
            for (int col = 0; col < 4; col++) p.dot(-0.6f + col * 0.4f, -0.22f + row * 0.32f, 0.07f);
        p.line(-0.4f, 0.36f, 0.4f, 0.36f);
        break;
    case Icon::Gauge:
        p.arc(0.f, 0.3f, 0.88f, pi, pi * 2.f);
        p.line(0.f, 0.3f, 0.48f, -0.25f, 1.2f);
        p.dot(0.f, 0.3f, 0.14f);
        p.line(-0.88f, 0.3f, -0.6f, 0.3f);
        p.line(0.88f, 0.3f, 0.6f, 0.3f);
        break;
    case Icon::Armor:
        p.poly({{-0.92f, -0.62f}, {-0.36f, -0.88f}, {0.f, -0.58f}, {0.36f, -0.88f}, {0.92f, -0.62f}, {0.66f, -0.1f}, {0.6f, 0.9f},
                {-0.6f, 0.9f}, {-0.66f, -0.1f}});
        break;
    case Icon::Compass:
        p.circle(0.f, 0.f, 0.92f);
        p.solid({{0.f, -0.62f}, {0.2f, 0.f}, {-0.2f, 0.f}});
        p.poly({{0.f, 0.62f}, {0.2f, 0.f}, {-0.2f, 0.f}});
        break;
    case Icon::Bag:
        p.rect(-0.9f, -0.45f, 0.9f, 0.82f, 0.12f);
        p.line(-0.9f, 0.05f, 0.9f, 0.05f);
        p.fill(-0.16f, -0.08f, 0.16f, 0.22f, 0.05f);
        p.poly({{-0.55f, -0.45f}, {-0.45f, -0.82f}, {0.45f, -0.82f}, {0.55f, -0.45f}}, false);
        break;
    case Icon::Clock:
        p.circle(0.f, 0.f, 0.9f);
        p.line(0.f, 0.f, 0.f, -0.55f);
        p.line(0.f, 0.f, 0.42f, 0.2f);
        break;
    case Icon::Layers:
        for (int i = 0; i < 3; i++) {
            float y = -0.48f + i * 0.48f;
            p.poly({{-0.95f, y}, {0.f, y - 0.36f}, {0.95f, y}, {0.f, y + 0.36f}});
        }
        break;
    case Icon::Info:
        p.circle(0.f, 0.f, 0.92f);
        p.dot(0.f, -0.42f, 0.12f);
        p.line(0.f, -0.1f, 0.f, 0.5f, 1.2f);
        break;
    case Icon::Signal:
        for (int i = 0; i < 4; i++) p.fill(-0.85f + i * 0.47f, 0.8f - (i + 1) * 0.4f, -0.55f + i * 0.47f, 0.8f, 0.06f);
        break;
    case Icon::Bell:
        p.arc(0.f, -0.2f, 0.58f, pi, pi * 2.f);
        p.line(-0.58f, -0.2f, -0.7f, 0.5f);
        p.line(0.58f, -0.2f, 0.7f, 0.5f);
        p.line(-0.9f, 0.5f, 0.9f, 0.5f);
        p.dot(0.f, 0.78f, 0.15f);
        p.line(0.f, -0.78f, 0.f, -0.95f);
        break;
    case Icon::Zoom:
        p.circle(-0.18f, -0.18f, 0.62f);
        p.line(0.28f, 0.28f, 0.88f, 0.88f, 1.5f);
        break;
    case Icon::Sun:
        p.circle(0.f, 0.f, 0.4f);
        for (int i = 0; i < 8; i++) {
            float a = i * pi * 0.25f;
            p.line(std::cos(a) * 0.62f, std::sin(a) * 0.62f, std::cos(a) * 0.95f, std::sin(a) * 0.95f);
        }
        break;
    case Icon::Crosshair:
        p.circle(0.f, 0.f, 0.6f);
        p.line(0.f, -1.f, 0.f, -0.32f);
        p.line(0.f, 1.f, 0.f, 0.32f);
        p.line(-1.f, 0.f, -0.32f, 0.f);
        p.line(1.f, 0.f, 0.32f, 0.f);
        break;
    case Icon::Cube:
        p.poly({{0.f, -0.95f}, {0.85f, -0.48f}, {0.85f, 0.48f}, {0.f, 0.95f}, {-0.85f, 0.48f}, {-0.85f, -0.48f}});
        p.poly({{-0.85f, -0.48f}, {0.f, 0.f}, {0.85f, -0.48f}}, false);
        p.line(0.f, 0.f, 0.f, 0.95f);
        break;
    case Icon::Camera:
        p.rect(-0.95f, -0.5f, 0.95f, 0.75f, 0.15f);
        p.circle(0.f, 0.12f, 0.36f);
        p.poly({{-0.4f, -0.5f}, {-0.25f, -0.8f}, {0.25f, -0.8f}, {0.4f, -0.5f}}, false);
        break;
    case Icon::Eye:
        p.arc(0.f, 0.55f, 1.05f, pi * 1.2f, pi * 1.8f);
        p.arc(0.f, -0.55f, 1.05f, pi * 0.2f, pi * 0.8f);
        p.dot(0.f, 0.f, 0.26f);
        break;
    case Icon::Pickaxe:
        p.arc(0.f, 0.55f, 1.05f, pi * 1.22f, pi * 1.78f);
        p.line(0.f, -0.5f, 0.f, 0.95f, 1.3f);
        break;
    case Icon::Palette:
        p.circle(0.f, 0.f, 0.92f);
        p.dot(-0.38f, -0.25f, 0.15f);
        p.dot(0.1f, -0.48f, 0.15f);
        p.dot(0.45f, -0.08f, 0.15f);
        p.dot(-0.2f, 0.38f, 0.15f);
        break;
    case Icon::Sparkle: draw::sparkle(dl, center, radius, color); break;
    case Icon::Cloud:
        dl->AddCircleFilled(p.at(-0.4f, 0.15f), 0.4f * radius, color);
        dl->AddCircleFilled(p.at(0.12f, -0.12f), 0.55f * radius, color);
        dl->AddCircleFilled(p.at(0.55f, 0.22f), 0.35f * radius, color);
        p.fill(-0.4f, 0.15f, 0.55f, 0.57f);
        break;
    case Icon::Tag:
        p.poly({{-0.9f, -0.9f}, {0.05f, -0.9f}, {0.92f, -0.03f}, {-0.03f, 0.92f}, {-0.9f, 0.05f}});
        p.dot(-0.45f, -0.45f, 0.14f);
        break;
    case Icon::Pin:
        p.arc(0.f, -0.25f, 0.6f, pi * 0.82f, pi * 2.18f);
        p.line(-0.52f, 0.03f, 0.f, 0.95f);
        p.line(0.52f, 0.03f, 0.f, 0.95f);
        p.dot(0.f, -0.25f, 0.18f);
        break;
    case Icon::Chat:
        p.rect(-0.95f, -0.75f, 0.95f, 0.45f, 0.25f);
        p.poly({{-0.5f, 0.45f}, {-0.55f, 0.9f}, {-0.05f, 0.45f}}, false);
        break;
    case Icon::Trophy:
        p.poly({{-0.55f, -0.85f}, {0.55f, -0.85f}, {0.48f, -0.1f}, {0.f, 0.25f}, {-0.48f, -0.1f}});
        p.arc(-0.6f, -0.5f, 0.3f, pi * 0.5f, pi * 1.5f);
        p.arc(0.6f, -0.5f, 0.3f, -pi * 0.5f, pi * 0.5f);
        p.line(0.f, 0.25f, 0.f, 0.65f);
        p.line(-0.45f, 0.85f, 0.45f, 0.85f, 1.3f);
        break;
    case Icon::Mask:
        p.dot(0.f, 0.f, 0.2f);
        p.arc(0.f, 0.f, 0.55f, -pi * 0.3f, pi * 0.3f);
        p.arc(0.f, 0.f, 0.55f, pi * 0.7f, pi * 1.3f);
        p.arc(0.f, 0.f, 0.92f, -pi * 0.3f, pi * 0.3f);
        p.arc(0.f, 0.f, 0.92f, pi * 0.7f, pi * 1.3f);
        break;
    case Icon::Wrench:
        p.line(-0.75f, 0.75f, 0.2f, -0.2f, 1.5f);
        p.arc(0.45f, -0.45f, 0.42f, pi * 0.95f, pi * 2.55f);
        break;
    case Icon::Plug:
        p.line(-0.32f, -0.95f, -0.32f, -0.45f);
        p.line(0.32f, -0.95f, 0.32f, -0.45f);
        p.rect(-0.62f, -0.45f, 0.62f, 0.2f, 0.12f);
        p.arc(0.f, 0.2f, 0.62f, 0.f, pi);
        p.line(0.f, 0.82f, 0.f, 1.f);
        break;
    case Icon::Chip:
        p.rect(-0.55f, -0.55f, 0.55f, 0.55f, 0.12f);
        p.fill(-0.2f, -0.2f, 0.2f, 0.2f);
        for (int i = -1; i <= 1; i++) {
            float o = i * 0.3f;
            p.line(o, -0.55f, o, -0.9f);
            p.line(o, 0.55f, o, 0.9f);
            p.line(-0.55f, o, -0.9f, o);
            p.line(0.55f, o, 0.9f, o);
        }
        break;
    case Icon::Chart:
        p.fill(-0.85f, 0.1f, -0.45f, 0.85f, 0.08f);
        p.fill(-0.2f, -0.4f, 0.2f, 0.85f, 0.08f);
        p.fill(0.45f, -0.85f, 0.85f, 0.85f, 0.08f);
        break;
    case Icon::Hex: {
        dl->PathClear();
        for (int i = 0; i < 6; i++) {
            float a = pi / 6.f + i * pi / 3.f;
            dl->PathLineTo(p.at(std::cos(a) * 0.92f, std::sin(a) * 0.92f));
        }
        dl->PathStroke(color, ImDrawFlags_Closed, p.w);
        p.dot(0.f, 0.f, 0.22f);
        break;
    }
    case Icon::Server:
        p.rect(-0.9f, -0.85f, 0.9f, -0.08f, 0.15f);
        p.rect(-0.9f, 0.08f, 0.9f, 0.85f, 0.15f);
        p.dot(-0.52f, -0.46f, 0.11f);
        p.dot(-0.52f, 0.46f, 0.11f);
        break;
    case Icon::Game:
        p.rect(-0.98f, -0.5f, 0.98f, 0.6f, 0.45f);
        p.line(-0.62f, 0.05f, -0.22f, 0.05f);
        p.line(-0.42f, -0.15f, -0.42f, 0.25f);
        p.dot(0.35f, -0.05f, 0.1f);
        p.dot(0.6f, 0.17f, 0.1f);
        break;
    case Icon::Grid:
        for (int i = 0; i < 4; i++) {
            float x = (i & 1) ? 0.12f : -0.82f, y = (i & 2) ? 0.12f : -0.82f;
            p.fill(x, y, x + 0.7f, y + 0.7f, 0.15f);
        }
        break;
    case Icon::Star: {
        dl->PathClear();
        for (int i = 0; i < 10; i++) {
            float a = -pi * 0.5f + i * pi / 5.f;
            float rr = i % 2 ? 0.42f : 0.98f;
            dl->PathLineTo(p.at(std::cos(a) * rr, std::sin(a) * rr));
        }
        dl->PathStroke(color, ImDrawFlags_Closed, p.w);
        break;
    }
    case Icon::Lock:
        p.fill(-0.68f, -0.1f, 0.68f, 0.9f, 0.15f);
        p.arc(0.f, -0.1f, 0.45f, pi, pi * 2.f);
        break;
    case Icon::Gear:
    default:
        p.circle(0.f, 0.f, 0.45f);
        for (int i = 0; i < 8; i++) {
            float a = i * pi * 0.25f;
            p.line(std::cos(a) * 0.68f, std::sin(a) * 0.68f, std::cos(a) * 0.98f, std::sin(a) * 0.98f, 1.5f);
        }
        break;
    }
}

}
