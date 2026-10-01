-- Shows your combo as a number with a small bar that empties after 1.5 seconds.
local last = 0

mochi.on("hit", function(e)
  last = mochi.time()
end)

mochi.on("tick", function(dt)
  local c = mochi.combat()
  if c.combo > 0 then
    local left = math.max(0, 1 - (mochi.time() - last) / 1.5)
    mochi.hud.text("combo", "Combo " .. c.combo, 0.5, 0.60, { color = 0xFF7DB5, scale = 1.4, align = "center" })
    mochi.hud.bar("combo_bar", 0.47, 0.645, 90, 5, left, { color = 0xFF7DB5 })
  else
    mochi.hud.remove("combo")
    mochi.hud.remove("combo_bar")
  end
end)
