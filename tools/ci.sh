#!/usr/bin/env bash
# Build, self-test and menu screenshots under Wine, used by .github/workflows/check.yml.
# usage: tools/ci.sh [outdir]
set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
out="${1:-$root/ci-out}"
build="${MOCHI_BUILD:-/tmp/mochi-build}"
export MOCHI_BUILD="$build"
export PATH="$PATH:/usr/lib/wine"
export WINEPREFIX="${WINEPREFIX:-$HOME/.wine-mochi}" WINEDEBUG=-all WINEARCH=win64
mkdir -p "$out" "$WINEPREFIX"

if ! "$root/tools/cross.sh" build > "$out/build.log" 2>&1; then
    grep -E "error|Error" "$out/build.log" | head -40 > "$out/errors.txt"
    echo "build failed"
    exit 1
fi

(Xvfb :77 -screen 0 1280x720x24 >/dev/null 2>&1 &)
sleep 2
export DISPLAY=:77
timeout 180 wine64 wineboot -i >/dev/null 2>&1 || true
sleep 3

data() { ls -d "$WINEPREFIX"/drive_c/users/*/AppData/Local/Mochi 2>/dev/null | head -1; }

cd "$build"
if [ -n "${MOCHI_CI_SELFTEST:-}" ]; then
    MOCHI_SELFTEST=1 timeout 420 wine64 testhost.exe dll/Mochi.dll 360 > "$out/selftest-host.txt" 2>&1 || true
    cp "$(data)/logs/latest.log" "$out/selftest.log" 2>/dev/null || true
    grep -E "selftest|\[error\]" "$out/selftest.log" 2>/dev/null | tail -60 > "$out/selftest-summary.txt" || true
fi

# each step: seconds:kind:a,b  (k = key, c = click, w = wheel, t = char)
script="${MOCHI_CI_SCRIPT:-}"
shots="${MOCHI_CI_SHOTS:-4 7 10 13 16 19 22 25}"
rm -rf "$(data)/configs" "$(data)/logs/latest.log"
(TESTHOST_MANUAL=1 TESTHOST_SCRIPT="$script" timeout 120 wine64 testhost.exe dll/Mochi.dll 40 > "$out/menu-host.txt" 2>&1 &)
# the host's script clock starts when the process does, which is about when the client logs "ready"
for i in $(seq 1 600); do
    grep -q "\] \[info\] ready" "$(data)/logs/latest.log" 2>/dev/null && break
    sleep 0.1
done
start=$(date +%s)
for t in $shots; do
    while [ $(( $(date +%s) - start )) -lt "$t" ]; do sleep 0.2; done
    import -window root "$out/menu-$t.png"
done
sleep 6
cp "$(data)/logs/latest.log" "$out/menu.log" 2>/dev/null || true
echo "done"
