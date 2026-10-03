rt.run("lib")
local targets = {0x29345a38c28 - 0x198, 0x290ed1df3b8 - 0x198}
local player = rt.u64(BASE + 0x11d61a70)
local nodes, seen = {{addr = player, path = "player", depth = 0}}, {[player] = true}
local head, lines = 1, {}
while head <= #nodes and #nodes < 150000 do
    local n = nodes[head]; head = head + 1
    local span = n.depth == 0 and 0x2000 or 0x1000
    for _, t in ipairs(targets) do
        if t >= n.addr and t < n.addr + 8 then lines[#lines + 1] = string.format("%s  contains %x at +0x%x", n.path, t, t - n.addr) end
    end
    if n.depth < 5 then
        for _, p in ipairs(rt.pointers(n.addr, span)) do
            if not seen[p[2]] then
                seen[p[2]] = true
                nodes[#nodes + 1] = {addr = p[2], path = string.format("%s>0x%x", n.path, p[1]), depth = n.depth + 1}
            end
        end
    end
end
lines[#lines + 1] = "nodes " .. #nodes
rt.out("invpath.txt", table.concat(lines, "\n"))
rt.log("invpath fin", #lines)
