rt.run("lib")
local lines = {}
do local h = rt.find("48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 30 48 8B 01 48 8B 40 08 48 8D 54 24 28 41 B8 32 00 00 00 FF 15 ? ? ? ? 48 8B 4C 24 28 48 89 C8 48 8B 49 08 48 8B 89 ? ? ? ? 48 85 C9 75 ED F3 0F 10 40 18", 8); lines[#lines + 1] = string.format("fx.gamma %d %s", #h, h[1] and string.format("%x", h[1] - BASE) or "-") end
do local h = rt.find("48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 30 48 8B 01 48 8B 40 08 48 8D 54 24 28 41 B8 03 00 00 00 FF 15 ? ? ? ? 48 8B 4C 24 28 48 89 C8 48 8B 49 08 48 8B 89 ? ? ? ? 48 85 C9 75 ED 8B 40 18", 8); lines[#lines + 1] = string.format("fx.perspective %d %s", #h, h[1] and string.format("%x", h[1] - BASE) or "-") end
do local h = rt.find("48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 30 48 8B 01 48 8B 40 08 48 8D 54 24 28 41 B8 26 00 00 00 FF 15 ? ? ? ? 48 8B 4C 24 28 48 89 C8 48 8B 49 08 48 8B 89 ? ? ? ? 48 85 C9 75 ED 0F B6 40 10", 8); lines[#lines + 1] = string.format("fx.viewBob %d %s", #h, h[1] and string.format("%x", h[1] - BASE) or "-") end
do local h = rt.find("48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 30 48 8B 01 48 8B 40 08 48 8D 54 24 28 41 B8 A6 01 00 00 FF 15 ? ? ? ? 48 8B 4C 24 28 48 89 C8 48 8B 49 08 48 8B 89 ? ? ? ? 48 85 C9 75 ED 0F B6 40 10", 8); lines[#lines + 1] = string.format("fx.hideHand %d %s", #h, h[1] and string.format("%x", h[1] - BASE) or "-") end
do local h = rt.find("48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 30 48 8B 01 48 8B 40 08 48 8D 54 24 28 41 B8 8A 01 00 00 FF 15 ? ? ? ? 48 8B 4C 24 28 48 89 C8 48 8B 49 08 48 8B 89 ? ? ? ? 48 85 C9 75 ED 0F B6 40 10", 8); lines[#lines + 1] = string.format("fx.clouds %d %s", #h, h[1] and string.format("%x", h[1] - BASE) or "-") end
rt.out("sigcheck.txt", table.concat(lines, "\n"))
rt.log("sigcheck done")

