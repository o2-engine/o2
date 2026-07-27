#pragma once

#include "o2/Utils/Editor/Attributes/PrototypeDeltaSearchAttribute.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Transform.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    class Actor;

    // -------------------------------------------------------------------------------------------
    // Actor transform. Represents the position of actor relative to his parent (the local space),
    // and calculates the position relative to world (the world space).
    // Position shows the pivot point, and pivot shows where is the rectangle of actor.
    // Also this rectangle has angle, scale, size and shear.
    //
    // Note: the rectangle of transform here means the rectangle without angle, scale and shear.
    // So, the final transform is rotated, scaled and sheared rectangle.
    // -------------------------------------------------------------------------------------------
    class ActorTransform: public ISerializable
    {
    public:
        PROPERTIES(ActorTransform);
        GETTER(Ref<Actor>, actor, GetOwnerActor); // Owner actor getter 

        PROPERTY(Vec3F, position, SetPosition, GetPosition);             // Position property
        PROPERTY(Vec2F, position2D, SetPosition2D, GetPosition2D);       // 2D position property, works with xy
        PROPERTY(float, positionX, SetPositionX, GetPositionX);          // Position property by X
        PROPERTY(float, positionY, SetPositionY, GetPositionY);          // Position property by Y
        PROPERTY(float, positionZ, SetPositionZ, GetPositionZ);          // Position property by Z
        PROPERTY(Vec3F, size, SetSize, GetSize);                         // Size property
        PROPERTY(Vec2F, size2D, SetSize2D, GetSize2D);                   // 2D size property, works with xy
        PROPERTY(float, width, SetWidth, GetWidth);                      // Width property
        PROPERTY(float, height, SetHeight, GetHeight);                   // Height property
        PROPERTY(float, sizeZ, SetSizeZ, GetSizeZ);                      // Size property by Z
        PROPERTY(Vec3F, scale, SetScale, GetScale);                      // Scale property
        PROPERTY(Vec2F, scale2D, SetScale2D, GetScale2D);                // 2D scale property, works with xy
        PROPERTY(float, scaleX, SetScaleX, GetScaleX);                   // Scale property by X
        PROPERTY(float, scaleY, SetScaleY, GetScaleY);                   // Scale property by Y
        PROPERTY(float, scaleZ, SetScaleZ, GetScaleZ);                   // Scale property by Z
        PROPERTY(Vec3F, pivot, SetPivot, GetPivot);                      // Pivot property, in local space
        PROPERTY(Vec2F, pivot2D, SetPivot2D, GetPivot2D);                // 2D pivot property, works with xy
        PROPERTY(Vec2F, szPivot, SetSizePivot, GetSizePivot);            // Pivot in size space property
        PROPERTY(float, pivotX, SetPivotX, GetPivotX);                   // Pivot property by X
        PROPERTY(float, pivotY, SetPivotY, GetPivotY);                   // Pivot property by Y
        PROPERTY(float, pivotZ, SetPivotZ, GetPivotZ);                   // Pivot property by Z
        PROPERTY(float, angle, SetAngle, GetAngle);                      // Rotation angle in radians
        PROPERTY(float, angleDegrees, SetAngleDegrees, GetAngleDegrees); // Rotation angle in degrees
        PROPERTY(Vec3F, shear, SetShear, GetShear);                      // Shear property: x - XY plane, y - XZ, z - YZ
        PROPERTY(float, shear2D, SetShear2D, GetShear2D);                // 2D shear property, works with x

        PROPERTY(Vec3F, eulerAngles, SetEulerAngles, GetEulerAngles); // Rotation euler angles in radians, z is the 2D angle
        PROPERTY(Vec3F, eulerAnglesDegrees, SetEulerAnglesDegrees, GetEulerAnglesDegrees); // Rotation euler angles in degrees
        PROPERTY(Quat, rotation, SetRotation, GetRotation);           // Rotation quaternion property

        PROPERTY(Basis, basis, SetBasis, GetBasis);                         // Transformation basis property
        PROPERTY(Basis, nonSizedBasis, SetNonSizedBasis, GetNonSizedBasis); // Non sizes transformation basis property

        PROPERTY(RectF, AABB, SetAxisAlignedRect, GetAxisAlignedRect); // Axis aligned rectangle
        PROPERTY(RectF, rect, SetRect, GetRect);                       // Rectangle property. Rectangle - transform without angle, scale and shear. 
                                                                       // Sets the position and size

        PROPERTY(Vec2F, leftTop, SetLeftTop, GetLeftTop);             // Left top corner property
        PROPERTY(Vec2F, leftBottom, SetLeftBottom, GetLeftBottom);    // Left bottom corner property
        PROPERTY(Vec2F, rightTop, SetRightTop, GetRightTop);          // Right top corner property
        PROPERTY(Vec2F, rightBottom, SetRightBottom, GetRightBottom); // Right bottom corner property
        PROPERTY(Vec2F, center, SetCenter, GetCenter);                // Center position property
        PROPERTY(Vec2F, downDir, SetDownDir, GetDownDir);             // Negative Y axis direction property
        PROPERTY(float, right, SetRight, GetRight);                   // Right border position property
        PROPERTY(float, left, SetLeft, GetLeft);                      // Left border position property
        PROPERTY(float, top, SetTop, GetTop);                         // Top border position property
        PROPERTY(float, bottom, SetBottom, GetBottom);                // Bottom border position property

        PROPERTY(Vec3F, worldPosition, SetWorldPosition, GetWorldPosition);                // World position property
        PROPERTY(Vec2F, worldPosition2D, SetWorldPosition2D, GetWorldPosition2D);          // 2D world position property, works with xy
        PROPERTY(Vec2F, worldPivot, SetWorldPivot, GetWorldPivot);                         // Pivot property, in world space
        PROPERTY(float, worldAngle, SetWorldAngle, GetWorldAngle);                         // World rotation angle in radians
        PROPERTY(float, worldAngleDegree, SetWorldAngleDegree, GetWorldAngleDegree);       // World rotation angle in degree
        PROPERTY(Basis, worldBasis, SetWorldBasis, GetWorldBasis);                         // World transformation basis
        PROPERTY(Basis, worldNonSizedBasis, SetWorldNonSizedBasis, GetWorldNonSizedBasis); // World transformation basis without size

        PROPERTY(Vec2F, worldLeftTop, SetWorldLeftTop, GetWorldLeftTop);             // World Left top corner property
        PROPERTY(Vec2F, worldLeftBottom, SetWorldLeftBottom, GetWorldLeftBottom);    // World Left bottom corner property
        PROPERTY(Vec2F, worldRightTop, SetWorldRightTop, GetWorldRightTop);          // World Right top corner property
        PROPERTY(Vec2F, worldRightBottom, SetWorldRightBottom, GetWorldRightBottom); // World Right bottom corner property
        PROPERTY(Vec2F, worldCenter, SetWorldCenter, GetWorldCenter);                // World center property
        PROPERTY(Vec2F, worldUpDir, SetWorldUpDir, GetWorldUpDir);                   // World Y axis direction property

        PROPERTY(float, worldRight, SetWorldRight, GetWorldRight);    // World Right border position property
        PROPERTY(float, worldLeft, SetWorldLeft, GetWorldLeft);       // World Left border position property
        PROPERTY(float, worldTop, SetWorldTop, GetWorldTop);          // World Top border position property
        PROPERTY(float, worldBottom, SetWorldBottom, GetWorldBottom); // World Bottom border position property

        PROPERTY(RectF, worldRect, SetWorldRect, GetWorldRect);                       // World rectangle property. Sets the position and size
        PROPERTY(RectF, worldAABB, SetWorldAxisAlignedRect, GetWorldAxisAlignedRect); // World direction aligned rectangle

    public:
        ActorTransform(const Vec2F& size = Vec2F(), const Vec2F& position = Vec2F(), float angle = 0.0f,
                       const Vec2F& scale = Vec2F(1.0f, 1.0f), const Vec2F& pivot = Vec2F(0.5f, 0.5f));

        // Copy-constructor
        ActorTransform(const ActorTransform& other);

        // Destructor
        ~ActorTransform();

        // Assign operator
        ActorTransform& operator=(const ActorTransform& other);

        // Assign operator
        //ActorTransform& operator=(const Transform& other);

        // Check EqualSid operator
        bool operator==(const ActorTransform& other) const;

        // Returns owner actor
        Ref<Actor> GetOwnerActor() const;

        // Sets transform dirty and needed to update @SCRIPTABLE
        virtual void SetDirty(bool fromParent = false);

        // Returns is transform dirty @SCRIPTABLE
        bool IsDirty() const;

        // Updates transformation
        virtual void Update();

        // Sets position @SCRIPTABLE
        void SetPosition(const Vec3F& position);

        // Returns position @SCRIPTABLE
        Vec3F GetPosition() const;

        // Sets 2D position, leaves z unchanged @SCRIPTABLE
        virtual void SetPosition2D(const Vec2F& position);

        // Returns 2D position @SCRIPTABLE
        Vec2F GetPosition2D() const;

        // Sets position by X @SCRIPTABLE
        virtual void SetPositionX(float value);

        // Returns position by X @SCRIPTABLE
        float GetPositionX() const;

        // Sets position by Y @SCRIPTABLE
        virtual void SetPositionY(float value);

        // Returns position by Y @SCRIPTABLE
        float GetPositionY() const;

        // Sets size @SCRIPTABLE
        void SetSize(const Vec3F& size);

        // Return size @SCRIPTABLE
        Vec3F GetSize() const;

        // Sets 2D size, leaves z unchanged @SCRIPTABLE
        virtual void SetSize2D(const Vec2F& size);

        // Return 2D size @SCRIPTABLE
        virtual Vec2F GetSize2D() const;

        // Sets width @SCRIPTABLE
        virtual void SetWidth(float value);

        // Return width @SCRIPTABLE
        virtual float GetWidth() const;

        // Sets height @SCRIPTABLE
        virtual void SetHeight(float value);

        // Return height @SCRIPTABLE
        virtual float GetHeight() const;

        // Sets pivot @SCRIPTABLE
        void SetPivot(const Vec3F& pivot);

        // Returns pivot @SCRIPTABLE
        Vec3F GetPivot() const;

        // Sets 2D pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top; leaves z unchanged @SCRIPTABLE
        virtual void SetPivot2D(const Vec2F& pivot);

        // Return 2D pivot, in local space, where (0, 0) - left down corner, (1; 1) - right top @SCRIPTABLE
        Vec2F GetPivot2D() const;

        // Sets size pivot, in local space, where (0, 0) - left down corner, (size.x, size.y) - right top @SCRIPTABLE
        void SetSizePivot(const Vec2F& relPivot);

        // Returns size pivot, in local space, where (0, 0) - left down corner, (size.x, size.y) - right top @SCRIPTABLE
        Vec2F GetSizePivot() const;

        // Sets pivot by X @SCRIPTABLE
        void SetPivotX(float value);

        // Returns pivot by X @SCRIPTABLE
        float GetPivotX() const;

        // Sets pivot by Y @SCRIPTABLE
        void SetPivotY(float value);
        
        // Returns pivot by Y @SCRIPTABLE
        float GetPivotY() const;

        // Sets scale @SCRIPTABLE
        void SetScale(const Vec3F& scale);

        // Returns scale @SCRIPTABLE
        Vec3F GetScale() const;

        // Sets 2D scale, leaves z unchanged @SCRIPTABLE
        void SetScale2D(const Vec2F& scale);

        // Returns 2D scale @SCRIPTABLE
        Vec2F GetScale2D() const;

        // Sets scale by X @SCRIPTABLE
        void SetScaleX(float scaleX);

        // Sets scale by Y @SCRIPTABLE
        void SetScaleY(float scaleY);

        // Returns scale by X @SCRIPTABLE
        float GetScaleX() const;

        // Returns scale by Y @SCRIPTABLE
        float GetScaleY() const;

        // Sets rotation angle, in radians @SCRIPTABLE
        void SetAngle(float rad);

        // Returns rotation angle in radians @SCRIPTABLE
        float GetAngle() const;

        // Sets rotation angle, in degrees @SCRIPTABLE
        void SetAngleDegrees(float deg);

        // Returns rotation angle in degrees @SCRIPTABLE
        float GetAngleDegrees() const;

        // Sets shear: x - XY plane, y - XZ, z - YZ @SCRIPTABLE
        void SetShear(const Vec3F& shear);

        // Returns shear @SCRIPTABLE
        Vec3F GetShear() const;

        // Sets 2D shear (XY plane), leaves other planes unchanged @SCRIPTABLE
        void SetShear2D(float shear);

        // Returns 2D shear (XY plane) @SCRIPTABLE
        float GetShear2D() const;

        // Sets position by Z @SCRIPTABLE
        void SetPositionZ(float value);

        // Returns position by Z @SCRIPTABLE
        float GetPositionZ() const;

        // Sets scale by Z @SCRIPTABLE
        void SetScaleZ(float value);

        // Returns scale by Z @SCRIPTABLE
        float GetScaleZ() const;

        // Sets size by Z @SCRIPTABLE
        void SetSizeZ(float value);

        // Returns size by Z @SCRIPTABLE
        float GetSizeZ() const;

        // Sets pivot by Z @SCRIPTABLE
        void SetPivotZ(float value);

        // Returns pivot by Z @SCRIPTABLE
        float GetPivotZ() const;

        // Sets rotation euler angles in radians, z is the 2D angle @SCRIPTABLE
        void SetEulerAngles(const Vec3F& radians);

        // Returns rotation euler angles in radians @SCRIPTABLE
        Vec3F GetEulerAngles() const;

        // Sets rotation euler angles in degrees @SCRIPTABLE
        void SetEulerAnglesDegrees(const Vec3F& degrees);

        // Returns rotation euler angles in degrees @SCRIPTABLE
        Vec3F GetEulerAnglesDegrees() const;

        // Sets rotation quaternion @SCRIPTABLE
        void SetRotation(const Quat& rotation);

        // Returns rotation quaternion @SCRIPTABLE
        Quat GetRotation() const;

        // Returns local 3D transform matrix; matches the non-sized 2D transform when not 3D @SCRIPTABLE
        Mat4 GetLocalTransform3D() const;

        // Returns world 3D transform matrix, composed through parents @SCRIPTABLE
        Mat4 GetWorldTransform3D() const;

        // Returns world position through the 3D transform @SCRIPTABLE
        Vec3F GetWorldPosition() const;

        // Sets world position through the 3D transform @SCRIPTABLE
        void SetWorldPosition(const Vec3F& position);

        // Returns world rotation quaternion through the 3D transform @SCRIPTABLE
        Quat GetWorldRotation() const;

        // Sets world rotation quaternion through the 3D transform @SCRIPTABLE
        void SetWorldRotation(const Quat& rotation);

        // Returns true when any 3D field differs from default @SCRIPTABLE
        bool Is3D() const;

        // Returns local 3D basis with size @SCRIPTABLE
        Basis3D GetBasis3D() const;

        // Returns local 3D basis without size @SCRIPTABLE
        Basis3D GetNonSizedBasis3D() const;

        // Returns world 3D basis with size @SCRIPTABLE
        Basis3D GetWorldBasis3D() const;

        // Returns world 3D basis without size @SCRIPTABLE
        Basis3D GetWorldNonSizedBasis3D() const;

        // Returns rect as 3D box @SCRIPTABLE
        o2::AABB GetRect3D() const;

        // Returns world rect as 3D box @SCRIPTABLE
        o2::AABB GetWorldRect3D() const;

        // Returns world axis aligned bounds of the sized transform @SCRIPTABLE
        o2::AABB GetWorldAABB() const;

        // Sets basis @SCRIPTABLE
        virtual void SetBasis(const Basis& basis);

        // Returns basis @SCRIPTABLE
        Basis GetBasis() const;

        // Sets basis without size @SCRIPTABLE
        virtual void SetNonSizedBasis(const Basis& basis);

        // Returns basis without size @SCRIPTABLE
        Basis GetNonSizedBasis() const;

        // Sets rect @SCRIPTABLE
        virtual void SetRect(const RectF& rect);

        // Returns rect @SCRIPTABLE
        virtual RectF GetRect() const;

        // Sets direction aligned rectangle transformation @SCRIPTABLE
        virtual void SetAxisAlignedRect(const RectF& rect);

        // Returns direction aligned rectangle transformation @SCRIPTABLE
        RectF GetAxisAlignedRect() const;

        // Sets left top corner position @SCRIPTABLE
        void SetLeftTop(const Vec2F& position);

        // Returns left top corner position @SCRIPTABLE
        Vec2F GetLeftTop() const;

        // Sets right top corner position @SCRIPTABLE
        void SetRightTop(const Vec2F& position);

        // Returns right top corner position @SCRIPTABLE
        Vec2F GetRightTop() const;

        // Sets left down corner position
        void SetLeftBottom(const Vec2F& position);

        // Returns left down corner position @SCRIPTABLE
        Vec2F GetLeftBottom() const;

        // Sets left right bottom position @SCRIPTABLE
        void SetRightBottom(const Vec2F& position);

        // Returns right bottom corner position @SCRIPTABLE
        Vec2F GetRightBottom() const;

        // Sets center position @SCRIPTABLE
        void SetCenter(const Vec2F& position);

        // Returns center position @SCRIPTABLE
        Vec2F GetCenter() const;

        // Set local right direction @SCRIPTABLE
        void SetRightDir(const Vec2F& dir);

        // Returns local right direction @SCRIPTABLE
        Vec2F GetRightDir() const;

        // Set local left direction @SCRIPTABLE
        void SetLeftDir(const Vec2F& dir);

        // Returns local left direction @SCRIPTABLE
        Vec2F GetLeftDir() const;

        // Set local up direction @SCRIPTABLE
        void SetUpDir(const Vec2F& dir);

        // Returns local up direction @SCRIPTABLE
        Vec2F GetUpDir() const;

        // Set local down direction @SCRIPTABLE
        void SetDownDir(const Vec2F& dir);

        // Returns local down direction @SCRIPTABLE
        Vec2F GetDownDir() const;

        // Set local right border position @SCRIPTABLE
        void SetRight(float value);

        // Returns local right border position @SCRIPTABLE
        float GetRight() const;

        // Set local left border position @SCRIPTABLE
        void SetLeft(float value);

        // Returns local left border position @SCRIPTABLE
        float GetLeft() const;

        // Set local top border position @SCRIPTABLE
        void SetTop(float value);

        // Returns local top border  @SCRIPTABLE
        float GetTop() const;

        // Set local down border position @SCRIPTABLE
        void SetBottom(float value);

        // Returns local down border position @SCRIPTABLE
        float GetBottom() const;

        // Transforms point from world space into local @SCRIPTABLE
        Vec2F World2LocalPoint(const Vec2F& worldPoint) const;

        // Transforms point from local space into world @SCRIPTABLE
        Vec2F Local2WorldPoint(const Vec2F& localPoint) const;

        // Transforms direction from world space into local @SCRIPTABLE
        Vec2F World2LocalDir(const Vec2F& worldDir) const;

        // Transforms direction from local space into world @SCRIPTABLE
        Vec2F Local2WorldDir(const Vec2F& localDir) const;

        // Returns true when point inside this @SCRIPTABLE
        bool IsPointInside(const Vec2F& point) const;

        // Sets 2D world position, in parent 2D basis @SCRIPTABLE
        void SetWorldPosition2D(const Vec2F& position);

        // Returns 2D world position, in parent 2D basis @SCRIPTABLE
        Vec2F GetWorldPosition2D() const;

        // Sets pivot by world coordinates @SCRIPTABLE
        void SetWorldPivot(const Vec2F& pivot);

        // Returns pivot position in world coordinates @SCRIPTABLE
        Vec2F GetWorldPivot() const;

        // Sets world rotation angle, in radians @SCRIPTABLE
        void SetWorldAngle(float rad);

        // Returns world rotation angle in radians @SCRIPTABLE
        float GetWorldAngle() const;

        // Sets world rotation angle, in degrees @SCRIPTABLE
        void SetWorldAngleDegree(float deg);

        // Returns world rotation angle in degrees @SCRIPTABLE
        float GetWorldAngleDegree() const;

        // Sets world basis @SCRIPTABLE
        void SetWorldBasis(const Basis& basis);

        // Returns world basis @SCRIPTABLE
        Basis GetWorldBasis() const;

        // Sets world basis without size @SCRIPTABLE
        void SetWorldNonSizedBasis(const Basis& basis);

        // Returns world basis without size @SCRIPTABLE
        Basis GetWorldNonSizedBasis() const;

        // Sets world rect @SCRIPTABLE
        void SetWorldRect(const RectF& rect);

        // Returns world rect @SCRIPTABLE
        RectF GetWorldRect() const;

        // Sets world direction aligned rectangle transformation @SCRIPTABLE
        void SetWorldAxisAlignedRect(const RectF& rect);

        // Returns world direction aligned rectangle transformation @SCRIPTABLE
        RectF GetWorldAxisAlignedRect() const;

        // Sets world left top corner position @SCRIPTABLE
        void SetWorldLeftTop(const Vec2F& position);

        // Returns world left top corner position @SCRIPTABLE
        Vec2F GetWorldLeftTop() const;

        // Sets world right top corner position @SCRIPTABLE
        void SetWorldRightTop(const Vec2F& position);

        // Returns world right top corner position @SCRIPTABLE
        Vec2F GetWorldRightTop() const;

        // Sets world left down corner position @SCRIPTABLE
        void SetWorldLeftBottom(const Vec2F& position);

        // Returns world left down corner position @SCRIPTABLE
        Vec2F GetWorldLeftBottom() const;

        // Sets world left right bottom position @SCRIPTABLE
        void SetWorldRightBottom(const Vec2F& position);

        // Returns world right bottom corner position @SCRIPTABLE
        Vec2F GetWorldRightBottom() const;

        // Sets world center position @SCRIPTABLE
        void SetWorldCenter(const Vec2F& position);

        // Returns world center position @SCRIPTABLE
        Vec2F GetWorldCenter() const;

        // Set World right direction @SCRIPTABLE
        void SetWorldRightDir(const Vec2F& dir);

        // Returns World right direction @SCRIPTABLE
        Vec2F GetWorldRightDir() const;

        // Set World left direction @SCRIPTABLE
        void SetWorldLeftDir(const Vec2F& dir);

        // Returns World left direction @SCRIPTABLE
        Vec2F GetWorldLeftDir() const;

        // Set World up direction @SCRIPTABLE
        void SetWorldUpDir(const Vec2F& dir);

        // Returns World up direction @SCRIPTABLE
        Vec2F GetWorldUpDir() const;

        // Set World down direction @SCRIPTABLE
        void SetWorldDownDir(const Vec2F& dir);

        // Returns World down direction @SCRIPTABLE
        Vec2F GetWorldDownDir() const;

        // Set World right border position @SCRIPTABLE
        void SetWorldRight(float value);

        // Returns World right border position @SCRIPTABLE
        float GetWorldRight() const;

        // Set World left border position @SCRIPTABLE
        void SetWorldLeft(float value);

        // Returns World left border position @SCRIPTABLE
        float GetWorldLeft() const;

        // Set World top border position @SCRIPTABLE
        void SetWorldTop(float value);

        // Returns World top border position @SCRIPTABLE
        float GetWorldTop() const;

        // Set World down border position @SCRIPTABLE
        void SetWorldBottom(float value);

        // Returns World down border position @SCRIPTABLE
        float GetWorldBottom() const;

        SERIALIZABLE(ActorTransform);

    protected:
        int mDirtyFrame = 1;  // Frame index, when layout was marked as dirty
        int mUpdateFrame = 1; // Frame index, when layout was updated

        Vec3F mPosition;               // Position, 2D API works with xy @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mSize;                   // Size, 2D content has zero z @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mScale = Vec3F(1, 1, 1); // Scale, (1, 1, 1) is default @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mPivot;                  // Pivot: (0, 0, 0) is left bottom near corner - (1, 1, 1) is right top far corner @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mEulerAngles;            // Rotation euler angles in radians, z is the 2D angle @DELTA_SEARCH_IF(IsSerializeEnabled)
        Vec3F mShear;                  // Shear planes: x - XY, y - XZ, z - YZ; 2D API works with x @DELTA_SEARCH_IF(IsSerializeEnabled)

        o2::AABB mLocalBox;          // Layout box in local space
        o2::AABB mParentBox;         // Parent layout box
        Vec3F    mParentBoxPosition; // Parent layout box pivot position
        o2::AABB mWorldBox;          // Layout box in world space

        Basis3D mTransform;         // Final transform basis
        Basis3D mNonSizedTransform; // Final transform basis without size

        Basis3D mWorldNonSizedTransform; // World transform without size
        Basis3D mWorldTransform;         // Result world basis

        Basis3D mParentInvertedTransform; // Parent world transform inverted
        Basis3D mParentTransform;         // Parent world transform

        int mParentInvertedTransformActualFrame; // last mParentInvertedTransform actual frame index

        WeakRef<Actor> mOwner; // Owner actor

    protected:
        // Returns is serialize enabled; used to turn off fields serialization
        virtual bool IsSerializeEnabled() const;

        // Copies data parameters from other transform
        virtual void CopyFrom(const ActorTransform& other);

        // Sets owner and updates transform
        virtual void SetOwner(const Ref<Actor>& actor);

        // Returns parent rectangle, or zero when no parent
        virtual RectF GetParentRectangle() const;

        // Updates world box and transform relative to parent or origin
        void UpdateWorldBoxAndTransform();

        // Updates local transformation
        void UpdateTransform();

        // Updates local layout box
        void UpdateLocalBox();

        // Check parentInvertedTransform for actual
        void CheckParentInvTransform();

        // Beginning serialization callback, writes data
        void OnSerialize(DataValue& node) const override;

        // Called when object was deserialized, reads data
        void OnDeserialized(const DataValue& node) override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        // Returns parent world rect position - left bottom corner
        Vec2F GetParentPosition() const;

        friend class Actor;
        friend class WidgetLayout;
    };
}
// --- META ---

