#!/usr/bin/env bash
# Cross-build the DLL/launcher with MinGW and run the DLL in the Wine test host.
# usage: tools/cross.sh setup | build | shots [outdir]
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
out="${MOCHI_BUILD:-/tmp/mochi-build}"
tc="$out/mingw.cmake"
export PATH="$PATH:/usr/lib/wine"
export WINEPREFIX="${WINEPREFIX:-/tmp/wineprefix}" WINEDEBUG=-all WINEARCH=win64

setup() {
    apt-get install -y --no-install-recommends cmake g++-mingw-w64-x86-64-posix wine64 xvfb imagemagick
}

toolchain() {
    mkdir -p "$out"
    cat > "$tc" <<T
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
T
}

build() {
    toolchain
    for part in dll launcher; do
        cmake -S "$root/$part" -B "$out/$part" -DCMAKE_TOOLCHAIN_FILE="$tc" -DCMAKE_BUILD_TYPE=Release >/dev/null
        cmake --build "$out/$part" -j"$(nproc)"
    done
    x86_64-w64-mingw32-g++-posix -std=c++20 -O1 -municode -static -static-libgcc -static-libstdc++ \
        "$root/tools/testhost/main.cpp" -o "$out/testhost.exe" -ld3d11 -ldxgi -luser32
}

shots() {
    local dir="${1:-$out/shots}"
    mkdir -p "$dir"
    (Xvfb :77 -screen 0 1280x720x24 >/dev/null 2>&1 &)
    sleep 2
    export DISPLAY=:77
    cd "$out"
    (wine64 testhost.exe dll/Mochi.dll 22 > "$dir/log.txt" 2>&1 &)
    for i in $(seq 1 14); do
        sleep 2
        import -window root "$dir/f$i.png"
    done
    cat "$dir/log.txt"
}

"${1:?usage: cross.sh setup|build|shots}" "${@:2}"
