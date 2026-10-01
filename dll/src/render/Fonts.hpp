#pragma once

#include <imgui.h>

namespace fonts {

void load();

ImFont* regular();
ImFont* bold();
ImFont* hud();
float hudSize();

}
