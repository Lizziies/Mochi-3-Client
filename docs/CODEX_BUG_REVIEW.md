# Additional source review, 2026-10-03

Independent review while Claude changes the Flarial integration. Only this report is added. No implementation changes, builds, game inputs or injection. Source findings are not reproduced Minecraft crashes or measured performance results. References describe the working tree at review time and may change during integration.

## 1. DLL unloaded even if background work is still running (P1)

`dll/src/core/Bg.cpp:38`, `dll/src/core/Client.cpp:64`, `dll/src/core/Client.cpp:79`.

`bg::drain(8000)` logs remaining workers after its timeout and returns anyway. `mainThread` subsequently calls `FreeLibraryAndExitThread`. A remaining shader compiler, inventory sweep or config worker can then execute unloaded code. The extra 30 ms sleep does not establish worker completion. Worker thread handles are closed immediately, so no actual join takes place.

Fix: stop accepting jobs during teardown, cancel supported jobs, retain completion/join information and refuse to free the DLL until every worker has exited. A timeout must leave the DLL loaded and report incomplete unload. Do not force-kill threads. Test with a deliberately delayed local worker, outside Minecraft.

## 2. Hold-mode Zoom stays active after a lost release (P1)

`dll/src/modules/camera/Camera.hpp:95`, `dll/src/modules/Manager.cpp:442`, `dll/src/hook/Input.cpp:248`.

Zoom's `active_` changes only through `press()` or module disable. `WM_KILLFOCUS` clears input keys but does not reset Zoom. The module manager also suppresses `onKey` for ordinary modules while the GUI captures the keyboard, including release events. Zoom's `onFrame` never reconciles its state with focus, cursor grab or the actual held key.

Reproduction path: hold C, open the client menu, release C, close the menu; or hold C and switch away from the game before releasing. Zoom can remain active with the key physically released. Freelook already has frame-level reconciliation; apply an equivalent explicit policy to Zoom, including mouse-bound keys. Test both Hold and Toggle so toggle behavior remains intentional.

## 3. Custom shader compilation blocks rendering (P1)

`dll/src/modules/post/PostFx.cpp:483`, `:490`, `:505`, `:605`.

The built-in shader compiles through `bg::run`, but the custom/preset shader path calls `D3DCompile` synchronously from `customShader`, reached by the render callback. The try-lock prevents waiting for another compiler; it does not make this compilation asynchronous. First use and use after reload can stall rendering. This is a concrete remaining stall path, not proof that it explains the user's previous random hangs.

Fix: compile bytecode on a worker, publish readiness safely, and use the last valid effect or skip the custom effect while pending. Keep GPU resource lifetime and unload coordination explicit. Measure compilation duration and frame timing in a later authorized test.

## 4. Patch restoration failures discard ownership (P1)

`dll/src/hook/FreeCamera.cpp:30`, `dll/src/hook/OwnNametag.cpp:19`.

Disable ignores the result of `codePatch::replace`, then clears tracked addresses. If restoring an otherwise still-owned NOP patch fails, e.g. VirtualProtect failure, it is left in game memory without a retryable ownership record. FreeCamera additionally resumes the native update while an angle-store patch may remain disabled. Foreign bytes must still never be overwritten.

Fix: distinguish restored, foreign-modified and restore-failed states. Retain ownership for a still-owned patch on failure, report it, retry safely, and prevent final unload while owned changes remain. Extend existing local lifecycle tests with injected restore failure; a foreign modification should continue to be preserved.

## 5. Native nametag failure is silently ignored (P2)

`dll/src/modules/world/Entities.hpp:29`.

`ownNametag::show(...)` reports whether the native patch actually took effect, but `ThirdPersonNametag::onFrame` ignores its return value. A resolved signature with unexpected instruction bytes or a failed patch leaves the module enabled with no native tag and no failure feedback. This conflicts with the project requirement that unavailable game effects must not appear silently functional.

Fix: distinguish a normal request to hide the tag from failure of a request to show it. Propagate installation/patch failure to module availability with a useful reason. A visible effect still needs an authorized game check.

## 6. Shader blobs can survive shutdown unfinished (P2)

`dll/src/modules/post/PostFx.cpp`, `shutdown`; `dll/src/modules/Manager.cpp:321`; `dll/src/core/Client.cpp:64`.

`post::shutdown` runs before draining background jobs. If compileState is Running, it returns after dropAll without releasing blobs. The worker can subsequently finish and publish Ready, but teardown never calls post::shutdown again. Finished or partially compiled COM blobs then remain unreleased. Simply moving cleanup after drain must also respect the worker lifetime fix in finding 1.

Fix: split stopping GPU work from releasing compiler results; join the compiler before final blob cleanup. Test unload during pending compilation in a local host.

## Separate blur finding

See `docs/CODEX_BLUR_REVIEW.md`: nonzero HUD rotation silently suppresses enabled blur. Preserve the existing early backdrop capture and anchor-shift fix during rendering integration.

## Suggested order

Resolve safe unload and patch restoration first, then Zoom input lifecycle, asynchronous custom compilation, and nametag availability feedback. Recheck these findings against Claude's latest edits before applying fixes. No claim that the client is ready for gameplay follows from this review.

## Follow-up (Claude, 2026-10-03)

All six handled in code; builds and local native tests pass, nothing checked in game yet.

1. `bg` keeps every worker handle, refuses new jobs once teardown starts and `drain` waits on the handles. If one is still running after the timeout, `teardown` reports it and the main thread exits without `FreeLibrary`, so the dll stays loaded instead of freeing code under a live thread.
2. Zoom in hold mode now follows the real key every frame (grabbed cursor, no menu, in a world), like Freelook. Toggle keeps its state on purpose. The mouse side buttons X1/X2 were never written into the key state, so a zoom bound to them could not be checked at all; fixed in `Input.cpp`.
3. Custom shaders compile on a worker; the frame skips the custom pass until the bytecode is in, a reload drops results of the old list by generation.
4. `codePatch::replace` returns Applied, Foreign or Failed. A failed restore stays owned and is retried on the next call; foreign bytes are left alone and released. Freelook keeps the native update paused while an angle store is still disabled. New native test: restore against an unreadable page fails, stays in place and succeeds on the retry. Unload is not blocked by a remaining patch: the patched bytes are NOPs or a jump inside the game and point at nothing of ours.
5. Third Person Nametag no longer needs `OwnNametagGate`. If the game's own tag cannot be switched on, it says so once in the log and the settings, and draws the name as overlay instead of doing nothing.
6. `post::releaseCompiled` runs after the workers are joined and releases built-in and custom compiler results.
