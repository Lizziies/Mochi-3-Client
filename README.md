# Monchi

A free PvP client for Minecraft Bedrock Edition on Windows (GDK builds). It comes as a small launcher plus a DLL that gets injected into the game. Working title, the name will change before the first real release.

It's an alpha. The menu, the HUD, the post effects, the latency tools and the settings work on any version. Modules that read or change the game itself (zoom, reach, combo, hitbox, ...) need signatures for your Minecraft version, and until we have them for a version those modules stay grey with "not available on this version". Nothing crashes because of it.

## What it does

- Around 140 modules: the usual PvP HUD (CPS, keystrokes, armor, potions, combo, reach), camera and world tweaks, crosshair editor, post effects, chat tools, Hive and Zeqa helpers.
- A menu with cards, per-module settings, hold mode and keybinds, profiles and a HUD editor where every element can be dragged, snapped to others, nudged with the arrow keys and scaled with the mouse wheel.
- Latency tools: click-to-frame measurement, lag analyzer that splits the delay into input, display, network and server, a network monitor with WLAN diagnosis, a frame limiter, short frame queue and a Performance Lock switch for steady frame times.
- Server rules: a module that a server doesn't allow can't be turned on there. The list lives in `servers/servers.json` and updates without a new release.
- Legit only. No reach, killaura, aim assist, autoclicker, velocity or anything that sends packets the vanilla client wouldn't.
- English and German. It follows the Windows language unless you pick one.

## Using it

1. Download `MonchiLauncher.exe` from the releases page.
2. Press Play. The launcher updates the client, starts Minecraft and injects it.
3. Press Right Shift in game to open the menu. Ctrl+L unloads the client.

Older Minecraft versions: the Versions page can download and start [LeviLauncher](https://github.com/LiteLDev/LeviLauncher), which installs and switches game versions with your own Minecraft license. Monchi attaches to whatever version is running. We don't ship or host any Minecraft files.

## Building

On Windows:

```
cmake -S dll -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cmake -S launcher -B build-launcher -G "Visual Studio 17 2022" -A x64
cmake --build build-launcher --config Release
```

On Linux (MinGW, tested under Wine):

```
tools/cross.sh setup
tools/cross.sh build
tools/cross.sh shots
```

`MONCHI_SELFTEST=1` makes the client switch every module on with demo data, poke its settings and report anything that breaks or has no German text.

## Docs

Start with `docs/MASTER.md`. The module list is in `docs/MODULES.md`, how signatures and updates work is in `docs/VERSIONING.md` and `docs/UPDATES.md`, and `docs/PARITY.md` is an honest look at where we stand next to Flarial and Onix.

## License

AGPL-3.0, see `LICENSE`. Parts are adapted from [Flarial](https://github.com/flarialmc/dll-oss), also AGPL-3.0; the upstream copy and its commit are in `vendor/flarial`. If you share a build, share its source too.
