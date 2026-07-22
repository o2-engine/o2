## Input system, o2::EventSystem and o2::Input
`o2::EventSystem` handles key and mouse (or touch) presses. It routes messages to the needed entities, resolving clipping and draw order issues internally.

`o2::Input` provides information about the current state of the input systems:
- key presses: just pressed, held down, just released
- cursor (touch) presses: press, release
- cursor position, cursor delta per frame

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
