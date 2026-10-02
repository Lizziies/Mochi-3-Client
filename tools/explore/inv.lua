rt.run("lib")
local player = rt.u64(BASE + 0x11d61a70)
local o = rt.u64(rt.u64(player + 0x4e0) + 0x7e0)
local lines = {string.format("obj %x", o)}
for k = 0, 0x7f8, 8 do
    local q = rt.u64(o + k) or 0
    local txt = ""
    if q > 0x10000 then local s = rt.cstr(q, 24); if s and #s > 2 and s:match("^[%w_:%.]+$") then txt = " str:" .. s end end
    lines[#lines + 1] = string.format("+0x%03x %016x%s", k, q, txt)
end
rt.out("inv.txt", table.concat(lines, "\n"))
rt.log("inv done")
