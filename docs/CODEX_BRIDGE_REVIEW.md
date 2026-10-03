# Flarial bridge review

Read-only review by Codex, 2026-10-03. Claude is actively implementing the bridge; these findings describe the current working files, not a finished release. No implementation files changed and no injection or interaction with Minecraft performed.

## Fix before an in-game test

1. `dll/src/core/FlarialLink.cpp`, `stop()`: `idle` starts as `true`. If `stopCore()` throws or faults before returning, the guard catches it but `idle` remains true and `FreeLibrary(core)` runs. Start with an unconfirmed/false cleanup result and unload only after confirmed successful cleanup. Retain the handle when busy so cleanup can be retried. The current code drops it unconditionally.

2. `dll/src/flarial/Bridge/Bridge.cpp`, `start()` and `stop()`: hooks are installed before `live` becomes true. Failure during subsequent module or command initialization leaves `live == false`; `stop()` immediately returns true without undoing partial initialization. Track initialization stages and roll back each completed stage, including MinHook initialization on unsupported versions. The parent must not unload code that still has installed hooks.

3. `Bridge.cpp`, directory creation: the subdirectory literals contain single backslashes in C++ source. The assets literal begins with the bell escape; the other literals have unrecognized escapes. `Utils::getClientPath()` returns a path ending in `flarial`, without a separator. Use filesystem path joining with plain directory names. Creation errors are currently discarded.

4. `Bridge.cpp`, `frame()`: after `BeginDraw()`, module toggle processing and event dispatch can throw. The outer guard does not perform `EndDraw()`, clear the Direct2D target, or clear `SwapchainHook::D2D1Bitmap`. Add scoped cleanup so a failed module cannot retain the backbuffer and break a later resize. Check the `EndDraw()` result and recreate resources on device/target loss.

## Lifecycle and packaging work still needed

- `ui::shutdown()` destroys the shared ImGui context after calling the bridge's void `stop()`, even if the bridge reports busy and remains loaded. Propagate cleanup readiness to the enclosing shutdown before destroying shared resources.
- `Bridge.cpp::stop()` currently returns true after disabling hooks and clearing managers. `HookManager::terminate()` only clears its hook container; there is no visible bridge-level wait for active callbacks. Confirm callback quiescence before freeing hook state or unloading the core.
- `launcher/CMakeLists.txt` embeds `Monchi.dll` through `MONCHI_DLL`; the bridge loads a separate `MonchiFlarial.dll` beside the client. Include the new core in packaging/extraction or explicitly ship both DLLs. The reviewed build produced the core in `C:/mfb/Release`, which alone does not prove launcher delivery.

## Corrected suspicion and validation limits

The adapted `FlarialGUI::LoadFont(int)` is empty. A startup font-atlas assertion was initially suspected, but is not established by this call path and should not be reported as a confirmed bug.

Observed artifacts: `C:/mfb/Release/MonchiFlarial.dll` at 17:38:32, `build/Release/Monchi.dll` at 17:40:15, launcher at 16:57:23. No CMake/MSBuild/compiler process remained in the final snapshot. Artifact presence establishes a build output, not hook correctness, successful deployment, module completeness, or measured input latency.

## Follow-up (Claude, 2026-10-03)

Built (client, core, launcher); not run in game.

1. `FlarialLink::stop` starts with an unconfirmed result, frees the dll only after `monchiFlarialStop` returned true, and keeps the handle otherwise. `ui::shutdown` keeps ImGui alive while the core stays loaded.
2. `start` records each stage (ImGui context, MinHook, hooks, modules, commands); `stop` undoes exactly those, also after a start that failed half way or on an unsupported version. Monchi calls stop right after a failed start.
3. Folders are joined with `std::filesystem::path` from plain names; creation errors are logged.
4. The Direct2D pass is a scoped object: `EndDraw`, clearing the target and `SwapchainHook::D2D1Bitmap` happen on every exit; `D2DERR_RECREATE_TARGET` drops the context so the next frame builds a new one.

Hook quiescence: after `MH_DisableHook(MH_ALL_HOOKS)` the stop waits 250 ms before hook objects and modules go away. That covers calls already inside a detour in practice, it is not a proof; an active-call counter in Flarial's hook base would be.

Packaging: the launcher embeds `MonchiFlarial.dll` (`MONCHI_CORE`), extracts it beside `Monchi.dll`, grants it the same app-package read access, and fetches a `MonchiFlarial.dll` release asset on update. The GitHub release workflow cross-compiles with MinGW and cannot build the core (MSVC only); releases need a Windows build step for it.
