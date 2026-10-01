#!/usr/bin/env python3
"""Writes a few small cosmetics in the MochiCosmetic format, used to test the client loader and preview.

usage: python3 tools/testdata/make_cosmetics.py
"""
import json
import math
import pathlib
import struct
import zlib

out = pathlib.Path(__file__).resolve().parent / "cosmetics"


def png(path, size, pixel):
    rows = []
    for y in range(size):
        row = bytearray([0])
        for x in range(size):
            row += bytes(pixel(x, y))
        rows.append(bytes(row))
    raw = zlib.compress(b"".join(rows), 9)

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)) + chunk(b"IDAT", raw) + chunk(b"IEND", b""))


def write(item, size, pixel):
    folder = out / item["id"]
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "item.json").write_text(json.dumps(item, indent=2) + "\n")
    png(folder / "tex.png", size, pixel)


def soft(x, y, size):
    shade = 0.78 + 0.22 * (1 - y / size)
    v = int(255 * shade)
    return (v, v, v, 255)


write(
    {
        "id": "halo",
        "name": "Halo",
        "slot": "head",
        "tags": ["head", "glow", "animated"],
        "texture": "tex.png",
        "tint": [{"name": "Glow", "default": "#ffe27a"}],
        "bones": [
            {
                "name": "ring",
                "pivot": [0, 34, 0],
                "rotation": [0, 0, 0],
                "anim": {"type": "float", "axis": "y", "amplitude": 0.8, "speed": 1.0, "phase": 0},
                "cubes": [
                    {"origin": [-4, 34, -5], "size": [8, 1, 1], "uv": [0, 0], "tint": "Glow"},
                    {"origin": [-4, 34, 4], "size": [8, 1, 1], "uv": [0, 0], "tint": "Glow"},
                    {"origin": [-5, 34, -4], "size": [1, 1, 8], "uv": [0, 0], "tint": "Glow"},
                    {"origin": [4, 34, -4], "size": [1, 1, 8], "uv": [0, 0], "tint": "Glow"},
                ],
            }
        ],
    },
    16,
    lambda x, y: soft(x, y, 16),
)

write(
    {
        "id": "cat_ears",
        "name": "Cat Ears",
        "slot": "head",
        "tags": ["head", "animal", "animated"],
        "texture": "tex.png",
        "tint": [{"name": "Fur", "default": "#ff9fc8"}, {"name": "Inner", "default": "#ffd6e8"}],
        "bones": [
            {
                "name": "ear_l",
                "pivot": [2.5, 32, 0],
                "anim": {"type": "twitch", "axis": "z", "amplitude": -14, "speed": 1.0, "phase": 0},
                "cubes": [
                    {"origin": [1, 32, -1], "size": [3, 3, 1], "uv": [0, 0], "tint": "Fur"},
                    {"origin": [1.5, 35, -1], "size": [2, 1, 1], "uv": [0, 0], "tint": "Inner"},
                ],
            },
            {
                "name": "ear_r",
                "pivot": [-2.5, 32, 0],
                "anim": {"type": "twitch", "axis": "z", "amplitude": 14, "speed": 1.0, "phase": 2.4},
                "cubes": [
                    {"origin": [-4, 32, -1], "size": [3, 3, 1], "uv": [0, 0], "tint": "Fur"},
                    {"origin": [-3.5, 35, -1], "size": [2, 1, 1], "uv": [0, 0], "tint": "Inner"},
                ],
            },
        ],
    },
    16,
    lambda x, y: soft(x, y, 16),
)


def petals(x, y):
    d = math.hypot(x - 16, y - 20)
    fade = max(0.0, 1.0 - d / 26.0)
    v = int(215 + 40 * fade)
    if (x * 7 + y * 13) % 19 == 0:
        return (255, 255, 255, 255)
    return (v, v, v, 255)


def wing(side):
    x0 = 2 if side > 0 else -12
    return {
        "name": "wing_l" if side > 0 else "wing_r",
        "pivot": [2.0 * side, 22, -2.5],
        "rotation": [0, -18 * side, 6 * side],
        "anim": {"type": "flap", "axis": "y", "amplitude": 22 * side, "speed": 0.8, "phase": 0},
        "cubes": [
            {"origin": [x0, 15, -3], "size": [10, 12, 1], "uv": [0, 0], "tint": "Main"},
            {"origin": [x0 + (10 if side > 0 else -6), 17, -3], "size": [6, 8, 1], "uv": [0, 0], "tint": "Main"},
            {"origin": [x0 + (2 if side > 0 else 4), 11, -3], "size": [4, 4, 1], "uv": [0, 0], "tint": "Accent"},
        ],
    }


write(
    {
        "id": "sakura_wings",
        "name": "Sakura Wings",
        "slot": "wings",
        "tags": ["wings", "pink", "animated"],
        "texture": "tex.png",
        "tint": [{"name": "Main", "default": "#ff7eb6"}, {"name": "Accent", "default": "#ffffff"}],
        "bones": [wing(1), wing(-1)],
    },
    32,
    petals,
)


def heart(x, y):
    u, v = (x - 8) / 4.5, (8 - y) / 4.5
    if (u * u + v * v - 1) ** 3 - u * u * v ** 3 <= 0:
        return (255, 230, 240, 255)
    return (255, 255, 255, 255)


write(
    {
        "id": "mochi_cape",
        "name": "Mochi Cape",
        "slot": "cape",
        "tags": ["cape", "pink"],
        "texture": "tex.png",
        "tint": [{"name": "Main", "default": "#ff7eb6"}, {"name": "Trim", "default": "#ffffff"}],
        "bones": [
            {
                "name": "cape",
                "pivot": [0, 24, -2],
                "rotation": [4, 0, 0],
                "physics": "cloth",
                "cubes": [
                    {"origin": [-4, 24 - 2 * (i + 1), -3.2], "size": [8, 2, 1], "uv": [0, 0], "tint": "Main" if i < 5 else "Trim"}
                    for i in range(6)
                ],
            }
        ],
    },
    16,
    heart,
)

print("wrote", sorted(p.name for p in out.iterdir()))
