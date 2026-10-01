#pragma once

#include "modules/Module.hpp"

#include <imgui.h>

#include <string>

namespace gui {

enum class Page { Hub, Modules, Cosmetics, Settings };

ImVec4 catColor(int cat);
void glyph(ImDrawList* dl, int kind, ImVec2 c, float r, ImU32 col);
std::string fitText(ImFont* font, float size, std::string text, float maxW);
void smoothScroll();
bool statePill(const char* id, ImVec2 min, ImVec2 max, bool on, bool locked);
void star(ImDrawList* dl, ImVec2 c, float r, ImU32 col, bool filled);
void drawLogo(ImDrawList* dl, ImVec2 p);
void drawServerChip(ImVec2 rowMin, ImVec2 rowMax, float right);

Category catOf(const Module& m);
void go(Page p);
Page page();
Module*& selectedModule();

char* searchText();
bool& showMoreModules();
bool& favoritesOnly();

void drawModulesPage(ImVec2 origin, ImVec2 size);
void drawModulePanel();
void drawSettingsPage(ImVec2 origin, ImVec2 size);
void drawCosmeticsPage(ImVec2 origin, ImVec2 size);
void reloadCosmetics();

}
