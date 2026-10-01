local base = rt.base()
for _, h in ipairs(rt.bytes(".?AV", false, 8)) do
    rt.log(string.format("%x", h - base), rt.cstr(h, 80))
end
for _, h in ipairs(rt.bytes("Player@@", false, 12)) do
    rt.log("Player@@ at", string.format("%x", h - base), rt.cstr(h - 24, 80))
end
local hits = rt.bytes(".?AV", false, 100000)
rt.log("total .?AV names", #hits)
rt.log("done")
