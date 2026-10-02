local player = rt.u64(BASE + 0x11d61a70)
WATCH = rt.u64(rt.u64(player + 0x408) + 0xa88) + 0xa0
WATCHMS = 2000
NODIS = false
rt.log("time now", rt.i32(WATCH))
