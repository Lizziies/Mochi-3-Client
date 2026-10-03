# GPU latency work, 2026-10-03

Separate branch `codex/gpu-input-latency`, starting at `7a6f1c4`. Claude's active checkout and the running Minecraft/Flarial session were not modified. This is an integration candidate, not a measured fastest-client claim.

## Implemented

- Select the backend from the actual D3D device's DXGI adapter. DX12 uses its adapter LUID; DX11 uses IDXGIDevice. A laptop's other GPU does not determine the selection.
- NVIDIA: direct public NVAPI initialization, device-specific support query and SetSleepMode, with optional explicit Boost. Load only the installed System32 driver library. No RTSS, global driver-profile edits, Streamline interposer, fabricated markers or Minecraft files.
- Documented NVIDIA driver-only low-latency mode works without calling Sleep. The UI labels it as driver-only and does not present it as a completed before-input Reflex integration.
- AMD: the official MIT Anti-Lag 2 DX12 SDK is integrated into the backend. Enabling is gated on a verified before-input frame hook. No private context fields are modified. No AMD SDK support claim is inferred merely from an AMD vendor ID.
- A frame-boundary contract accepts monotonically increasing nonzero render-frame IDs, paces at most once for each, disables after backend failure and retains configuration ownership if reset fails. Losing the verified hook disables pacing.
- Low Latency settings default the new GPU mode to Off. Unsupported modes are hidden; AMD reports the missing verified hook. Driver errors are visible.
- Recent frame interval statistics show mean/P95/P99 over 256 samples. Sorting occurs when settings are drawn, not every frame. Click timing is explicitly event-to-Present-return, not input-to-photon or proof of causality.
- DXGI_PRESENT_TEST calls skip overlay drawing, frame statistics and the limiter. Other swapchains, failed presents and occlusion statuses do not count as successful game presents.
- Teardown resets owned latency state. A failed reset prevents final DLL unload.

## Verified locally

MSVC Release DLL builds with MONCHI_FLARIAL=OFF. Standalone CTest checks cover duplicate/older frame IDs, missing verified hooks, AMD enable gating, driver failure shutdown, retryable restore and bounded percentile statistics.

`tools/latency/gpu_probe.cpp` creates separate DX12 devices. It identified NVIDIA GeForce RTX 5070 (0x10de) and AMD Radeon(TM) Graphics (0x1002) against each device's adapter. NVIDIA GetSleepStatus returned success; NVIDIA mode On then Off returned success on the separate test device. AMD Anti-Lag 2 Initialize also returned S_OK on the separate AMD device, and the probe verified that a request to enable it without a verified input boundary is rejected. AMD pacing was not enabled. No Minecraft process was accessed.

An early probe version crashed on device release after NvAPI_Unload. Retaining the DLL mapping alone did not fix it. Keeping the NVAPI initialization alive for the process lifetime fixed the local probe. The injected client does not own or destroy Minecraft's device, so it resets its settings but does not unload that driver session. The driver mapping/initialization reference remains until process exit; repeated DLL reloads can retain another initialization reference. This is a documented lifetime tradeoff to review, not a claimed NVAPI-wide bug.

## Still required for complete game integration

Nothing currently calls `bindFrameStart(true)` or `beforeInput(frameId)` from Minecraft. Do not enable either by guessing a Present counter is a simulation frame.

The incoming Flarial core has `InputHandler::tick` and `ClientInstance::update` candidates. Source signatures and a four-argument callback do not prove their position before input sampling, cadence once per rendered frame, or thread/frame identity on 1.26.52.3. Actor tick at 20 Hz is not a valid substitute.

In an explicitly authorized future Monchi test session:

1. Trace frame start, first GameInput reading, simulation completion, render submission and Present across threads. Establish a reliable frame ID and full ABI. Install the native hook under the crash guard.
2. Bind the controller only after the hook is installed and this ordering is verified. Call beforeInput once before the actual reading, never once per keyboard/mouse packet. Stop binding before hook removal.
3. Validate NVIDIA driver reports and AMD's built-in latency monitor. Full Reflex markers require real simulation/submit/present boundaries sharing frame IDs; they are currently deliberately absent.
4. Ensure only one pacing strategy controls the frame. Do not stack the after-Present limiter, waitable-object pacing and the driver limiter without controlled measurements.
5. Run unchanged settings/workload, GPU-bound and CPU-bound cases, randomized Off/On comparisons, mouse polling 125/1000/8000 Hz, menu/focus/resize/server switches. Record median/P95/P99 frame intervals and input-to-photon using an external sensor or high-speed camera. Event-to-Present timestamps alone cannot prove the fastest input.

No numerical improvement or freedom from stutter has been measured in Minecraft. No injection is authorized while the user plays with Flarial.

## Flarial comparison and sources

The copied public `RawInputBuffer` module only contains enable/disable delegation and placeholder settings. Its `MouseHook` dispatches buffered mouse packets through its event system; it is not Reflex or Anti-Lag 2 pacing. Replaying raw packets on top of Bedrock's GameInput readings risks duplicate input and is not added.

Monchi's concrete additions over that public baseline are real vendor API calls, actual-device adapter selection, explicit partial-support reporting, failure handling and frame percentiles. Whether they yield better game latency remains to be measured.

- [NVIDIA public NVAPI SDK and API documentation](https://github.com/NVIDIA/nvapi): SetSleepMode supports operation without Sleep, with less optimal delay placement. Sleep belongs before input, exactly once each frame.
- [NVIDIA Reflex integration guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideReflex.md): full engine integration needs correct device, marker placement and frame identity. Direct NVAPI is used here rather than late Streamline initialization.
- [AMD Anti-Lag 2 integration guide](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK): DX12 Initialize must succeed on the game's device; Update belongs immediately before input sampling. Official validation tools are described there.
- [Flarial source baseline](https://github.com/flarialmc/dll-oss): see vendor/flarial/UPSTREAM.json.

The SDK copies preserve MIT licenses and pinned source commits in vendor/nvapi and vendor/antilag2. No closed-source Onix code is used.
