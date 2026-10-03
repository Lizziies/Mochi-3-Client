// SPDX-License-Identifier: AGPL-3.0-only
// Adapted from Flarial src/Client/Module/Modules/Misc/ScriptMarketplace/ScriptMarketplace.hpp: the Lua script and
// config store is not part of Mochi (it has its own script list and config sharing), only the call the menu makes.
#pragma once

class ScriptMarketplace {
public:
    static void reloadAllConfigs() {}
};
