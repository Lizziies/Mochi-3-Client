#include "Profile.hpp"
#include "core/Paths.hpp"
#include "render/Ui.hpp"

#include <imgui.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace gui::profile {

static Stats current, shown;
static float body = 0.f;
static float probeY = 0.f;
static float scrollY = 0.f;
static std::string label;
static FILE* out = nullptr;
static int frames = 0;
static bool wanted = std::getenv("MONCHI_PROFILE") != nullptr;
static double freq = [] {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return double(f.QuadPart);
}();

const Stats& stats() { return shown; }

bool recording() { return wanted; }

double stamp() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return double(t.QuadPart) * 1e6 / freq;
}

double since(double from) { return stamp() - from; }

void row() { current.rows++; }

void height(float h) { body = h; }

void probe(float y, float scroll) {
    probeY = y;
    scrollY = scroll;
}

void phase(const char* name) { label = name; }

void menu(float listUs, float detailsUs) {
    current.listUs = listUs;
    current.detailsUs = detailsUs;
    current.seen = ui::time();
}

void frame(float frameUs) { current.frameUs = frameUs; }

void finish(float dt) {
    auto* data = ImGui::GetDrawData();
    current.windows = ImGui::GetIO().MetricsRenderWindows;
    current.vertices = data ? data->TotalVtxCount : 0;
    current.cmds = 0;
    if (data)
        for (int i = 0; i < data->CmdListsCount; i++) current.cmds += data->CmdLists[i]->CmdBuffer.Size;

    if (current.seen > 0.0 && ui::time() - current.seen < 1.0) {
        float k = 0.1f;
        shown.listUs += (current.listUs - shown.listUs) * k;
        shown.detailsUs += (current.detailsUs - shown.detailsUs) * k;
        shown.frameUs += (current.frameUs - shown.frameUs) * k;
        shown.rows = current.rows;
        shown.cmds = current.cmds;
        shown.vertices = current.vertices;
        shown.windows = current.windows;
        shown.seen = current.seen;
    }

    if (wanted && current.seen > 0.0 && ui::time() - current.seen < 1.0) {
        if (!out) {
            auto file = paths::logs() / L"profile.csv";
            out = _wfopen(file.c_str(), L"w");
            if (out) std::fputs("frame,time,dt_ms,frame_us,list_us,details_us,rows,cmds,vertices,windows,body_h,probe_y,scroll_y,phase\n", out);
        }
        if (out) {
            std::fprintf(out, "%d,%.4f,%.3f,%.1f,%.1f,%.1f,%d,%d,%d,%d,%.2f,%.2f,%.2f,%s\n", frames, ui::time(), dt * 1000.f, current.frameUs, current.listUs,
                         current.detailsUs, current.rows, current.cmds, current.vertices, current.windows, body, probeY, scrollY, label.c_str());
            if (++frames % 120 == 0) std::fflush(out);
        }
    }
    current.rows = 0;
    body = 0.f;
}

}
