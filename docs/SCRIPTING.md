# Lua scripting

Monchi can run your own Lua 5.4 scripts. Scripts are plain text files in `%LOCALAPPDATA%\Monchi\scripts`. Turn on the module "Lua Scripts" (category Client), put a `.lua` file in the folder and it starts. Save the file and it reloads by itself.

The Lua sources are not part of this repository. CMake downloads Lua 5.4.8 from `github.com/lua/lua` at configure time (`FetchContent`, see `dll/CMakeLists.txt`), so the first configure needs network access and `git`.

## What a script can do

Everything goes through the global table `monchi`. A script cannot touch files, run programs or open the network. The libraries `io`, `package` and `debug` are not there, `os` only has `time`, `date` and `clock`, `load`, `dofile` and `require` are removed.

Limits per script: 24 MB of memory, about 3 million Lua instructions per call (a call that runs longer is stopped and the script is switched off with an error in the settings).

### Events

```lua
monchi.on("hit", function(e) end)      -- e.reach, e.crit, e.crystal, e.damage, e.target
monchi.on("hurt", function(e) end)     -- e.damage
monchi.on("kill", function(e) end)     -- e.name
monchi.on("death", function() end)
monchi.on("respawn", function() end)
monchi.on("chat", function(text) end)  -- text without color codes
monchi.on("sound", function(e) end)    -- e.id, e.distance
monchi.on("key", function(e) end)      -- e.vk, e.down
monchi.on("server", function(e) end)   -- e.name, e.host, e.joined
monchi.on("tick", function(dt) end)    -- once per frame
monchi.every(2.5, function() end)      -- every 2.5 seconds
```

Events only arrive when the game data behind them is available. With the demo data (module "Game Support") all of them are simulated.

### Reading the game

| Call | Returns |
|---|---|
| `monchi.player()` | table: `name x y z vx vy vz yaw pitch health maxHealth hunger level dimension slot onGround sprinting sneaking swimming held` |
| `monchi.target()` | `nil` or table: `kind name distance isPlayer health breakProgress` |
| `monchi.world()` | `time day raining biome ping players entities` |
| `monchi.combat()` | `combo bestCombo hits swings kills deaths streak lastReach` |
| `monchi.server()` | name of the server (`""` when unknown) |
| `monchi.screen()` | `"game"` or `"menu"` |
| `monchi.fps()`, `monchi.time()` | frames per second, seconds since the client started |

### Showing things

Positions are fractions of the screen (0 to 1). Elements stay on the screen until you remove them.

```lua
monchi.hud.text(id, text, x, y, options)
monchi.hud.bar(id, x, y, width, height, fraction, options)   -- width and height in pixels
monchi.hud.remove(id)
monchi.hud.clear()
```

Options: `color` (number like `0xFF7DB5` or a table `{r=1, g=0.5, b=0.7, a=1}`), `scale`, `shadow`, `align` (`"left"`, `"center"`, `"right"`), `background` (true or false) and `fill` (color of the background or the empty part of a bar).

### Other

```lua
monchi.log("anything", 42)                 -- goes to the log file
monchi.notify("Title", "Text", "ok")       -- kinds: info, ok, warn, error
monchi.save("key", value)                  -- text, number or boolean, stored next to the script
monchi.load("key", default)
monchi.say("gg")                           -- only when "Let scripts type in chat" is on
```

`monchi.say` types the text into the chat the same way a player would, at most one message every 1.2 seconds, and only while the game window is focused. It is off by default.

## Example

```lua
monchi.on("tick", function(dt)
  local p = monchi.player()
  monchi.hud.text("hp", string.format("%.1f hearts", p.health / 2), 0.01, 0.30, { background = true })
end)
```

More examples are in the `scripts` folder of this repository. The module has a button "Script list from GitHub" that shows `scripts/index.json` from the repository and installs a script with one click. Scripts from the list also run in the sandbox, but only install what you trust.

## Rules on servers

A script can only show or compute things the normal client could show too. It cannot send packets, move the player or click for you. Check the rules of your server before you use scripts that react to other players.
