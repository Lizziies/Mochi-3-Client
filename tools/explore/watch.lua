rt.run("lib")
rt.run("watchtag")
local hits = rt.watch(WATCH, WATCHMS or 3000)
local lines = {string.format("watch %x", WATCH)}
for i = 1, #hits, 2 do
    local rip, n = hits[i], hits[i + 1]
    local f = rt.func(rip)
    lines[#lines + 1] = string.format("== after %x  x%d  func %s", rip, n, f and string.format("%x +0x%x (rva %x)", f, rip - f, f - BASE) or "?")
    if f then lines[#lines + 1] = rt.disasm(math.max(f, rip - 0x30), 16) end
end
rt.out("watch.txt", table.concat(lines, "\n"))
rt.log("watch done", #hits // 2)
