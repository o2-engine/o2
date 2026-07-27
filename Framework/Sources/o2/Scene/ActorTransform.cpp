#include "o2/stdafx.h"
#include "ActorTransform.h"

#include "o2/Application/Input.h"
#include "o2/Scene/Actor.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    namespace
    {
        // Reads Vec3F member in both formats: new {x, y, z} and legacy {x, y}; missing components stay untouched
        void ReadVec3FCompat(const DataValue& node, const char* name, Vec3F& value)
        {
            auto member = node.FindMember(name);
            if (!member || !member->IsObject())
                return;

            if (auto x = member->FindMember("x"))
                x->Get(value.x);

            if (auto y = member->FindMember("y"))
                y->Get(value.y);

            if (auto z = member->FindMember("z"))
                z->Get(value.z);
        }

        // Reads transform fields in the current format and both legacy ones: pre-3D (angle and shear
        // as numbers) and phase-2 (positionZ, angleXY, scaleZ members)
        void ReadTransformFieldsCompat(const DataValue& node, Vec3F& position, Vec3F& size, Vec3F& scale,
                                       Vec3F& pivot, Vec3F& eulerAngles, Vec3F& shear)
        {
            ReadVec3FCompat(node, "position", position);
            ReadVec3FCompat(node, "size", size);
            ReadVec3FCompat(node, "scale", scale);
            ReadVec3FCompat(node, "pivot", pivot);
            ReadVec3FCompat(node, "eulerAngles", eulerAngles);
            ReadVec3FCompat(node, "angleXY", eulerAngles);

            if (auto member = node.FindMember("angle"); member && member->IsNumber())
                member->Get(eulerAngles.z);

            if (auto member = node.FindMember("shear"))
            {
                if (member->IsObject())
                    ReadVec3FCompat(node, "shear", shear);
                else if (member->IsNumber())
                    member->Get(shear.x);
            }

            if (auto member = node.FindMember("positionZ"); member && member->IsNumber())
                member->Get(position.z);

            if (auto member = node.FindMember("scaleZ"); member && member->IsNumber())
                member->Get(scale.z);
        }
    }

    ActorTransform::ActorTransform(const Vec2F& size /*= Vec2F()*/, const Vec2F& position /*= Vec2F()*/,
                                   float angle /*= 0.0f*/, const Vec2F& scale /*= Vec2F(1.0f, 1.0f)*/,
                                   const Vec2F& pivot /*= Vec2F(0.5f, 0.5f)*/)
    {
        mDirtyFrame = 1;
        mUpdateFrame = 1;

        mSize = Vec3F(size);
        mPosition = Vec3F(position);
        mEulerAngles = Vec3F(0, 0, angle);
        mScale = Vec3F(scale, 1.0f);
        mPivot = Vec3F(pivot);
    }

    ActorTransform::ActorTransform(const ActorTransform& other)
    {
        mDirtyFrame = 1;
        mUpdateFrame = 1;

        mSize = other.mSize;
        mPosition = other.mPosition;
        mScale = other.mScale;
        mPivot = other.mPivot;
        mEulerAngles = other.mEulerAngles;
        mShear = other.mShear;
    }

    ActorTransform::~ActorTransform()
    {}

    void ActorTransform::CopyFrom(const ActorTransform& other)
    {
        mSize = other.mSize;
        mPosition = other.mPosition;
        mScale = other.mScale;
        mPivot = other.mPivot;
        mEulerAngles = other.mEulerAngles;
        mShear = other.mShear;

        SetDirty();
    }

    ActorTransform& ActorTransform::operator=(const ActorTransform& other)
    {
        CopyFrom(other);
        return *this;
    }

    bool ActorTransform::operator==(const ActorTransform& other) const
    {
        return Math::Equals(mSize, other.mSize) &&
            Math::Equals(mPosition, other.mPosition) &&
            Math::Equals(mScale, other.mScale) &&
            Math::Equals(mPivot, other.mPivot) &&
            Math::Equals(mEulerAngles, other.mEulerAngles) &&
            Math::Equals(mShear, other.mShear);
    }

    void ActorTransform::SetPosition(const Vec3F& position)
    {
        mPosition = position;
        SetDirty();
    }

    Vec3F ActorTransform::GetPosition() const
    {
        return mPosition;
    }

    void ActorTransform::SetPosition2D(const Vec2F& position)
    {
        mPosition.x = position.x;
        mPosition.y = position.y;
        SetDirty();
    }

    Vec2F ActorTransform::GetPosition2D() const
    {
        return mPosition.XY();
    }

    void ActorTransform::SetPositionX(float value)
    {
        SetPosition2D(Vec2F(value, mPosition.y));
    }

    float ActorTransform::GetPositionX() const
    {
        return mPosition.x;
    }

    void ActorTransform::SetPositionY(float value)
    {
        SetPosition2D(Vec2F(mPosition.x, value));
    }

    float ActorTransform::GetPositionY() const
    {
        return mPosition.y;
    }

    void ActorTransform::SetSize(const Vec3F& size)
    {
        mSize.z = size.z;
        SetSize2D(size.XY());
    }

    Vec3F ActorTransform::GetSize() const
    {
        return mSize;
    }

    void ActorTransform::SetSize2D(const Vec2F& size)
    {
        mSize.x = size.x;
        mSize.y = size.y;
        SetDirty();
    }

    Vec2F ActorTransform::GetSize2D() const
    {
        return mSize.XY();
    }

    void ActorTransform::SetWidth(float value)
    {
        mSize.x = value;
        SetDirty();
    }

    float ActorTransform::GetWidth() const
    {
        return mSize.x;
    }

    void ActorTransform::SetHeight(float value)
    {
        mSize.y = value;
        SetDirty();
    }

    float ActorTransform::GetHeight() const
    {
        return mSize.y;
    }

    void ActorTransform::SetPivot(const Vec3F& pivot)
    {
        mPivot.z = pivot.z;
        SetPivot2D(pivot.XY());
    }

    Vec3F ActorTransform::GetPivot() const
    {
        return mPivot;
    }

    void ActorTransform::SetPivot2D(const Vec2F& pivot)
    {
        mPivot.x = pivot.x;
        mPivot.y = pivot.y;
        SetDirty();
    }

    Vec2F ActorTransform::GetPivot2D() const
    {
        return mPivot.XY();
    }

    void ActorTransform::SetSizePivot(const Vec2F& relPivot)
    {
        SetPivot2D(relPivot / mSize.XY());
    }

    Vec2F ActorTransform::GetSizePivot() const
    {
        return mPivot.XY()*mSize.XY();
    }

    void ActorTransform::SetPivotX(float value)
    {
        SetPivot2D(Vec2F(value, mPivot.y));
    }

    float ActorTransform::GetPivotX() const
    {
        return mPivot.x;
    }

    void ActorTransform::SetPivotY(float value)
    {
        SetPivot2D(Vec2F(mPivot.x, value));
    }

    float ActorTransform::GetPivotY() const
    {
        return mPivot.y;
    }

    void ActorTransform::SetRect(const RectF& rect)
    {
        mSize.x = rect.Width();
        mSize.y = rect.Height();

        Vec2F position = rect.LeftBottom() + mSize.XY()*mPivot.XY();
        mPosition.x = position.x;
        mPosition.y = position.y;

        SetDirty();
    }

    RectF ActorTransform::GetRect() const
    {
        Vec2F leftBottom = mPosition.XY() - mSize.XY()*mPivot.XY();
        return RectF(leftBottom, leftBottom + mSize.XY());
    }

    void ActorTransform::SetScale(const Vec3F& scale)
    {
        mScale = scale;
        SetDirty();
    }

    Vec3F ActorTransform::GetScale() const
    {
        return mScale;
    }

    void ActorTransform::SetScale2D(const Vec2F& scale)
    {
        mScale.x = scale.x;
        mScale.y = scale.y;
        SetDirty();
    }

    Vec2F ActorTransform::GetScale2D() const
    {
        return mScale.XY();
    }

    void ActorTransform::SetScaleX(float scaleX)
    {
        mScale.x = scaleX;
        SetDirty();
    }

    void ActorTransform::SetScaleY(float scaleY)
    {
        mScale.y = scaleY;
        SetDirty();
    }

    float ActorTransform::GetScaleX() const
    {
        return mScale.x;
    }

    float ActorTransform::GetScaleY() const
    {
        return mScale.y;
    }

    void ActorTransform::SetAngle(float rad)
    {
        mEulerAngles.z = rad;
        SetDirty();
    }

    float ActorTransform::GetAngle() const
    {
        return mEulerAngles.z;
    }

    void ActorTransform::SetAngleDegrees(float deg)
    {
        mEulerAngles.z = Math::Deg2rad(deg);
        SetDirty();
    }

    float ActorTransform::GetAngleDegrees() const
    {
        return Math::Rad2deg(mEulerAngles.z);
    }

    void ActorTransform::SetShear(const Vec3F& shear)
    {
        mShear = shear;
        SetDirty();
    }

    Vec3F ActorTransform::GetShear() const
    {
        return mShear;
    }

    void ActorTransform::SetShear2D(float shear)
    {
        mShear.x = shear;
        SetDirty();
    }

    float ActorTransform::GetShear2D() const
    {
        return mShear.x;
    }

    void ActorTransform::SetPositionZ(float value)
    {
        mPosition.z = value;
        SetDirty();
    }

    float ActorTransform::GetPositionZ() const
    {
        return mPosition.z;
    }

    void ActorTransform::SetScaleZ(float value)
    {
        mScale.z = value;
        SetDirty();
    }

    float ActorTransform::GetScaleZ() const
    {
        return mScale.z;
    }

    void ActorTransform::SetSizeZ(float value)
    {
        mSize.z = value;
        SetDirty();
    }

    float ActorTransform::GetSizeZ() const
    {
        return mSize.z;
    }

    void ActorTransform::SetPivotZ(float value)
    {
        mPivot.z = value;
        SetDirty();
    }

    float ActorTransform::GetPivotZ() const
    {
        return mPivot.z;
    }

    void ActorTransform::SetEulerAngles(const Vec3F& radians)
    {
        mEulerAngles = radians;
        SetDirty();
    }

    Vec3F ActorTransform::GetEulerAngles() const
    {
        return mEulerAngles;
    }

    void ActorTransform::SetEulerAnglesDegrees(const Vec3F& degrees)
    {
        SetEulerAngles(degrees*Math::Deg2rad(1.0f));
    }

    Vec3F ActorTransform::GetEulerAnglesDegrees() const
    {
        return GetEulerAngles()*Math::Rad2deg(1.0f);
    }

    void ActorTransform::SetRotation(const Quat& rotation)
    {
        SetEulerAngles(rotation.ToEuler());
    }

    Quat ActorTransform::GetRotation() const
    {
        return Quat::FromEuler(GetEulerAngles());
    }

    Mat4 ActorTransform::GetLocalTransform3D() const
    {
        return Basis3D::Build(mPosition, mScale, mEulerAngles, mShear).ToMat4();
    }

    Mat4 ActorTransform::GetWorldTransform3D() const
    {
        Mat4 local = GetLocalTransform3D();

        auto owner = mOwner.Lock();
        if (owner && owner->mParent)
            return owner->mParent.Lock()->transform->GetWorldTransform3D()*local;

        return local;
    }

    Vec3F ActorTransform::GetWorldPosition() const
    {
        return GetWorldTransform3D().TransformPoint(Vec3F());
    }

    void ActorTransform::SetWorldPosition(const Vec3F& position)
    {
        auto owner = mOwner.Lock();
        if (owner && owner->mParent)
            SetPosition(owner->mParent.Lock()->transform->GetWorldTransform3D().Inverted().TransformPoint(position));
        else
            SetPosition(position);
    }

    Quat ActorTransform::GetWorldRotation() const
    {
        auto owner = mOwner.Lock();
        if (owner && owner->mParent)
            return owner->mParent.Lock()->transform->GetWorldRotation()*GetRotation();

        return GetRotation();
    }

    void ActorTransform::SetWorldRotation(const Quat& rotation)
    {
        auto owner = mOwner.Lock();
        if (owner && owner->mParent)
            SetRotation(owner->mParent.Lock()->transform->GetWorldRotation().Inverted()*rotation);
        else
            SetRotation(rotation);
    }

    bool ActorTransform::Is3D() const
    {
        return !Math::Equals(mPosition.z, 0.0f) ||
            !Math::Equals(mEulerAngles.x, 0.0f) ||
            !Math::Equals(mEulerAngles.y, 0.0f) ||
            !Math::Equals(mScale.z, 1.0f) ||
            !Math::Equals(mSize.z, 0.0f) ||
            !Math::Equals(mPivot.z, 0.0f) ||
            !Math::Equals(mShear.y, 0.0f) ||
            !Math::Equals(mShear.z, 0.0f);
    }

    Basis3D ActorTransform::GetBasis3D() const
    {
        return mTransform;
    }

    Basis3D ActorTransform::GetNonSizedBasis3D() const
    {
        return mNonSizedTransform;
    }

    Basis3D ActorTransform::GetWorldBasis3D() const
    {
        if (IsDirty())
            const_cast<ActorTransform*>(this)->Update();

        return mWorldTransform;
    }

    Basis3D ActorTransform::GetWorldNonSizedBasis3D() const
    {
        if (IsDirty())
            const_cast<ActorTransform*>(this)->Update();

        return mWorldNonSizedTransform;
    }

    AABB ActorTransform::GetRect3D() const
    {
        Vec3F leftBottom = mPosition - mSize*mPivot;
        return o2::AABB(leftBottom, leftBottom + mSize);
    }

    AABB ActorTransform::GetWorldRect3D() const
    {
        if (IsDirty())
            const_cast<ActorTransform*>(this)->Update();

        return mWorldBox;
    }

    AABB ActorTransform::GetWorldAABB() const
    {
        if (IsDirty())
            const_cast<ActorTransform*>(this)->Update();

        return mWorldTransform.AABB();
    }

    namespace
    {
        // Projected 2D lengths of the rotated unit x and sheared unit y axes; the flat 2D
        // decomposition treats their foreshortening as size change and their skew as shear
        void Get3DProjectedAxisLengths(const Vec3F& eulerAngles, float shearXY, float& xLength, float& yLength)
        {
            Quat rotation = Quat::FromEuler(eulerAngles);
            xLength = (rotation*Vec3F(1.0f, 0.0f, 0.0f)).XY().Length();

            float shearYY = Math::Sqrt(Math::Max(0.0f, 1.0f - shearXY*shearXY));
            yLength = (rotation*Vec3F(shearXY, shearYY, 0.0f)).XY().Length();
        }
    }

    void ActorTransform::SetBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        // Zero-length x axis (zero size or scale) decodes into garbage angle and shear,
        // they must not stomp the current rotation
        bool xAxisValid = basis.xv.SqrLength() > FLT_EPSILON;

        if (Math::Equals(mEulerAngles.x, 0.0f) && Math::Equals(mEulerAngles.y, 0.0f))
        {
            if (xAxisValid)
            {
                mEulerAngles.z = angle;
                mShear.x = shear;
            }

            Vec2F size = scale / mScale.XY();
            mSize.x = size.x;
            mSize.y = size.y;
        }
        else
        {
            // Compensate the x/y euler foreshortening baked into the projected basis; shear is kept
            float xLength, yLength;
            Get3DProjectedAxisLengths(mEulerAngles, mShear.x, xLength, yLength);

            // The decoded angle comes from the projected x axis: with a degenerate projection
            // (yaw near 90 degrees) it is garbage and must not stomp the current euler z
            if (xLength > 0.001f && xAxisValid)
                mEulerAngles.z = angle;

            if (xLength > 0.001f && Math::Abs(mScale.x) > FLT_EPSILON)
                mSize.x = scale.x/(mScale.x*xLength);

            if (yLength > 0.001f && Math::Abs(mScale.y) > FLT_EPSILON)
                mSize.y = scale.y/(mScale.y*yLength);
        }

        Vec2F position = basis.origin + basis.xv*mPivot.x + basis.yv*mPivot.y;
        mPosition.x = position.x;
        mPosition.y = position.y;

        SetDirty();
    }

    Basis ActorTransform::GetBasis() const
    {
        return mTransform.ToBasis();
    }

    void ActorTransform::SetNonSizedBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        // Zero-length x axis (zero scale) decodes into garbage angle and shear,
        // they must not stomp the current rotation
        bool xAxisValid = basis.xv.SqrLength() > FLT_EPSILON;

        if (Math::Equals(mEulerAngles.x, 0.0f) && Math::Equals(mEulerAngles.y, 0.0f))
        {
            if (xAxisValid)
            {
                mEulerAngles.z = angle;
                mShear.x = shear;
            }

            mScale.x = scale.x;
            mScale.y = scale.y;
        }
        else
        {
            float xLength, yLength;
            Get3DProjectedAxisLengths(mEulerAngles, mShear.x, xLength, yLength);

            // The decoded angle comes from the projected x axis: with a degenerate projection
            // (yaw near 90 degrees) it is garbage and must not stomp the current euler z
            if (xLength > 0.001f)
            {
                if (xAxisValid)
                    mEulerAngles.z = angle;

                mScale.x = scale.x/xLength;
            }

            if (yLength > 0.001f)
                mScale.y = scale.y/yLength;
        }

        mPosition.x = basis.origin.x;
        mPosition.y = basis.origin.y;

        SetDirty();
    }

    Basis ActorTransform::GetNonSizedBasis() const
    {
        return mNonSizedTransform.ToBasis();
    }

    void ActorTransform::SetAxisAlignedRect(const RectF& rect)
    {
        RectF curRect = GetAxisAlignedRect();
        Basis curRectBasis(curRect.LeftBottom(), Vec2F::Right()*curRect.Width(), Vec2F::Up()*curRect.Height());
        Basis rectBasis(rect.LeftBottom(), Vec2F::Right()*rect.Width(), Vec2F::Up()*rect.Height());

        SetBasis(mTransform.ToBasis()*curRectBasis.Inverted()*rectBasis);
    }

    RectF ActorTransform::GetAxisAlignedRect() const
    {
        return mTransform.ToBasis().AABB();
    }

    void ActorTransform::SetLeftTop(const Vec2F& position)
    {
        RectF rect = GetRect();
        SetRect(RectF(position.x, rect.bottom, rect.right, position.y));
    }

    Vec2F ActorTransform::GetLeftTop() const
    {
        return GetRect().LeftTop();
    }

    void ActorTransform::SetRightTop(const Vec2F& position)
    {
        RectF rect = GetRect();
        SetRect(RectF(position.x, rect.bottom, rect.right, position.y));
    }

    Vec2F ActorTransform::GetRightTop() const
    {
        return GetRect().RightTop();
    }

    void ActorTransform::SetLeftBottom(const Vec2F& position)
    {
        RectF rect = GetRect();
        SetRect(RectF(position.x, position.y, rect.right, rect.top));
    }

    Vec2F ActorTransform::GetLeftBottom() const
    {
        return GetRect().LeftBottom();
    }

    void ActorTransform::SetRightBottom(const Vec2F& position)
    {
        RectF rect = GetRect();
        SetRect(RectF(rect.left, position.y, position.x, rect.top));
    }

    Vec2F ActorTransform::GetRightBottom() const
    {
        return GetRect().RightBottom();
    }

    void ActorTransform::SetCenter(const Vec2F& position)
    {
        RectF rect = GetRect();
        SetRect(rect + (position - rect.Center()));
    }

    Vec2F ActorTransform::GetCenter() const
    {
        return GetRect().Center();
    }

    void ActorTransform::SetRightDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetRightDir().SignedAngle(dir));
        SetWorldBasis(mTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetRightDir() const
    {
        return mNonSizedTransform.xv.XY();
    }

    void ActorTransform::SetLeftDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetLeftDir().SignedAngle(dir));
        SetWorldBasis(mTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetLeftDir() const
    {
        return mNonSizedTransform.xv.XY().Inverted();
    }

    void ActorTransform::SetUpDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetUpDir().SignedAngle(dir));
        SetWorldBasis(mTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetUpDir() const
    {
        return mNonSizedTransform.yv.XY();
    }

    void ActorTransform::SetDownDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetDownDir().SignedAngle(dir));
        SetWorldBasis(mTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetDownDir() const
    {
        return mNonSizedTransform.yv.XY().Inverted();
    }

    void ActorTransform::SetRight(float value)
    {
        RectF rect = GetRect();
        SetRect(RectF(rect.left, rect.bottom, value, rect.top));
    }

    float ActorTransform::GetRight() const
    {
        return GetRect().right;
    }

    void ActorTransform::SetLeft(float value)
    {
        RectF rect = GetRect();
        SetRect(RectF(value, rect.bottom, rect.right, rect.top));
    }

    float ActorTransform::GetLeft() const
    {
        return GetRect().left;
    }

    void ActorTransform::SetTop(float value)
    {
        RectF rect = GetRect();
        SetRect(RectF(rect.left, rect.bottom, rect.right, value));
    }

    float ActorTransform::GetTop() const
    {
        return GetRect().top;
    }

    void ActorTransform::SetBottom(float value)
    {
        RectF rect = GetRect();
        SetRect(RectF(rect.left, value, rect.right, rect.top));
    }

    float ActorTransform::GetBottom() const
    {
        return GetRect().bottom;
    }

    Ref<Actor> ActorTransform::GetOwnerActor() const
    {
        return mOwner.Lock();
    }

    bool ActorTransform::IsDirty() const
    {
        return mUpdateFrame == 0;
    }

    void ActorTransform::SetWorldPivot(const Vec2F& pivot)
    {
        Basis trasform = mWorldTransform.ToBasis();
        SetSizePivot(World2LocalPoint(pivot));
        SetWorldBasis(trasform);
    }

    Vec2F ActorTransform::GetWorldPivot() const
    {
        return Local2WorldPoint(mPivot.XY()*mSize.XY());
    }

    void ActorTransform::SetWorldPosition2D(const Vec2F& position)
    {
        CheckParentInvTransform();
        SetPosition2D(position*mParentInvertedTransform.ToBasis());
    }

    Vec2F ActorTransform::GetWorldPosition2D() const
    {
        return mPosition.XY()*mParentTransform.ToBasis();
    }

    void ActorTransform::SetWorldRect(const RectF& rect)
    {
        CheckParentInvTransform();
        Basis parentInverted = mParentInvertedTransform.ToBasis();
        SetRect(RectF(rect.LeftBottom()*parentInverted, rect.RightTop()*parentInverted));
    }

    RectF ActorTransform::GetWorldRect() const
    {
        return mWorldBox.ToRect();
    }

    void ActorTransform::SetWorldAngle(float rad)
    {
        SetAngle(rad - mParentTransform.ToBasis().GetAngle());
    }

    float ActorTransform::GetWorldAngle() const
    {
        return mWorldTransform.ToBasis().GetAngle();
    }

    void ActorTransform::SetWorldAngleDegree(float deg)
    {
        SetWorldAngle(Math::Deg2rad(deg));
    }

    float ActorTransform::GetWorldAngleDegree() const
    {
        return Math::Rad2deg(GetWorldAngle());
    }

    void ActorTransform::SetWorldBasis(const Basis& basis)
    {
        CheckParentInvTransform();
        SetBasis(basis*mParentInvertedTransform.ToBasis());
    }

    Basis ActorTransform::GetWorldBasis() const
    {
        if (IsDirty())
            const_cast<ActorTransform*>(this)->Update();

        return mWorldTransform.ToBasis();
    }

    void ActorTransform::SetWorldNonSizedBasis(const Basis& basis)
    {
        CheckParentInvTransform();
        SetNonSizedBasis(basis*mParentInvertedTransform.ToBasis());
    }

    Basis ActorTransform::GetWorldNonSizedBasis() const
    {
        return mWorldNonSizedTransform.ToBasis();
    }

    void ActorTransform::SetWorldAxisAlignedRect(const RectF& rect)
    {
        CheckParentInvTransform();
        Basis parentInverted = mParentInvertedTransform.ToBasis();
        SetAxisAlignedRect(RectF(rect.LeftBottom()*parentInverted, rect.RightTop()*parentInverted));
    }

    RectF ActorTransform::GetWorldAxisAlignedRect() const
    {
        Basis parentTransform = mParentTransform.ToBasis();
        RectF localAARect = GetRect();
        RectF worldAARect(localAARect.LeftBottom()*parentTransform, localAARect.RightTop()*parentTransform);
        return worldAARect;
    }

    void ActorTransform::SetWorldLeftTop(const Vec2F& position)
    {
        SetLeftTop(position - GetParentPosition());
    }

    Vec2F ActorTransform::GetWorldLeftTop() const
    {
        return Vec2F(GetWorldLeft(), GetWorldTop());
    }

    void ActorTransform::SetWorldRightTop(const Vec2F& position)
    {
        SetRightTop(position - GetParentPosition());
    }

    Vec2F ActorTransform::GetWorldRightTop() const
    {
        return Vec2F(GetWorldRight(), GetWorldTop());
    }

    void ActorTransform::SetWorldLeftBottom(const Vec2F& position)
    {
        SetLeftTop(position - GetParentPosition());
    }

    Vec2F ActorTransform::GetWorldLeftBottom() const
    {
        return Vec2F(GetWorldLeft(), GetWorldBottom());
    }

    void ActorTransform::SetWorldRightBottom(const Vec2F& position)
    {
        SetRightBottom(position - GetParentPosition());
    }

    Vec2F ActorTransform::GetWorldRightBottom() const
    {
        return Vec2F(GetWorldRight(), GetWorldBottom());
    }

    void ActorTransform::SetWorldCenter(const Vec2F& position)
    {
        Vec2F translate = position - GetWorldCenter();
        SetWorldBasis(mWorldTransform.ToBasis()*Basis::Translated(translate));
    }

    Vec2F ActorTransform::GetWorldCenter() const
    {
        return (mWorldTransform.origin + (mWorldTransform.xv + mWorldTransform.yv)*0.5f).XY();
    }

    void ActorTransform::SetWorldRightDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetRightDir().SignedAngle(dir));
        SetWorldBasis(mWorldTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetWorldRightDir() const
    {
        return mWorldNonSizedTransform.xv.XY();
    }

    void ActorTransform::SetWorldLeftDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetLeftDir().SignedAngle(dir));
        SetWorldBasis(mWorldTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetWorldLeftDir() const
    {
        return mWorldNonSizedTransform.xv.XY().Inverted();
    }

    void ActorTransform::SetWorldUpDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetUpDir().SignedAngle(dir));
        SetWorldBasis(mWorldTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetWorldUpDir() const
    {
        return mWorldNonSizedTransform.yv.XY();
    }

    void ActorTransform::SetWorldDownDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetDownDir().SignedAngle(dir));
        SetWorldBasis(mWorldTransform.ToBasis()*transf);
    }

    Vec2F ActorTransform::GetWorldDownDir() const
    {
        return mWorldNonSizedTransform.yv.XY().Inverted();
    }

    void ActorTransform::SetWorldRight(float value)
    {
        SetRight(value - GetParentPosition().x);
    }

    float ActorTransform::GetWorldRight() const
    {
        return GetWorldRect().right;
    }

    void ActorTransform::SetWorldLeft(float value)
    {
        SetLeft(value - GetParentPosition().x);
    }

    float ActorTransform::GetWorldLeft() const
    {
        return GetWorldRect().left;
    }

    void ActorTransform::SetWorldTop(float value)
    {
        SetTop(value - GetParentPosition().y);
    }

    float ActorTransform::GetWorldTop() const
    {
        return GetWorldRect().top;
    }

    void ActorTransform::SetWorldBottom(float value)
    {
        SetBottom(value - GetParentPosition().y);
    }

    float ActorTransform::GetWorldBottom() const
    {
        return GetWorldRect().bottom;
    }

    Vec2F ActorTransform::World2LocalPoint(const Vec2F& worldPoint) const
    {
        Vec2F nx = mWorldTransform.xv.XY(), ny = mWorldTransform.yv.XY(), offs = mWorldTransform.origin.XY(), w = worldPoint;
        float lx = (w.x*ny.y - offs.x*ny.y - w.y*ny.x + offs.y*ny.x) / (nx.x*ny.y - ny.x*nx.y);
        float ly = (w.y - offs.y - nx.y*lx) / ny.y;
        return Vec2F(lx, ly)*mSize.XY();
    }

    Vec2F ActorTransform::Local2WorldPoint(const Vec2F& localPoint) const
    {
        return mWorldTransform.ToBasis()*(localPoint / mSize.XY());
    }

    Vec2F ActorTransform::World2LocalDir(const Vec2F& worldDir) const
    {
        Vec2F nx = mWorldTransform.xv.XY() / (mSize.x*mScale.x), ny = mWorldTransform.yv.XY() / (mSize.y*mScale.y), wd = worldDir;
        float ldy = (wd.x*nx.y - wd.y*nx.x) / (nx.y*ny.x - ny.y*nx.x);
        float ldx = (wd.x - ny.x*ldy) / nx.x;
        return Vec2F(ldx, ldy);
    }

    Vec2F ActorTransform::Local2WorldDir(const Vec2F& localDir) const
    {
        Vec2F nx = mWorldTransform.xv.XY() / (mSize.x*mScale.x), ny = mWorldTransform.yv.XY() / (mSize.y*mScale.y);
        return nx*localDir.x + ny*localDir.y;
    }

    bool ActorTransform::IsPointInside(const Vec2F& point) const
    {
        Vec2F rs = (mScale*mSize).XY();
        Vec2F nx = mWorldTransform.xv.XY() / rs.x, ny = mWorldTransform.yv.XY() / rs.y;
        Vec2F lp = point - mWorldTransform.origin.XY();

        float dx = lp.Dot(nx);
        float dy = lp.Dot(ny);

        return dx >= 0.0f && dx <= rs.x && dy >= 0.0f && dy < rs.y;
    }

    void ActorTransform::SetOwner(const Ref<Actor>& actor)
    {
        mOwner = actor;
        SetDirty();
    }

    void ActorTransform::SetDirty(bool fromParent /*= false*/)
    {
        if (o2::Time::IsSingletonInitialzed())
            mDirtyFrame = o2Time.GetCurrentFrame();

        mUpdateFrame = 0;

#if IS_EDITOR
        if (mOwner && !fromParent)
            mOwner.Lock()->OnChanged();
#endif
    }

    void ActorTransform::Update()
    {
        UpdateLocalBox();
        UpdateTransform();
        UpdateWorldBoxAndTransform();

        mUpdateFrame = mDirtyFrame;

        if (auto owner = mOwner.Lock())
            owner->OnTransformUpdated();
    }

    void ActorTransform::UpdateLocalBox()
    {
        mLocalBox.min = mPosition - mSize*mPivot;
        mLocalBox.max = mLocalBox.min + mSize;
    }

    void ActorTransform::UpdateTransform()
    {
        mNonSizedTransform = Basis3D::Build(mPosition, mScale, mEulerAngles, mShear);

        mTransform.Set(mNonSizedTransform.origin, mNonSizedTransform.xv*mSize.x,
                         mNonSizedTransform.yv*mSize.y, mNonSizedTransform.zv*mSize.z);
        mTransform.origin -= mTransform.xv*mPivot.x + mTransform.yv*mPivot.y + mTransform.zv*mPivot.z;
    }

    void ActorTransform::UpdateWorldBoxAndTransform()
    {
        auto ownerActor = mOwner.Lock();
        if (mOwner && ownerActor->mParent)
        {
            auto parentTransform = ownerActor->mParent.Lock()->transform;
            mParentBox = parentTransform->mWorldBox;
            mParentBoxPosition = mParentBox.min + parentTransform->mSize*parentTransform->mPivot;

            mParentTransform = parentTransform->mWorldNonSizedTransform;
            mWorldNonSizedTransform = mNonSizedTransform*mParentTransform;
            mWorldTransform = mTransform*mParentTransform;
        }
        else
        {
            mParentBox = o2::AABB();
            mParentBoxPosition = Vec3F();

            mParentTransform = Basis3D::Identity();
            mWorldNonSizedTransform = mNonSizedTransform;
            mWorldTransform = mTransform;
        }

        mWorldBox.min = mParentBoxPosition + mLocalBox.min;
        mWorldBox.max = mParentBoxPosition + mLocalBox.max;
    }

    void ActorTransform::CheckParentInvTransform()
    {
        if (mParentInvertedTransformActualFrame == o2Time.GetCurrentFrame())
            return;

        mParentInvertedTransformActualFrame = o2Time.GetCurrentFrame();

        if (mOwner && mOwner.Lock()->mParent)
        {
            auto parentTransform = mOwner.Lock()->mParent.Lock()->transform;
            mParentInvertedTransform = parentTransform->mWorldNonSizedTransform.Inverted();
        }
        else
            mParentInvertedTransform = Basis3D::Identity();
    }

    void ActorTransform::OnSerialize(DataValue& node) const
    {
        if (!IsSerializeEnabled())
            return;

        auto serialize = [&node](const char* name, const Vec3F& value, const Vec3F& defaultValue) {
            if (!Math::Equals(value, defaultValue))
                node.AddMember(name).Set(value);
        };

        serialize("position", mPosition, Vec3F());
        serialize("size", mSize, Vec3F());
        serialize("scale", mScale, Vec3F(1, 1, 1));
        serialize("pivot", mPivot, Vec3F());
        serialize("eulerAngles", mEulerAngles, Vec3F());
        serialize("shear", mShear, Vec3F());
    }

    void ActorTransform::OnDeserialized(const DataValue& node)
    {
        ReadTransformFieldsCompat(node, mPosition, mSize, mScale, mPivot, mEulerAngles, mShear);
        SetDirty();
    }

    void ActorTransform::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        if (!IsSerializeEnabled())
            return;

        auto& other = dynamic_cast<const ActorTransform&>(origin);
        auto serialize = [&node](const char* name, const Vec3F& value, const Vec3F& originValue) {
            if (!EqualsForDeltaSerialize(value, originValue))
                node.AddMember(name).Set(value);
        };

        serialize("position", mPosition, other.mPosition);
        serialize("size", mSize, other.mSize);
        serialize("scale", mScale, other.mScale);
        serialize("pivot", mPivot, other.mPivot);
        serialize("eulerAngles", mEulerAngles, other.mEulerAngles);
        serialize("shear", mShear, other.mShear);
    }

    void ActorTransform::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        auto& other = dynamic_cast<const ActorTransform&>(origin);

        mPosition = other.mPosition;
        mSize = other.mSize;
        mScale = other.mScale;
        mPivot = other.mPivot;
        mEulerAngles = other.mEulerAngles;
        mShear = other.mShear;

        ReadTransformFieldsCompat(node, mPosition, mSize, mScale, mPivot, mEulerAngles, mShear);

        SetDirty();
    }

    Vec2F ActorTransform::GetParentPosition() const
    {
        if (!mOwner || !mOwner.Lock()->mParent)
            return Vec2F();

        return mOwner.Lock()->mParent.Lock()->transform->mWorldBox.min.XY();
    }

    RectF ActorTransform::GetParentRectangle() const
    {
        if (!mOwner || !mOwner.Lock()->mParent)
            return RectF();

        return mOwner.Lock()->mParent.Lock()->transform->GetWorldRect();
    }

    bool ActorTransform::IsSerializeEnabled() const
    {
        return true;
    }

}
// --- META ---

DECLARE_CLASS(o2::ActorTransform, o2__ActorTransform);
// --- END META ---
