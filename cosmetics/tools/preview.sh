#!/usr/bin/env bash
# Renders preview.png (front and back) for cosmetics through the Wine test host.
# usage: cosmetics/tools/preview.sh [id ...]      without ids it renders everything
set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
build="${MOCHI_BUILD:-/tmp/mochi-build}"
export PATH="$PATH:/usr/lib/wine" WINEPREFIX="${WINEPREFIX:-/tmp/wineprefix}" WINEDEBUG=-all WINEARCH=win64 DISPLAY=:77
pgrep Xvfb >/dev/null || (Xvfb :77 -screen 0 1280x720x24 >/dev/null 2>&1 &)
sleep 1

data="$(ls -d "$WINEPREFIX"/drive_c/users/*/AppData/Local/Mochi | head -1)"
SHOT_AT="${SHOT_AT:-4.5}"
ids=("$@")
[ ${#ids[@]} -gt 0 ] || mapfile -t ids < <(find "$here" -mindepth 2 -maxdepth 2 -name item.json -printf '%h\n' | xargs -n1 basename | sort)
winpath="Z:$(echo "$here" | tr / '\\')"

for id in "${ids[@]}"; do
    slot="$(python3 -c "import json,sys;print(json.load(open('$here/$id/item.json'))['slot'])")"
    mkdir -p "$data/configs"
    MOTION="${MOTION:-1}" python3 - "$data" "$slot" "$id" <<'PY'
import json, os, sys
data, slot, cid = sys.argv[1:4]
worn = json.dumps({slot: {"id": cid, "tint": []}})
settings = {"show": True, "size": 400, "x": 0.1, "y": 0.1, "views": 1, "rotate": False, "angle": 24, "worn": worn,
            "motion": int(os.environ.get("MOTION", "1")), "bg": True, "bgColor": [0.10, 0.07, 0.13, 1.0], "rounding": 14, "padding": 10, "padY": 10}
json.dump({"modules": {"Game Support": {"enabled": True, "settings": {"demo": True}}, "Cosmetics": {"enabled": True, "settings": settings}}},
          open(os.path.join(data, "configs", "default.json"), "w"))
PY
    (cd "$build" && MOCHI_COSMETICS="$winpath" TESTHOST_MANUAL=1 wine64 testhost.exe dll/Mochi.dll $(( ${SHOT_AT%.*} + 4 )) >/dev/null 2>&1 &)
    sleep "${SHOT_AT:-4.5}"
    import -window root "$build/shot_$id.png"
    sleep 2
    convert "$build/shot_$id.png" -crop 760x440+120+92 +repage -background '#1a1020' -gravity center -resize 512x512 -extent 512x512 "$here/$id/preview.png"
    echo "preview $id"
done
rm -rf "$data/configs"
