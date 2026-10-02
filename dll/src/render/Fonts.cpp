#include "Fonts.hpp"
#include "core/Client.hpp"
#include "core/Log.hpp"
#include "Ui.hpp"

#include "../../res/resource.h"

#include <windows.h>

namespace fonts {

static ImFont* reg = nullptr;
static ImFont* bld = nullptr;

static ImFont* fromResource(int id, float size) {
    HMODULE self = client::module();
    HRSRC res = FindResourceW(self, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10));
    if (!res) return nullptr;
    HGLOBAL data = LoadResource(self, res);
    void* bytes = data ? LockResource(data) : nullptr;
    int len = (int)SizeofResource(self, res);
    if (!bytes || !len) return nullptr;

    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 2;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(bytes, len, size, &cfg);
}

void load() {
    auto& io = ImGui::GetIO();
    reg = fromResource(IDR_FONT_REGULAR, 18.f);
    bld = fromResource(IDR_FONT_BOLD, 18.f);
    if (!reg) {
        logger::warn("embedded font missing, falling back to default");
        reg = io.Fonts->AddFontDefault();
    }
    if (!bld) bld = reg;
    io.FontDefault = reg;
}

ImFont* regular() { return reg; }
ImFont* bold() { return bld; }
ImFont* hud() { return bld; }
float hudSize() { return 18.f * ui::scale(); }

}
