rt.run("lib")
local out = {}
local function str(at)
    local len, cap = rt.u64(at + 16) or 0, rt.u64(at + 24) or 0
    if len == 0 or len > 300 or cap < len or cap > 0x10000 then return nil end
    return rt.cstr(cap > 15 and rt.u64(at) or at, math.min(len, 60))
end
for _, a in ipairs({0x196e6bcf2c8, 0x196bf765ff0}) do
    out[#out + 1] = string.format("== %x", a)
    for k = -0x100, 0x100, 8 do
        local s = str(a + k)
        local q = rt.u64(a + k) or 0
        local note = s and ("str '" .. s .. "'") or ""
        if q >= BASE and q < BASE + 0x14000000 then note = note .. string.format(" rva %x", q - BASE) end
        out[#out + 1] = string.format("  %5d %016x %s", k, q, note)
    end
end
rt.out("gm.txt", table.concat(out, "\n"))
rt.log("gm fin")
