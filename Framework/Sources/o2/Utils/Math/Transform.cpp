#include "o2/stdafx.h"
#include "Transform.h"

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

        // Reads transform fields in the current format and the legacy pre-3D one: float mAngle and mShear
        void ReadTransformFieldsCompat(const DataValue& node, Vec3F& position, Vec3F& size, Vec3F& scale,
                                       Vec3F& pivot, Vec3F& eulerAngles, Vec3F& shear)
        {
            ReadVec3FCompat(node, "mPosition", position);
            ReadVec3FCompat(node, "mSize", size);
            ReadVec3FCompat(node, "mScale", scale);
            ReadVec3FCompat(node, "mPivot", pivot);
            ReadVec3FCompat(node, "mEulerAngles", eulerAngles);

            if (auto member = node.FindMember("mAngle"); member && member->IsNumber())
                member->Get(eulerAngles.z);

            if (auto member = node.FindMember("mShear"))
            {
                if (member->IsObject())
                    ReadVec3FCompat(node, "mShear", shear);
                else if (member->IsNumber())
                    member->Get(shear.x);
            }
        }
    }

    Transform::Transform(const Vec2F& size /*= Vec2F()*/, const Vec2F& position /*= Vec2F()*/,
                         float angle /*= 0.0f*/, const Vec2F& scale /*= Vec2F(1.0f, 1.0f)*/,
                         const Vec2F& pivot /*= Vec2F(0.5f, 0.5f)*/):
        mSize(size), mPosition(position), mEulerAngles(0, 0, angle), mScale(scale, 1.0f), mPivot(pivot), mShear()
    {
        UpdateTransform();
    }

    Transform::Transform(const Transform& other):
        mSize(other.mSize), mPosition(other.mPosition), mEulerAngles(other.mEulerAngles), mScale(other.mScale),
        mPivot(other.mPivot), mShear(other.mShear), mTransform(other.mTransform),
        mNonSizedTransform(other.mNonSizedTransform)
    {}

    void Transform::OnSerialize(DataValue& node) const
    {
        if (!IsSerializeEnabled())
            return;

        auto serialize = [&node](const char* name, const Vec3F& value, const Vec3F& defaultValue) {
            if (!Math::Equals(value, defaultValue))
                node.AddMember(name).Set(value);
        };

        serialize("mPosition", mPosition, Vec3F());
        serialize("mSize", mSize, Vec3F());
        serialize("mScale", mScale, Vec3F(1, 1, 1));
        serialize("mPivot", mPivot, Vec3F());
        serialize("mEulerAngles", mEulerAngles, Vec3F());
        serialize("mShear", mShear, Vec3F());
    }

    void Transform::OnDeserialized(const DataValue& node)
    {
        ReadTransformFieldsCompat(node, mPosition, mSize, mScale, mPivot, mEulerAngles, mShear);
        UpdateTransform();
    }

    void Transform::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        if (!IsSerializeEnabled())
            return;

        auto& other = dynamic_cast<const Transform&>(origin);
        auto serialize = [&node](const char* name, const Vec3F& value, const Vec3F& originValue) {
            if (!EqualsForDeltaSerialize(value, originValue))
                node.AddMember(name).Set(value);
        };

        serialize("mPosition", mPosition, other.mPosition);
        serialize("mSize", mSize, other.mSize);
        serialize("mScale", mScale, other.mScale);
        serialize("mPivot", mPivot, other.mPivot);
        serialize("mEulerAngles", mEulerAngles, other.mEulerAngles);
        serialize("mShear", mShear, other.mShear);
    }

    void Transform::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        auto& other = dynamic_cast<const Transform&>(origin);

        mPosition = other.mPosition;
        mSize = other.mSize;
        mScale = other.mScale;
        mPivot = other.mPivot;
        mEulerAngles = other.mEulerAngles;
        mShear = other.mShear;

        ReadTransformFieldsCompat(node, mPosition, mSize, mScale, mPivot, mEulerAngles, mShear);

        UpdateTransform();
    }

    void Transform::UpdateTransform()
    {
        mNonSizedTransform = Basis3D::Build(mPosition, mScale, mEulerAngles, mShear);
        mTransform.Set(mNonSizedTransform.origin, mNonSizedTransform.xv*mSize.x, mNonSizedTransform.yv*mSize.y,
                       mNonSizedTransform.zv*mSize.z);
        mTransform.origin -= mTransform.xv*mPivot.x + mTransform.yv*mPivot.y + mTransform.zv*mPivot.z;
        mNonSizedTransform.origin = mTransform.origin;

        BasisChanged();
    }

    bool Transform::IsSerializeEnabled() const
    {
        return mSerializeEnabled;
    }

    Transform& Transform::operator=(const Transform& other)
    {
        mPosition = other.mPosition;
        mSize = other.mSize;
        mScale = other.mScale;
        mPivot = other.mPivot;
        mEulerAngles = other.mEulerAngles;
        mShear = other.mShear;
        mTransform = other.mTransform;
        mNonSizedTransform = other.mNonSizedTransform;

        UpdateTransform();
        BasisChanged();

        return *this;
    }

    bool Transform::operator==(const Transform& other) const
    {
        return mPosition == other.mPosition &&
            mSize == other.mSize &&
            mScale == other.mScale &&
            mPivot == other.mPivot &&
            mEulerAngles == other.mEulerAngles &&
            mShear == other.mShear &&
            mTransform == other.mTransform &&
            mNonSizedTransform == other.mNonSizedTransform;
    }

    bool Transform::operator!=(const Transform& other) const
    {
        return !operator==(other);
    }

    void Transform::SetPosition(const Vec3F& position)
    {
        mPosition = position;
        UpdateTransform();
    }

    Vec3F Transform::GetPosition() const
    {
        return mPosition;
    }

    void Transform::SetPosition2D(const Vec2F& position)
    {
        mPosition.x = position.x;
        mPosition.y = position.y;
        UpdateTransform();
    }

    Vec2F Transform::GetPosition2D() const
    {
        return mPosition.XY();
    }

    void Transform::SetPositionX(float value)
    {
        mPosition.x = value;
        UpdateTransform();
    }

    float Transform::GetPositionX() const
    {
        return mPosition.x;
    }

    void Transform::SetPositionY(float value)
    {
        mPosition.y = value;
        UpdateTransform();
    }

    float Transform::GetPositionY() const
    {
        return mPosition.y;
    }

    void Transform::SetPositionZ(float value)
    {
        mPosition.z = value;
        UpdateTransform();
    }

    float Transform::GetPositionZ() const
    {
        return mPosition.z;
    }

    void Transform::SetSize(const Vec3F& size)
    {
        mSize = size;
        UpdateTransform();
    }

    Vec3F Transform::GetSize() const
    {
        return mSize;
    }

    void Transform::SetSize2D(const Vec2F& size)
    {
        mSize.x = size.x;
        mSize.y = size.y;
        UpdateTransform();
    }

    Vec2F Transform::GetSize2D() const
    {
        return mSize.XY();
    }

    void Transform::SetWidth(float width)
    {
        mSize.x = width;
        UpdateTransform();
    }

    float Transform::GetWidth() const
    {
        return mSize.x;
    }

    void Transform::SetHeight(float height)
    {
        mSize.y = height;
        UpdateTransform();
    }

    float Transform::GetHeight() const
    {
        return mSize.y;
    }

    void Transform::SetSizeZ(float value)
    {
        mSize.z = value;
        UpdateTransform();
    }

    float Transform::GetSizeZ() const
    {
        return mSize.z;
    }

    void Transform::SetPivot(const Vec3F& pivot)
    {
        mPivot = pivot;
        UpdateTransform();
    }

    Vec3F Transform::GetPivot() const
    {
        return mPivot;
    }

    void Transform::SetPivot2D(const Vec2F& pivot)
    {
        mPivot.x = pivot.x;
        mPivot.y = pivot.y;
        UpdateTransform();
    }

    Vec2F Transform::GetPivot2D() const
    {
        return mPivot.XY();
    }

    void Transform::SetWorldPivot(const Vec2F& pivot)
    {
        SetSizePivot(World2LocalPoint(pivot));
    }

    Vec2F Transform::GetWorldPivot() const
    {
        return Local2WorldPoint(mPivot.XY()*mSize.XY());
    }

    void Transform::SetSizePivot(const Vec2F& relPivot)
    {
        SetPivot2D(relPivot / mSize.XY());
    }

    Vec2F Transform::GetSizePivot() const
    {
        return mPivot.XY()*mSize.XY();
    }

    void Transform::SetRect(const RectF& rect, bool bySize /*= true*/)
    {
        Vec2F size;
        if (bySize)
        {
            size = rect.Size()/mScale.XY();
            mSize.x = size.x;
            mSize.y = size.y;
        }
        else
        {
            Vec2F scale = rect.Size()/mSize.XY();
            mScale.x = scale.x;
            mScale.y = scale.y;
        }

        Vec2F position = rect.LeftBottom() + Vec2F(mSize.x*mScale.x, mSize.y*mScale.y)*mPivot.XY();
        mPosition.x = position.x;
        mPosition.y = position.y;

        UpdateTransform();
    }

    RectF Transform::GetRect() const
    {
        Vec2F origin = mTransform.origin.XY();
        return RectF(origin, origin + mTransform.xv.XY() + mTransform.yv.XY());
    }

    void Transform::SetScale(const Vec3F& scale)
    {
        mScale = scale;
        UpdateTransform();
    }

    Vec3F Transform::GetScale() const
    {
        return mScale;
    }

    void Transform::SetScale2D(const Vec2F& scale)
    {
        mScale.x = scale.x;
        mScale.y = scale.y;
        UpdateTransform();
    }

    Vec2F Transform::GetScale2D() const
    {
        return mScale.XY();
    }

    void Transform::SetScaleX(float scale)
    {
        mScale.x = scale;
        UpdateTransform();
    }

    float Transform::GetScaleX() const
    {
        return mScale.x;
    }

    void Transform::SetScaleY(float scale)
    {
        mScale.y = scale;
        UpdateTransform();
    }

    float Transform::GetScaleY() const
    {
        return mScale.y;
    }

    void Transform::SetScaleZ(float scale)
    {
        mScale.z = scale;
        UpdateTransform();
    }

    float Transform::GetScaleZ() const
    {
        return mScale.z;
    }

    void Transform::SetAngle(float rad)
    {
        mEulerAngles.z = rad;
        UpdateTransform();
    }

    float Transform::GetAngle() const
    {
        return mEulerAngles.z;
    }

    void Transform::SetAngleDegrees(float deg)
    {
        mEulerAngles.z = Math::Deg2rad(deg);
        UpdateTransform();
    }

    float Transform::GetAngleDegrees() const
    {
        return Math::Rad2deg(mEulerAngles.z);
    }

    void Transform::SetEulerAngles(const Vec3F& radians)
    {
        mEulerAngles = radians;
        UpdateTransform();
    }

    Vec3F Transform::GetEulerAngles() const
    {
        return mEulerAngles;
    }

    void Transform::SetEulerAnglesDegrees(const Vec3F& degrees)
    {
        SetEulerAngles(degrees*Math::Deg2rad(1.0f));
    }

    Vec3F Transform::GetEulerAnglesDegrees() const
    {
        return mEulerAngles*Math::Rad2deg(1.0f);
    }

    void Transform::SetRotation(const Quat& rotation)
    {
        SetEulerAngles(rotation.ToEuler());
    }

    Quat Transform::GetRotation() const
    {
        return Quat::FromEuler(mEulerAngles);
    }

    void Transform::SetShear(const Vec3F& shear)
    {
        mShear = shear;
        UpdateTransform();
    }

    Vec3F Transform::GetShear() const
    {
        return mShear;
    }

    void Transform::SetShear2D(float shear)
    {
        mShear.x = shear;
        UpdateTransform();
    }

    float Transform::GetShear2D() const
    {
        return mShear.x;
    }

    void Transform::SetBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        mEulerAngles.z = angle;
        mShear.x = shear;

        Vec2F size = scale / mScale.XY();
        mSize.x = size.x;
        mSize.y = size.y;

        Vec2F position = basis.origin + basis.xv*mPivot.x + basis.yv*mPivot.y;
        mPosition.x = position.x;
        mPosition.y = position.y;

        UpdateTransform();
    }

    Basis Transform::GetBasis() const
    {
        return mTransform.ToBasis();
    }

    void Transform::SetNonSizedBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        mEulerAngles.z = angle;
        mScale.x = scale.x;
        mScale.y = scale.y;
        mShear.x = shear;

        Vec2F position = basis.origin + basis.xv*mPivot.x*mSize.x + basis.yv*mPivot.y*mSize.y;
        mPosition.x = position.x;
        mPosition.y = position.y;

        UpdateTransform();
    }

    Basis Transform::GetNonSizedBasis() const
    {
        return mNonSizedTransform.ToBasis();
    }

    Basis3D Transform::GetBasis3D() const
    {
        return mTransform;
    }

    Basis3D Transform::GetNonSizedBasis3D() const
    {
        return mNonSizedTransform;
    }

    void Transform::SetAxisAlignedRect(const RectF& rect)
    {
        RectF curRect = GetAxisAlignedRect();

        Basis curRectBasis(curRect.LeftBottom(), Vec2F::Right()*curRect.Width(), Vec2F::Up()*curRect.Height());
        Basis rectBasis(rect.LeftBottom(), Vec2F::Right()*rect.Width(), Vec2F::Up()*rect.Height());

        SetBasis(mTransform.ToBasis()*curRectBasis.Inverted()*rectBasis);
    }

    RectF Transform::GetAxisAlignedRect() const
    {
        return mTransform.ToBasis().AABB();
    }

    void Transform::SetLeftTop(const Vec2F& position)
    {
        Basis transformed = mTransform.ToBasis();
        Vec2F lastHandleCoords = Vec2F(0.0f, 1.0f)*transformed;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(transformed.xv);
        Vec2F frameDeltaY = delta.Project(transformed.yv);

        transformed.origin += frameDeltaX;
        transformed.xv -= frameDeltaX;
        transformed.yv += frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetLeftTop() const
    {
        return (mTransform.origin + mTransform.yv).XY();
    }

    void Transform::SetRightTop(const Vec2F& position)
    {
        Basis transformed = mTransform.ToBasis();
        Vec2F lastHandleCoords = Vec2F(1.0f, 1.0f)*transformed;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(transformed.xv);
        Vec2F frameDeltaY = delta.Project(transformed.yv);

        transformed.xv += frameDeltaX;
        transformed.yv += frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetRightTop() const
    {
        return (mTransform.origin + mTransform.yv + mTransform.xv).XY();
    }

    void Transform::SetLeftBottom(const Vec2F& position)
    {
        Basis transformed = mTransform.ToBasis();
        Vec2F lastHandleCoords = Vec2F(0.0f, 0.0f)*transformed;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(transformed.xv);
        Vec2F frameDeltaY = delta.Project(transformed.yv);

        transformed.origin += frameDeltaX + frameDeltaY;
        transformed.xv -= frameDeltaX;
        transformed.yv -= frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetLeftBottom() const
    {
        return mTransform.origin.XY();
    }

    void Transform::SetRightBottom(const Vec2F& position)
    {
        Basis transformed = mTransform.ToBasis();
        Vec2F lastHandleCoords = Vec2F(1.0f, 0.0f)*transformed;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(transformed.xv);
        Vec2F frameDeltaY = delta.Project(transformed.yv);

        transformed.origin += frameDeltaY;
        transformed.xv += frameDeltaX;
        transformed.yv -= frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetRightBottom() const
    {
        return (mTransform.origin + mTransform.xv).XY();
    }

    void Transform::SetCenter(const Vec2F& position)
    {
        Vec2F translate = position - GetCenter();
        SetBasis(mTransform.ToBasis()*Basis::Translated(translate));
    }

    Vec2F Transform::GetCenter() const
    {
        return (mTransform.origin + (mTransform.xv + mTransform.yv)*0.5f).XY();
    }

    void Transform::SetRight(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetRight().SignedAngle(dir));
        SetBasis(mTransform.ToBasis()*transf);
    }

    Vec2F Transform::GetRight() const
    {
        return mNonSizedTransform.xv.XY() / (mSize.x*mScale.x);
    }

    void Transform::SetLeft(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetLeft().SignedAngle(dir));
        SetBasis(mTransform.ToBasis()*transf);
    }

    Vec2F Transform::GetLeft() const
    {
        return mNonSizedTransform.xv.XY() / (-(mSize.x*mScale.x));
    }

    void Transform::SetUp(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetUp().SignedAngle(dir));
        SetBasis(mTransform.ToBasis()*transf);
    }

    Vec2F Transform::GetUp() const
    {
        return mNonSizedTransform.yv.XY() / (mSize.y*mScale.y);
    }

    void Transform::SetDown(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetDown().SignedAngle(dir));
        SetBasis(mTransform.ToBasis()*transf);
    }

    Vec2F Transform::GetDown() const
    {
        return mNonSizedTransform.yv.XY() / (-(mSize.y*mScale.y));
    }

    void Transform::LookAt(const Vec2F& worldPoint)
    {
        SetUp((worldPoint - mPosition.XY()).Normalized());
    }

    Vec2F Transform::World2LocalPoint(const Vec2F& worldPoint) const
    {
        Vec2F nx = mTransform.xv.XY(), ny = mTransform.yv.XY(), offs = mTransform.origin.XY(), w = worldPoint;
        float lx = (w.x*ny.y - offs.x*ny.y - w.y*ny.x + offs.y*ny.x) / (nx.x*ny.y - ny.x*nx.y);
        float ly = (w.y - offs.y - nx.y*lx) / ny.y;
        return Vec2F(lx, ly)*mSize.XY();
    }

    Vec2F Transform::Local2WorldPoint(const Vec2F& localPoint) const
    {
        return mTransform.ToBasis()*(localPoint / mSize.XY());
    }

    Vec2F Transform::World2LocalDir(const Vec2F& worldDir) const
    {
        Vec2F nx = mTransform.xv.XY() / (mSize.x*mScale.x), ny = mTransform.yv.XY() / (mSize.y*mScale.y), wd = worldDir;
        float ldy = (wd.x*nx.y - wd.y*nx.x) / (nx.y*ny.x - ny.y*nx.x);
        float ldx = (wd.x - ny.x*ldy) / nx.x;
        return Vec2F(ldx, ldy);
    }

    Vec2F Transform::Local2WorldDir(const Vec2F& localDir) const
    {
        Vec2F nx = mTransform.xv.XY() / (mSize.x*mScale.x), ny = mTransform.yv.XY() / (mSize.y*mScale.y);
        return nx*localDir.x + ny*localDir.y;
    }

    bool Transform::IsPointInside(const Vec2F& point) const
    {
        Vec2F rs = mScale.XY()*mSize.XY();
        Vec2F nx = mTransform.xv.XY() / rs.x, ny = mTransform.yv.XY() / rs.y;
        Vec2F lp = point - mTransform.origin.XY();

        float dx = lp.Dot(nx);
        float dy = lp.Dot(ny);

        return dx >= 0.0f && dx <= rs.x && dy >= 0.0f && dy < rs.y;
    }

    void Transform::SetSerializeEnabled(bool enabled)
    {
        mSerializeEnabled = enabled;
    }

}
// --- META ---

DECLARE_CLASS(o2::Transform, o2__Transform);
// --- END META ---
