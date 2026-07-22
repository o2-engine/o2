# `Debug` documentation

## Description
A singleton for log output and drawing debug primitives: lines, arrows, circles, rectangles and text.

## Macro
- **o2Debug** — global access point to the `Debug` instance.

## Constructor and destructor
- **Debug(RefCounter\* refCounter)** — initializes the logging and font systems.
- **~Debug()** — releases resources.

## Logging methods
- **Log(format, ...)** / **LogStr(out)** — outputs informational messages.
- **LogWarning(format, ...)** / **LogWarningStr(out)** — outputs warnings.
- **LogError(format, ...)** / **LogErrorStr(out)** — outputs errors.
- **GetLog()** — returns the main log.

## Drawing methods
Draw debug primitives immediately or with a specified disappearing time (`delay`):
- **DrawLine(...)**
- **DrawArrow(...)**
- **DrawRay(...)**
- **DrawCircle(...)**
- **DrawRect(...)**
- **DrawLine(Vector<Vec2F> points, ...)**
- **DrawText(...)**

## Update and display methods
- **Update(isEditor, dt)** — updates the debug objects' timers.
- **Draw(isEditor)** — draws all active debug objects.

## Internal structures
- **IDbgDrawable** — base debug object interface (color, time).
- **DbgLine**, **DbgArrow**, **DbgCircle**, **DbgRect**, **DbgPolyLine**, **DbgText** — structures storing data and drawing the specific primitives.
