-- Counts your kills per server and remembers the best streak between sessions.
local best = mochi.load("best", 0)
local streak = 0

mochi.on("kill", function(e)
  streak = streak + 1
  if streak > best then
    best = streak
    mochi.save("best", best)
    mochi.notify("New record", "Kill streak " .. best, "ok")
  end
end)

mochi.on("death", function()
  streak = 0
end)

mochi.on("tick", function(dt)
  mochi.hud.text("kills", "Streak " .. streak .. "  ·  best " .. best, 0.01, 0.40, { background = true })
end)
