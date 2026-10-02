local player = rt.u64(BASE + 0x11d61a70)
local opt = rt.u64(rt.u64(rt.u64(player + 0x778) + 0xb8) + 0x10 + 0x1a6 * 8)
rt.log("hidehand raw", rt.hex(opt + 0x10, 4))
WATCH = opt + 0x10
WATCHMS = 3000
