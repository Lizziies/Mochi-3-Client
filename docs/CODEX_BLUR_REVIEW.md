# HUD blur review, 2026-10-03

Independent source review while Claude integrates the Flarial core. No shared implementation files changed, no build run, no game injection or input. This report does not verify visible GPU output.

## Confirmed remaining issue

`dll/src/modules/HudModule.cpp`, `HudModule::onRender`, only submits blur when `rotation_.f == 0.f`. Any nonzero rotation silently disables an enabled Background blur setting. The text, border and background still rotate, so the settings disagree with the result.

Implement rotation in the blur job and pixel shader: keep the original local rounded rectangle, inverse-rotate each framebuffer pixel around the rectangle center before evaluating its rounded mask, and bound the scissor by the rotated corners. Sample the backdrop at the original framebuffer pixel. Rotating only the scissor or using an axis-aligned bounding rectangle would blur outside the actual HUD shape.

## Existing fix to preserve during integration

`modules::frame` submits post effects and the backdrop callback before module geometry. `Ui.cpp` draws the GUI afterwards. The backdrop callback copies the game image once for all HUD/menu blur rectangles; `blurPass` skips drawing if that copy is unavailable. Preserve this ordering and the reset-render-state callbacks when replacing rendering or adding Flarial events.

The blur job rectangle already includes the anchor shift calculated from the new content size. The draw vertices receive the same shift after channel merge. This addresses the previous mismatch between the callback rectangle and HUD geometry.

## Checks for the later authorized game session

- CPS blur on, menu closed: blur must cover the CPS field.
- Menu open: changing CPS blur must affect CPS independently of global menu blur.
- Background off, blur on: CPS still has a blurred footprint.
- Right/center anchor and changing digit widths: blur follows the field.
- Rotation at 1, 45 and 90 degrees: the blur mask now rotates with the field (inverse rotation in `psBlur`, scissor from the rotated corners); shader compiles with fxc, not yet seen in game.
- Resize/window mode change: no stale backdrop, black rectangle or displaced mask.

Do not mark the original screenshot issue resolved based on source review alone.

## Follow-up (Claude, 2026-10-03)

Rotation implemented as suggested: `post::blur` takes the angle, `psBlur` evaluates the rounded mask in the field's unrotated frame and samples the backdrop at the real pixel, the scissor covers the rotated corners. Release build and offline shader compile pass. Visible result still open until the next authorized game session.
