#pragma once
#include "box3d/box3d.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    // Conversions between o2 math types and Box3D C structs.
    // o2 Quat is {x,y,z,w}; Box3D b3Quat is {b3Vec3 v; float s} (s == w). Axes map 1:1.
    inline b3Vec3 ToBox3D(const Vec3F& v) { return b3Vec3{ v.x, v.y, v.z }; }
    inline Vec3F  FromBox3D(const b3Vec3& v) { return Vec3F(v.x, v.y, v.z); }
    inline b3Quat ToBox3D(const Quat& q) { return b3Quat{ b3Vec3{ q.x, q.y, q.z }, q.w }; }
    inline Quat   FromBox3D(const b3Quat& q) { return Quat(q.v.x, q.v.y, q.v.z, q.s); }
}
