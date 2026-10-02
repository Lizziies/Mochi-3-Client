rt.run("lib")
local h = rt.find("56 57 53 48 83 EC 60 44 0F 29 44 24 50 0F 29 7C 24 40 0F 29 74 24 30 44 89 C3 0F 28 F1 48 89 CE 45 84 C0 0F 84 ? ? ? ? 48 8B 86 ? ? ? ? 48 8B 88 ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? 48 8D 54 24 28 41 B8 2F 00 00 00", 8)
rt.out("sigcheck.txt", string.format("fov %d %s", #h, h[1] and string.format("%x", h[1] - BASE) or "-"))
rt.log("sigcheck done")