CLASS_BASES_META(o2::ActorTransform)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::ActorTransform)
{
    FIELD().PUBLIC().NAME(actor);
    FIELD().PUBLIC().NAME(position);
    FIELD().PUBLIC().NAME(position2D);
    FIELD().PUBLIC().NAME(positionX);
    FIELD().PUBLIC().NAME(positionY);
    FIELD().PUBLIC().NAME(positionZ);
    FIELD().PUBLIC().NAME(size);
    FIELD().PUBLIC().NAME(size2D);
    FIELD().PUBLIC().NAME(width);
    FIELD().PUBLIC().NAME(height);
    FIELD().PUBLIC().NAME(sizeZ);
    FIELD().PUBLIC().NAME(scale);
    FIELD().PUBLIC().NAME(scale2D);
    FIELD().PUBLIC().NAME(scaleX);
    FIELD().PUBLIC().NAME(scaleY);
    FIELD().PUBLIC().NAME(scaleZ);
    FIELD().PUBLIC().NAME(pivot);
    FIELD().PUBLIC().NAME(pivot2D);
    FIELD().PUBLIC().NAME(szPivot);
    FIELD().PUBLIC().NAME(pivotX);
    FIELD().PUBLIC().NAME(pivotY);
    FIELD().PUBLIC().NAME(pivotZ);
    FIELD().PUBLIC().NAME(angle);
    FIELD().PUBLIC().NAME(angleDegrees);
    FIELD().PUBLIC().NAME(shear);
    FIELD().PUBLIC().NAME(shear2D);
    FIELD().PUBLIC().NAME(eulerAngles);
    FIELD().PUBLIC().NAME(eulerAnglesDegrees);
    FIELD().PUBLIC().NAME(rotation);
    FIELD().PUBLIC().NAME(basis);
    FIELD().PUBLIC().NAME(nonSizedBasis);
    FIELD().PUBLIC().NAME(AABB);
    FIELD().PUBLIC().NAME(rect);
    FIELD().PUBLIC().NAME(leftTop);
    FIELD().PUBLIC().NAME(leftBottom);
    FIELD().PUBLIC().NAME(rightTop);
    FIELD().PUBLIC().NAME(rightBottom);
    FIELD().PUBLIC().NAME(center);
    FIELD().PUBLIC().NAME(downDir);
    FIELD().PUBLIC().NAME(right);
    FIELD().PUBLIC().NAME(left);
    FIELD().PUBLIC().NAME(top);
    FIELD().PUBLIC().NAME(bottom);
    FIELD().PUBLIC().NAME(worldPosition);
    FIELD().PUBLIC().NAME(worldPosition2D);
    FIELD().PUBLIC().NAME(worldPivot);
    FIELD().PUBLIC().NAME(worldAngle);
    FIELD().PUBLIC().NAME(worldAngleDegree);
    FIELD().PUBLIC().NAME(worldBasis);
    FIELD().PUBLIC().NAME(worldNonSizedBasis);
    FIELD().PUBLIC().NAME(worldLeftTop);
    FIELD().PUBLIC().NAME(worldLeftBottom);
    FIELD().PUBLIC().NAME(worldRightTop);
    FIELD().PUBLIC().NAME(worldRightBottom);
    FIELD().PUBLIC().NAME(worldCenter);
    FIELD().PUBLIC().NAME(worldUpDir);
    FIELD().PUBLIC().NAME(worldRight);
    FIELD().PUBLIC().NAME(worldLeft);
    FIELD().PUBLIC().NAME(worldTop);
    FIELD().PUBLIC().NAME(worldBottom);
    FIELD().PUBLIC().NAME(worldRect);
    FIELD().PUBLIC().NAME(worldAABB);
    FIELD().PROTECTED().DEFAULT_VALUE(1).NAME(mDirtyFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(1).NAME(mUpdateFrame);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mPosition);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mSize);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).DEFAULT_VALUE(Vec3F(1, 1, 1)).NAME(mScale);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mPivot);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mEulerAngles);
    FIELD().PROTECTED().DELTA_SEARCH_IF_ATTRIBUTE(IsSerializeEnabled).NAME(mShear);
    FIELD().PROTECTED().NAME(mLocalBox);
    FIELD().PROTECTED().NAME(mParentBox);
    FIELD().PROTECTED().NAME(mParentBoxPosition);
    FIELD().PROTECTED().NAME(mWorldBox);
    FIELD().PROTECTED().NAME(mTransform);
    FIELD().PROTECTED().NAME(mNonSizedTransform);
    FIELD().PROTECTED().NAME(mWorldNonSizedTransform);
    FIELD().PROTECTED().NAME(mWorldTransform);
    FIELD().PROTECTED().NAME(mParentInvertedTransform);
    FIELD().PROTECTED().NAME(mParentTransform);
    FIELD().PROTECTED().NAME(mParentInvertedTransformActualFrame);
    FIELD().PROTECTED().NAME(mOwner);
}
END_META;
CLASS_METHODS_META(o2::ActorTransform)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const ActorTransform&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Actor>, GetOwnerActor);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetDirty, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsDirty);
    FUNCTION().PUBLIC().SIGNATURE(void, Update);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPosition, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetPosition);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPosition2D, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetPosition2D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPositionX, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPositionX);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPositionY, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPositionY);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSize, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSize2D, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetSize2D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWidth, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetHeight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPivot, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetPivot);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPivot2D, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetPivot2D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSizePivot, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetSizePivot);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPivotX, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPivotX);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPivotY, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPivotY);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetScale, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetScale);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetScale2D, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetScale2D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetScaleX, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetScaleY, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetScaleX);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetScaleY);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAngle, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetAngle);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAngleDegrees, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetAngleDegrees);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetShear, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetShear);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetShear2D, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetShear2D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPositionZ, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPositionZ);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetScaleZ, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetScaleZ);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSizeZ, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetSizeZ);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPivotZ, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPivotZ);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEulerAngles, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetEulerAngles);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEulerAnglesDegrees, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetEulerAnglesDegrees);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetRotation, const Quat&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Quat, GetRotation);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Mat4, GetLocalTransform3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Mat4, GetWorldTransform3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec3F, GetWorldPosition);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldPosition, const Vec3F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Quat, GetWorldRotation);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldRotation, const Quat&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, Is3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis3D, GetBasis3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis3D, GetNonSizedBasis3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis3D, GetWorldBasis3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis3D, GetWorldNonSizedBasis3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(o2::AABB, GetRect3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(o2::AABB, GetWorldRect3D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(o2::AABB, GetWorldAABB);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetBasis, const Basis&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis, GetBasis);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetNonSizedBasis, const Basis&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis, GetNonSizedBasis);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetRect, const RectF&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectF, GetRect);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAxisAlignedRect, const RectF&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectF, GetAxisAlignedRect);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetLeftTop, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetLeftTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetRightTop, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetRightTop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeftBottom, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetLeftBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetRightBottom, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetRightBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetCenter, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetCenter);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetRightDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetRightDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetLeftDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetLeftDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetUpDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetUpDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetDownDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetDownDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetRight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetRight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetLeft, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetLeft);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetTop, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetBottom, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, World2LocalPoint, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, Local2WorldPoint, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, World2LocalDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, Local2WorldDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsPointInside, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldPosition2D, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldPosition2D);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldPivot, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldPivot);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldAngle, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWorldAngle);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldAngleDegree, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWorldAngleDegree);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldBasis, const Basis&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis, GetWorldBasis);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldNonSizedBasis, const Basis&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Basis, GetWorldNonSizedBasis);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldRect, const RectF&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectF, GetWorldRect);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldAxisAlignedRect, const RectF&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectF, GetWorldAxisAlignedRect);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldLeftTop, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldLeftTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldRightTop, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldRightTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldLeftBottom, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldLeftBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldRightBottom, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldRightBottom);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldCenter, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldCenter);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldRightDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldRightDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldLeftDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldLeftDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldUpDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldUpDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldDownDir, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetWorldDownDir);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldRight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWorldRight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldLeft, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWorldLeft);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldTop, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWorldTop);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWorldBottom, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWorldBottom);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsSerializeEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, CopyFrom, const ActorTransform&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetOwner, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetParentRectangle);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateWorldBoxAndTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLocalBox);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckParentInvTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, GetParentPosition);
}
END_META;
// --- END META ---
