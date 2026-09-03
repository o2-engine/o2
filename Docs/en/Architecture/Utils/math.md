## Math
The engine provides basic types for 2D and 3D math:
- `Vec2F/I` - two-dimensional vector
- `Vec3F/I` - three-dimensional vector
- `RectF/I` - rectangle with automatic edge sorting - left is always less than right, top is always above bottom
- `BorderF/I` - border around a rectangle
- `Basis` - transformation basis, effectively a 3x2 matrix
- `Basis3D` - three-dimensional transformation basis
- `Mat4` - 4x4 matrix
- `Quat` - quaternion
- `Transform` - transformation
- `Spline` - two-dimensional spline
- `Curve` - curve as a function

There are also math functions in the `o2::Math` namespace: Min, Max, Floor, Ceil, Round, Clamp, Sin, Cos, etc.

## Coordinate system
By default the engine uses a Cartesian coordinate system: zero at the screen center, the X axis pointing right, the Y axis pointing up.

## Basis
It is a 3x2 matrix, but in a somewhat unusual yet more convenient form. It is defined by a pair of axes `xv` and `yv`, specifying the directions of the X and Y axes. They do not have to be unit-length or perpendicular, just like in a matrix. These axes are measured from the `origin` point.

This way the transformation basis is defined intuitively while working like a matrix. There are methods for composing it from position/angle/scale and for the inverse decomposition.

## Transform
A more detailed transformation description. The transformation is three-dimensional (`Vec3F`), but each parameter has a 2D property for working in 2D (`position2D`, `size2D`, `scale2D`, `pivot2D`, `shear2D`). It contains:
- position - position relative to pivot
- pivot - offset point. The position is measured from it, so the pivot is always at zero
- size - size
- scale - scale. Defaults to (1; 1; 1), multiplies size relative to pivot
- angle - 2D rotation angle; the full rotation is set by the `rotation` quaternion or the `eulerAngles` Euler angles
- shear - shear/skew, turning the rectangle into a parallelogram

And many useful helper methods.

## Spline
A Bezier curve in two-dimensional space. Anchor points are defined, with optional support points for the previous and next segments. Regular 4-point Bezier interpolation is used.

## Curve
Also a curve, but representing a function interpolated along the X axis, returning a single function result value along Y. It also uses 4-point Bezier curves internally.

## Random numbers

`Math::Random(min, max)` draws from the current source, the global `rand()` by default.
`Math::RandomScope scope(seed)` routes `Math::Random` through its own deterministic generator
(minstd) while alive — nested scopes restore the previous one, the global `rand()` is neither reseeded
nor consumed. The particles emitter bakes frames reproducibly this way (`seed + frame index`) without
breaking randomness elsewhere (`Math::RandomUnit()` — a 0..1 value from the current source)
