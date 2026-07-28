#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Singleton.h"

// Gizmos drawer access macros
#define o2Gizmos o2::Gizmos::Instance()

namespace o2
{
    // ---------------------------------------------------------------------------------------------
    // Editor gizmos drawer. Actors and components draw their gizmos here from OnDrawGizmos, in world
    // coordinates; the scene view sets projection, which maps world point to the current draw space
    // ---------------------------------------------------------------------------------------------
    class Gizmos: public Singleton<Gizmos>
    {
    public:
        static const Color4 colliderColor; // Default color of physics colliders gizmos
        static const Color4 jointColor;    // Default color of physics joints gizmos
        static const Color4 cameraColor;   // Default color of camera gizmos

    public:
        // Default constructor
        Gizmos(RefCounter* refCounter);

        // Sets projection from world point to drawing space point
        void SetProjection(const Function<Vec2F(const Vec3F&)>& projection);

        // Resets projection to plane projection, which drops z coordinate
        void ResetProjection();

        // Sets color for next drawings
        void SetColor(const Color4& color);

        // Returns current drawing color
        const Color4& GetColor() const;

        // Draws line between world points
        void DrawLine(const Vec3F& begin, const Vec3F& end);

        // Draws poly line by world points, closes it when closed is true
        void DrawPolyLine(const Vector<Vec3F>& points, bool closed = false);

        // Draws circle in plane, defined by two perpendicular axes
        void DrawCircle(const Vec3F& center, const Vec3F& axisU, const Vec3F& axisV, float radius, int segments = 32);

        // Draws circle in xy plane
        void DrawCircle(const Vec3F& center, float radius, int segments = 32);

        // Draws rectangle by center and two half axes
        void DrawRect(const Vec3F& center, const Vec3F& halfAxisX, const Vec3F& halfAxisY);

        // Draws box wireframe by center and three half axes
        void DrawBox(const Vec3F& center, const Vec3F& halfAxisX, const Vec3F& halfAxisY, const Vec3F& halfAxisZ);

        // Draws sphere wireframe by three rings
        void DrawSphere(const Vec3F& center, float radius, int segments = 32);

        // Draws capsule wireframe, oriented along up axis, height is distance between cap centers
        void DrawCapsule(const Vec3F& center, const Vec3F& up, float radius, float height, int segments = 24);

        // Draws cross mark on point
        void DrawPoint(const Vec3F& point, float size = 5.0f);

        // Returns count of primitives, drawn since last ResetDrawnPrimitives
        int GetDrawnPrimitives() const;

        // Resets drawn primitives counter
        void ResetDrawnPrimitives();

    private:
        Function<Vec2F(const Vec3F&)> mProjection; // World point to drawing space point projection

        Color4 mColor = colliderColor; // Current drawing color

        int mDrawnPrimitives = 0; // Count of primitives, drawn since last counter reset

    private:
        // Projects world points and draws them as poly line
        void DrawProjectedLine(const Vector<Vec3F>& points, bool closed);
    };
}
