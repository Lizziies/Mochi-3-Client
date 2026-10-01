local base = rt.base()
local names = {
    "LocalPlayer", "Player", "Mob", "Actor", "ServerPlayer", "ClientInstance", "MinecraftGame", "Minecraft",
    "Level", "ClientLevel", "GameMode", "SurvivalMode", "CreativeMode", "Options", "ItemStack", "Inventory",
    "PlayerInventory", "ScreenView", "LevelRenderer", "GameRenderer", "BlockSource", "Dimension", "MinecraftScreenModel",
    "ItemRenderer", "ActorRenderDispatcher", "PlayerRenderer", "HumanoidMobRenderer", "ActorRenderer",
}
for _, n in ipairs(names) do
    local list = rt.rtti(n)
    if #list == 0 then
        rt.log(n, "no rtti")
    end
    for _, e in ipairs(list) do
        rt.log(n, string.format("vtable rva %x offset %d entries %d", e.vtable - base, e.offset, #rt.vtable(e.vtable, 600)))
    end
end
rt.log("done")
