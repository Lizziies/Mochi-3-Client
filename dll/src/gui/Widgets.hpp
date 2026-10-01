#pragma once

#include "modules/Setting.hpp"

#include <imgui.h>

#include <string>

namespace widgets {

bool toggle(const char* id, bool& value, bool enabled = true);
bool setting(Setting& s);
bool button(const char* label, ImVec2 size = {0, 0}, bool primary = false);
bool keyCapture(const char* id, int& vk);
void sectionTitle(const char* text);
void hint(const char* text);

std::string keyName(int vk);
bool capturingKey();

}
