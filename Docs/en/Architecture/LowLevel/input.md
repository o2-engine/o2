## Input system, o2::EventSystem and o2::Input
`o2::EventSystem` handles key and mouse (or touch) presses. It routes messages to the needed entities, resolving clipping and draw order issues internally.

`o2::Input` provides information about the current state of the input systems:
- key presses: just pressed, held down, just released
- cursor (touch) presses: press, release
- cursor position, cursor delta per frame

### Key codes
Keys are `o2::KeyboardKey` values from `o2/Application/VKCodes.h`, and they are **platform dependent**: Windows and Linux use the Windows virtual key codes, while Mac, iOS, WebAssembly and Android share the macOS table, where letters and digits are their ASCII codes and everything else is a negated macOS hardware key code (`VK_LEFT` is `-123` there). Always compare against the `VK_*` macros, never against a literal.

Every platform backend converts its native events into these values: Mac through `MacHardwareKeyToKeyboardKey`, the browser through `DomKeyCodeToKeyboardKey`, which maps the layout independent DOM `KeyboardEvent.code` ("KeyW", "ArrowLeft") and leaves keys it doesn't know to the page.

### Key handling, o2::KeyboardEventsListener
This class is used as a key press handler interface. You can inherit from it and override the needed functions. Key press/release messages arrive to all interface descendants automatically

### Cursor and touch handling, o2::CursorAreaEventsListener
This interface handles not all presses and events, but only when the cursor hits it, taking into account overlap by other touch handlers.

After inheriting from the interface, you need to override the `IsUnderPoint` function, which checks whether a point hits the area, without accounting for clipping. For example, a hit on some geometric shape or on graphics. You also need to call the `OnDrawn` function at the moment the entity appears graphically on screen

The input system itself deals with clipping and listeners overlapping each other.

You also need to override the needed cursor and touch message functions:
-`OnCursorPressed` - cursor pressed on the entity
-`OnCursorReleased` - cursor released, not necessarily over it
-`OnCursorPressBreak` - pressed cursor interrupted (system-level, leaving the screen area, etc.)
-`OnCursorPressedOutside` - cursor pressed outside
-`OnCursorReleasedOutside` - cursor released outside
-`OnCursorStillDown` - cursor is still pressed, not necessarily inside
-`OnCursorMoved` - cursor moved inside the area
-`OnCursorEnter` - cursor entered the area
-`OnCursorExit` - cursor left the area
-`OnCursorDblClicked` - double click on the area

For the right and middle mouse buttons there are analogous messages: `OnCursorRightMousePressed`/`OnCursorRightMouseReleased`, `OnCursorMiddleMousePressed`/`OnCursorMiddleMouseReleased`, etc.

### Mouse wheel

`o2Input.GetMouseWheelDelta()` returns the per-frame wheel delta in engine units: one classic wheel notch is ~120 (as `WHEEL_DELTA` on Windows/Linux). macOS reports deltas differently: touchpad and Magic Mouse give precise ones (`hasPreciseScrollingDeltas`, in pixels, frequent and large), a classic wheel gives lines (±1..3). The platform layer brings them to one scale with `Input::NormalizeWheelDelta(delta, preciseDeltas)`: precise deltas pass through, line deltas are multiplied by `Input::kWheelLineDelta` (60). The distinction is made by the system flag, not by guessing from delta magnitude.
