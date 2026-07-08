#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Editor/Attributes/AnimatableAttribute.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Editor/Attributes/PrototypeDeltaSearchAttribute.h"
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class Transform : virtual public ISerializable
    {
    public:
        PROPERTIES(Transform);
        PROPERTY(Vec3F, position, SetPosition, GetPosition);         // Position property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, position2D, SetPosition2D, GetPosition2D);   // 2D position property, works with xy @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, positionX, SetPositionX, GetPositionX);      // Position property by X @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, positionY, SetPositionY, GetPositionY);      // Position property by Y @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, positionZ, SetPositionZ, GetPositionZ);      // Position property by Z @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec3F, size, SetSize, GetSize);                     // Size property @EDITOR_IGNORE @SCRIPTABLE
        PROPERTY(Vec2F, size2D, SetSize2D, GetSize2D);               // 2D size property, works with xy @EDITOR_IGNORE @SCRIPTABLE
        PROPERTY(float, width, SetWidth, GetWidth);                  // Width property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, height, SetHeight, GetHeight);               // Height property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, sizeZ, SetSizeZ, GetSizeZ);                  // Size property by Z @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec3F, scale, SetScale, GetScale);                  // Scale property @EDITOR_IGNORE @SCRIPTABLE
        PROPERTY(Vec2F, scale2D, SetScale2D, GetScale2D);            // 2D scale property, works with xy @EDITOR_IGNORE @SCRIPTABLE
        PROPERTY(float, scaleX, SetScaleX, GetScaleX);               // Scale X property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, scaleY, SetScaleY, GetScaleY);               // Scale Y property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, scaleZ, SetScaleZ, GetScaleZ);               // Scale Z property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec3F, pivot, SetPivot, GetPivot);                  // Pivot property, in local space @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, pivot2D, SetPivot2D, GetPivot2D);            // 2D pivot property, works with xy @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, worldPivot, SetWorldPivot, GetWorldPivot);   // Pivot property, in world space @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, szPivot, SetSizePivot, GetSizePivot);        // Pivot in size space property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, angle, SetAngle, GetAngle);                  // Rotation angle in radians @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, angleDegree, SetAngleDegrees, GetAngleDegrees); // Rotation angle in degrees @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec3F, shear, SetShear, GetShear);                  // Shear property: x - XY plane, y - XZ, z - YZ @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, shear2D, SetShear2D, GetShear2D);            // 2D shear property, works with x @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE

        PROPERTY(Vec3F, eulerAngles, SetEulerAngles, GetEulerAngles); // Rotation euler angles in radians, z is the 2D angle @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec3F, eulerAnglesDegrees, SetEulerAnglesDegrees, GetEulerAnglesDegrees); // Rotation euler angles in degrees @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Quat, rotation, SetRotation, GetRotation);           // Rotation quaternion property @EDITOR_IGNORE @SCRIPTABLE

        PROPERTY(Basis, basis, SetBasis, GetBasis);                         // Transformation basis property @EDITOR_IGNORE @ANIMATABLE
        PROPERTY(Basis, nonSizedBasis, SetNonSizedBasis, GetNonSizedBasis); // Non sizes transformation basis property @EDITOR_IGNORE @ANIMATABLE

        PROPERTY(RectF, rect, SetRect, GetRect);                       // Rectangle property. Sets the position and size @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(RectF, AABB, SetAxisAlignedRect, GetAxisAlignedRect); // Axis aligned rectangle @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE

        PROPERTY(Vec2F, leftTop, SetLeftTop, GetLeftTop);             // Left top corner property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, leftBottom, SetLeftBottom, GetLeftBottom);    // Left bottom corner property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, rightTop, SetRightTop, GetRightTop);          // Left top corner property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, rightBottom, SetRightBottom, GetRightBottom); // Left top corner property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, center, SetCenter, GetCenter);                // Center property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE

        PROPERTY(Vec2F, up, SetUp, GetUp); // Y Axis direction property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE

    public:
        // Constructor
        Transform(const Vec2F& size = Vec2F(), const Vec2F& position = Vec2F(), float angle = 0.0f,
                  const Vec2F& scale = Vec2F(1.0f, 1.0f), const Vec2F& pivot = Vec2F(0.5f, 0.5f));

        // Copy-constructor
        Transform(const Transform& other);

        // Virtual destructor
        virtual ~Transform() { }

        // Assign operator
        Transform& operator=(const Transform& other);

        // Check equals operator
        bool operator==(const Transform& other) const;

        // Not equals operator
        bool operator!=(const Transform& other) const;

        // Sets position
        virtual void SetPosition(const Vec3F& position);

        // Returns position
        virtual Vec3F GetPosition() const;

        // Sets 2D position, leaves z unchanged
        virtual void SetPosition2D(const Vec2F& position);

        // Returns 2D position
        virtual Vec2F GetPosition2D() const;

        // Sets position by X
        virtual void SetPositionX(float value);

        // Returns position by X
        virtual float GetPositionX() const;

        // Sets position by Y
        virtual void SetPositionY(float value);

        // Returns position by Y
        virtual float GetPositionY() const;

        // Sets position by Z
        virtual void SetPositionZ(float value);

        // Returns position by Z
        virtual float GetPositionZ() const;

        // Sets size
        virtual void SetSize(const Vec3F& size);

        // Return size
        virtual Vec3F GetSize() const;

        // Sets 2D size, leaves z unchanged
        virtual void SetSize2D(const Vec2F& size);

        // Returns 2D size
        virtual Vec2F GetSize2D() const;

        // Sets width
        virtual void SetWidth(float width);

        // Returns width
        virtual float GetWidth() const;

        // Sets height
        virtual void SetHeight(float height);

        // Returns width
        virtual float GetHeight() const;

        // Sets size by Z
        virtual void SetSizeZ(float value);

        // Returns size by Z
        virtual float GetSizeZ() const;

        // Sets pivot, in local space, where (0, 0, 0) is left down near corner, (1, 1, 1) - right top far
        virtual void SetPivot(const Vec3F& pivot);

        // Return pivot, in local space
        virtual Vec3F GetPivot() const;

        // Sets 2D pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top; leaves z unchanged
        virtual void SetPivot2D(const Vec2F& pivot);

        // Returns 2D pivot
        virtual Vec2F GetPivot2D() const;

        // Sets pivot by world coordinates
        virtual void SetWorldPivot(const Vec2F& pivot);

        // Returns pivot position in world coordinates
        virtual Vec2F GetWorldPivot() const;

        // Sets size pivot, in local space, where (0, 0) - left down corner, (mSize.x, mSize.y) - right top
        virtual void SetSizePivot(const Vec2F& relPivot);

        // Returns size pivot, in local space, where (0, 0) - left down corner, (mSize.x, mSize.y) - right top
        virtual Vec2F GetSizePivot() const;

        // Sets rect. If bySIze is true, rectangle adjusting by transform's size, overwise by scale
        virtual void SetRect(const RectF& rect, bool bySize = true);

        // Returns rect
        virtual RectF GetRect() const;

        // Sets scale
        virtual void SetScale(const Vec3F& scale);

        // Returns scale
        virtual Vec3F GetScale() const;

        // Sets 2D scale, leaves z unchanged
        virtual void SetScale2D(const Vec2F& scale);

        // Returns 2D scale
        virtual Vec2F GetScale2D() const;

        // Sets scale by X
        virtual void SetScaleX(float scale);

        // Returns scale by X
        virtual float GetScaleX() const;

        // Sets scale by Y
        virtual void SetScaleY(float scale);

        // Returns scale by Y
        virtual float GetScaleY() const;

        // Sets scale by Z
        virtual void SetScaleZ(float scale);

        // Returns scale by Z
        virtual float GetScaleZ() const;

        // Sets rotation angle, in radians
        virtual void SetAngle(float rad);

        // Returns rotation angle in radians
        virtual float GetAngle() const;

        // Sets rotation angle, in degrees
        virtual void SetAngleDegrees(float deg);

        // Returns rotation angle in degrees
        virtual float GetAngleDegrees() const;

        // Sets rotation euler angles in radians, z is the 2D angle
        virtual void SetEulerAngles(const Vec3F& radians);

        // Returns rotation euler angles in radians
        virtual Vec3F GetEulerAngles() const;

        // Sets rotation euler angles in degrees
        virtual void SetEulerAnglesDegrees(const Vec3F& degrees);

        // Returns rotation euler angles in degrees
        virtual Vec3F GetEulerAnglesDegrees() const;

        // Sets rotation quaternion
        virtual void SetRotation(const Quat& rotation);

        // Returns rotation quaternion
        virtual Quat GetRotation() const;

        // Sets shear: x - XY plane, y - XZ, z - YZ
        virtual void SetShear(const Vec3F& shear);

        // Returns shear
        virtual Vec3F GetShear() const;

        // Sets 2D shear (XY plane), leaves other planes unchanged
        virtual void SetShear2D(float shear);

        // Returns 2D shear (XY plane)
        virtual float GetShear2D() const;

        // Sets basis
        virtual void SetBasis(const Basis& basis);

        // Returns basis
        virtual Basis GetBasis() const;

        // Sets basis without size
        virtual void SetNonSizedBasis(const Basis& basis);

        // Returns basis without size
        virtual Basis GetNonSizedBasis() const;

        // Returns 3D basis with size
        virtual Basis3D GetBasis3D() const;

        // Returns 3D basis without size
        virtual Basis3D GetNonSizedBasis3D() const;

        // Sets axis aligned rectangle transformation
        virtual void SetAxisAlignedRect(const RectF& rect);

        // Returns axis aligned rectangle transformation
        virtual RectF GetAxisAlignedRect() const;

        // Sets left top corner position
        virtual void SetLeftTop(const Vec2F& position);

        // Returns left top corner position
        virtual Vec2F GetLeftTop() const;

        // Sets right top corner position
        virtual void SetRightTop(const Vec2F& position);

        // Returns right top corner position
        virtual Vec2F GetRightTop() const;

        // Sets left down corner position
        virtual void SetLeftBottom(const Vec2F& position);

        // Returns left down corner position
        virtual Vec2F GetLeftBottom() const;

        // Sets left right bottom position
        virtual void SetRightBottom(const Vec2F& position);

        // Returns right bottom corner position
        virtual Vec2F GetRightBottom() const;

        // Sets center position
        virtual void SetCenter(const Vec2F& position);

        // Returns center position
        virtual Vec2F GetCenter() const;

        // Set local x axis direction
        virtual void SetRight(const Vec2F& dir);

        // Returns local x axis direction
        virtual Vec2F GetRight() const;

        // Set negative local x axis direction
        virtual void SetLeft(const Vec2F& dir);

        // Returns negative local x axis direction
        virtual Vec2F GetLeft() const;

        // Set local y axis direction
        virtual void SetUp(const Vec2F& dir);

        // Returns local y axis direction
        virtual Vec2F GetUp() const;

        // Set negative local y axis direction
        virtual void SetDown(const Vec2F& dir);

        // Returns negative local y axis direction
        virtual Vec2F GetDown() const;

        // Sets x axis directed to worldPoint
        virtual void LookAt(const Vec2F& worldPoint);

        // Transforms point from world space into local @SCRIPTABLE
        virtual Vec2F World2LocalPoint(const Vec2F& worldPoint) const;

        // Transforms point from local space into world @SCRIPTABLE
        virtual Vec2F Local2WorldPoint(const Vec2F& localPoint) const;

        // Transforms direction from world space into local @SCRIPTABLE
        virtual Vec2F World2LocalDir(const Vec2F& worldDir) const;

        // Transforms direction from local space into world @SCRIPTABLE
        virtual Vec2F Local2WorldDir(const Vec2F& localDir) const;

        // Returns true when point inside this @SCRIPTABLE
        virtual bool IsPointInside(const Vec2F& point) const;

        // Sets serialization enable or disable
        void SetSerializeEnabled(bool enabled);

        SERIALIZABLE(Transform);

    protected:
        Vec3F mPosition;               // Position, 2D API works with xy @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mSize;                   // Size, 2D content has zero z @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mScale = Vec3F(1, 1, 1); // Scale, (1, 1, 1) is default @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mPivot;                  // Pivot: (0, 0, 0) is left bottom near corner - (1, 1, 1) is right top far corner @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mEulerAngles;            // Rotation euler angles in radians, z is the 2D angle @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mShear;                  // Shear planes: x - XY, y - XZ, z - YZ; 2D API works with x @DELTA_SEARCH_IF(IsSerializeEnabled)

        Basis3D mTransform;         // Final transform basis
        Basis3D mNonSizedTransform; // Final transform basis without size

        bool mSerializeEnabled = true; // Is serializations fields enabled

    protected:
        // Called when basis changed
        virtual void BasisChanged() { }

        // Beginning serialization callback, writes data
        void OnSerialize(DataValue& node) const override;

        // Called when object was deserialized, reads data
        void OnDeserialized(const DataValue& node) override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        // Updates mTransform
        virtual void UpdateTransform();

        // Returns is serialize enabled; used to turn off fields serialization
        virtual bool IsSerializeEnabled() const;
    };
}
// --- META ---

