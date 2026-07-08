#pragma once

#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Transform.h"
#include "o2/Utils/Math/Vector3.h"


namespace o2
{
    class Texture;

    // ------
    // Camera
    // ------
    class Camera: public Transform
    {
    public:
        enum class Projection { Orthographic, Perspective, Orthographic3D };

    public:
        Projection projection = Projection::Orthographic; // Projection type @SERIALIZABLE
        float      fov = Math::Deg2rad(60.0f);            // Vertical field of view in radians, perspective only @SERIALIZABLE
        float      nearClip = 0.1f;                       // Near clipping plane, perspective only @SERIALIZABLE
        float      farClip = 1000.0f;                     // Far clipping plane, perspective only @SERIALIZABLE

    public:
        // Constructor
        Camera(const Vec2F& position = Vec2F(), const Vec2F& size = Vec2F(), float angle = 0.0f);

        // Equals operator
        bool operator==(const Camera& other) const;

        // Not equals operator
        bool operator!=(const Camera& other) const;

        // Returns view matrix for perspective mode: inverse of position and rotation transform
        Mat4 GetViewMatrix3D() const;

        // Returns projection matrix for viewport size
        Mat4 GetProjectionMatrix(const Vec2F& viewportSize) const;

        // Returns default camera
        static Camera Default();

        // Returns camera with fixed size
        static Camera FixedSize(const Vec2F& size);

        // Returns camera with fixed aspect
        static Camera FittedSize(const Vec2F& size);

        // Returns camera with physical correct units
        static Camera PhysicalCorrect(Units units);

        // Returns perspective camera
        static Camera Perspective(float fov, float nearClip, float farClip);

        // Returns 3D orthographic camera with view volume width and height; position and rotation drive the view
        static Camera Orthographic3D(float width, float height, float nearClip, float farClip);

        SERIALIZABLE(Camera);

    protected:
        // Called when object was deserialized; reads legacy position3D/rotation3D members
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

PRE_ENUM_META(o2::Camera::Projection);

CLASS_BASES_META(o2::Camera)
{
    BASE_CLASS(o2::Transform);
}
END_META;
CLASS_FIELDS_META(o2::Camera)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Projection::Orthographic).NAME(projection);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Math::Deg2rad(60.0f)).NAME(fov);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.1f).NAME(nearClip);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1000.0f).NAME(farClip);
}
END_META;
CLASS_METHODS_META(o2::Camera)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, const Vec2F&, float);
    FUNCTION().PUBLIC().SIGNATURE(Mat4, GetViewMatrix3D);
    FUNCTION().PUBLIC().SIGNATURE(Mat4, GetProjectionMatrix, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Camera, Default);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Camera, FixedSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Camera, FittedSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Camera, PhysicalCorrect, Units);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Camera, Perspective, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Camera, Orthographic3D, float, float, float, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
