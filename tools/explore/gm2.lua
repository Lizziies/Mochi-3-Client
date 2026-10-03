rt.run("lib")
local a = 0x196e6bcf2c8
local out = {}
for _, r in ipairs(rt.heapr(a - 0x1000, a + 0x110, 30)) do
    local v = rt.u64(r)
    local line = {}
    for back = 0, 0x80, 8 do local q = rt.u64(r - back); if q and q >= BASE and q < BASE + 0x14000000 then line[#line + 1] = string.format("-0x%x:%x", back, q - BASE) end end
    out[#out + 1] = string.format("at %x -> %x (a%+d) | %s", r, v, v - a, table.concat(line, " "))
end
rt.out("gm2.txt", table.concat(out, "\n"))
rt.log("gm2 fin")
