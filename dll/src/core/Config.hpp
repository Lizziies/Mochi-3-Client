#pragma once

#include <string>
#include <vector>

namespace config {

void load();
void save();
void saveLater();
void saveIfDirty();
void tick();
void markDirty();

const std::string& profile();
std::vector<std::string> profiles();
void switchProfile(const std::string& name);
void deleteProfile(const std::string& name);

}
