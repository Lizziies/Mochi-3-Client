-- Turns a warning on screen when your ping goes over 120 ms.
local limit = 120

mochi.on("tick", function(dt)
  local w = mochi.world()
  if w.ping > limit then
    mochi.hud.text("ping", "High ping: " .. w.ping .. " ms", 0.5, 0.12, { color = 0xFF6B73, scale = 1.3, align = "center", background = true })
  else
    mochi.hud.remove("ping")
  end
end)
