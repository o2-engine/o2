#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Editor/Attributes/AnimatableAttribute.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class Transform : virtual public ISerializable
    {
    public:
        PROPERTIES(Transform);
        PROPERTY(Vec2F, position, SetPosition, GetPosition);            // Position property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, positionX, SetPositionX, GetPositionX);         // Position property by X @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, positionY, SetPositionY, GetPositionY);         // Position property by Y @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, size, SetSize, GetSize);                        // Size property @EDITOR_IGNORE @SCRIPTABLE
        PROPERTY(float, width, SetWidth, GetWidth);                     // Width property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, height, SetHeight, GetHeight);                  // Height property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, scale, SetScale, GetScale);                     // Scale property @EDITOR_IGNORE @SCRIPTABLE
        PROPERTY(float, scaleX, SetScaleX, GetScaleX);                  // Scale X property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, scaleY, SetScaleY, GetScaleY);                  // Scale Y property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, pivot, SetPivot, GetPivot);                     // Pivot property, in local space @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, worldPivot, SetWorldPivot, GetWorldPivot);      // Pivot property, in world space @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(Vec2F, szPivot, SetSizePivot, GetSizePivot);           // Pivot in size space property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, angle, SetAngle, GetAngle);                     // Rotation angle in radians @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, angleDegree, SetAngleDegrees, GetAngleDegrees); // Rotation angle in degrees @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE
        PROPERTY(float, shear, SetShear, GetShear);                     // Shear property @EDITOR_IGNORE @ANIMATABLE @SCRIPTABLE

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
        virtual void SetPosition(const Vec2F& position);

        // Returns position
        virtual Vec2F GetPosition() const;

        // Sets position by X
        virtual void SetPositionX(float value);

        // Returns position by X
        virtual float GetPositionX() const;

        // Sets position by Y
        virtual void SetPositionY(float value);

        // Returns position by Y
        virtual float GetPositionY() const;

        // Sets size
        virtual void SetSize(const Vec2F& size);

        // Return size
        virtual Vec2F GetSize() const;

        // Sets width
        virtual void SetWidth(float width);

        // Returns width
        virtual float GetWidth() const;

        // Sets height
        virtual void SetHeight(float height);

        // Returns width
        virtual float GetHeight() const;

        // Sets pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top
        virtual void SetPivot(const Vec2F& pivot);

        // Return pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top
        virtual Vec2F GetPivot() const;

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
        virtual void SetScale(const Vec2F& scale);

        // Returns scale
        virtual Vec2F GetScale() const;

        // Sets scale by X
        virtual void SetScaleX(float scale);

        // Returns scale by X
        virtual float GetScaleX() const;

        // Sets scale by Y
        virtual void SetScaleY(float scale);

        // Returns scale by Y
        virtual float GetScaleY() const;

        // Sets rotation angle, in radians
        virtual void SetAngle(float rad);

        // Returns rotation angle in radians
        virtual float GetAngle() const;

        // Sets rotation angle, in degrees
        virtual void SetAngleDegrees(float deg);

        // Returns rotation angle in degrees
        virtual float GetAngleDegrees() const;

        // Sets shear
        virtual void SetShear(float shear);

        // Returns shear
        virtual float GetShear() const;

        // Sets basis
        virtual void SetBasis(const Basis& basis);

        // Returns basis
        virtual Basis GetBasis() const;

        // Sets basis without size
        virtual void SetNonSizedBasis(const Basis& basis);

        // Returns basis without size
        virtual Basis GetNonSizedBasis() const;

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
        Vec2F  mPosition; // Position @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
        Vec2F  mSize;     // Size @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
        Vec2F  mScale;    // Scale, (1; 1) is default @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
        Vec2F  mPivot;    // Pivot: (0; 0) is left bottom corner - (1; 1) is right top corner @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
        float  mAngle;    // Rotation angle in radians @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)
        float  mShear;    // Shear @SERIALIZABLE @SERIALIZE_IF(IsSerializeEnabled)

        Basis  mTransform;         // Final transform basis
        Basis  mNonSizedTransform; // Final transform basis without size

        bool mSerializeEnabled = true; // Is serializations fields enabled

    protected:
        // Called when basis changed
        virtual void BasisChanged() { }

        // Called when object was deserialized
        void OnDeserialized(const DataValue& node) override;

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
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(positionX);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(positionY);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(size);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(width);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(height);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scale);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scaleX);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(scaleY);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(pivot);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(worldPivot);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(szPivot);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(angle);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(angleDegree);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_IGNORE_ATTRIBUTE().SCRIPTABLE_ATTRIBUTE().NAME(shear);
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
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mPosition);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mSize);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mScale);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mPivot);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mAngle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().SERIALIZE_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mShear);
    FIELD().PROTECTED().NAME(mTransform);
    FIELD().PROTECTED().NAME(mNonSizedTransform);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mSerializeEnabled);
}
END_META;
CLASS_METHODS_META(o2::Transform)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Transform&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPositionX, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPositionX);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPositionY, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPositionY);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWorldPivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetWorldPivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSizePivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSizePivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRect, const RectF&, bool);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetRect);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScale, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScaleX, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScaleX);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScaleY, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScaleY);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngleDegrees, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngleDegrees);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShear, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetShear);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBasis, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetBasis);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNonSizedBasis, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetNonSizedBasis);
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
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransform);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsSerializeEnabled);
}
END_META;
// --- END META ---
