rt.run("lib")
local player = rt.u64(BASE + 0x11d61a70)
local hr = rt.u64(player + 0x1e8)
local er = rt.u64(hr + 0x38)
local b, e = rt.u64(er + 0x50), rt.u64(er + 0x58)
local out = {string.format("nodes %d", (e - b) // 32)}
for n = b, e - 32, 32 do
    local key, pool = rt.u32(n + 8), rt.u64(n + 0x10)
    local vt = rt.u64(pool) or 0
    local found = {}
    -- vectors inside the pool object: (begin,end,cap) triples of heap pointers
    for k = 8, 0x80, 8 do
        local vb, ve = rt.u64(pool + k), rt.u64(pool + k + 8)
        if vb and ve and ve > vb and ve - vb < 0x100000 and vb > 0x10000 then
            -- elements may be page pointers; check both direct and one level
            for i = vb, math.min(ve, vb + 0x800) - 8, 8 do
                local q = rt.u64(i)
                if q == player then found[#found + 1] = string.format("direct vec@+%x idx %d", k, (i - vb) // 8) end
                if q and q > 0x10000 and q < 0x7fffffffffff then
                    local hits = rt.scanf and nil
                    local raw = rt.raw(q, 0x2000)
                    if raw then
                        local pos = 1
                        local pat = string.pack("<I8", player)
                        local s = raw:find(pat, 1, true)
                        if s then found[#found + 1] = string.format("page vec@+%x page %d off %x", k, (i - vb) // 8, s - 1) end
                    end
                end
            end
        end
    end
    if #found > 0 then out[#out + 1] = string.format("key %08x pool %x vt %x: %s", key, pool, vt - BASE, table.concat(found, "; ")) end
end
rt.out("pools.txt", table.concat(out, "\n"))
rt.log("pools fin")