CLASS_BASES_META(o2::Transform)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::Transform)
{
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(position);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(position2D);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(positionX);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(positionY);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(positionZ);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(size);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(size2D);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(width);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(height);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(sizeZ);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scale);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scale2D);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scaleX);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scaleY);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scaleZ);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(pivot);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(pivot2D);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(worldPivot);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(szPivot);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(angle);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(angleDegree);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(shear);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(shear2D);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(eulerAngles);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(eulerAnglesDegrees);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(rotation);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().NAME(basis);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().NAME(nonSizedBasis);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(rect);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(AABB);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(leftTop);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(leftBottom);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(rightTop);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(rightBottom);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(center);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(up);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mPosition);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mSize);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).DEFAULT_VALUE(Vec3F(1, 1, 1)).NAME(mScale);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mPivot);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mEulerAngles);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mShear);
    FIELD().PROTECTED().NAME(mTransform);
    FIELD().PROTECTED().NAME(mNonSizedTransform);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mSerializeEnabled);
}
END_META;
CLASS_METHODS_META(o2::Transform)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Transform&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPosition2D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPositionX, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPositionX);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPositionY, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPositionY);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPositionZ, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPositionZ);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSize2D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSizeZ, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSizeZ);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPivot, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetPivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPivot2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPivot2D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWorldPivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetWorldPivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSizePivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSizePivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRect, const RectF&, bool);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetRect);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScale, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScale2D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetScale2D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScaleX, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScaleX);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScaleY, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScaleY);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScaleZ, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScaleZ);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngleDegrees, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngleDegrees);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEulerAngles, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetEulerAngles);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEulerAnglesDegrees, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetEulerAnglesDegrees);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRotation, const Quat&);
    FUNCTION().PUBLIC().SIGNATURE(Quat, GetRotation);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShear, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetShear);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShear2D, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetShear2D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBasis, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetBasis);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNonSizedBasis, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetNonSizedBasis);
    FUNCTION().PUBLIC().SIGNATURE(Basis3D, GetBasis3D);
    FUNCTION().PUBLIC().SIGNATURE(Basis3D, GetNonSizedBasis3D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAxisAlignedRect, const RectF&);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetAxisAlignedRect);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeftTop, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetLeftTop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRightTop, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetRightTop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeftBottom, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetLeftBottom);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRightBottom, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetRightBottom);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCenter, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetCenter);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRight, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetRight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeft, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetLeft);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUp, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetUp);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDown, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetDown);
    FUNCTION().PUBLIC().SIGNATURE(void, LookAt, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, World2LocalPoint, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, Local2WorldPoint, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, World2LocalDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, Local2WorldDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsPointInside, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSerializeEnabled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransform);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsSerializeEnabled);
}
END_META;
// --- END META ---
