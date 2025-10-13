#include "o2/stdafx.h"
#include "Transform.h"

namespace o2
{
    Transform::Transform(const Vec2F& size /*= Vec2F()*/, const Vec2F& position /*= Vec2F()*/,
                         float angle /*= 0.0f*/, const Vec2F& scale /*= Vec2F(1.0f, 1.0f)*/,
                         const Vec2F& pivot /*= Vec2F(0.5f, 0.5f)*/):
        mSize(size), mPosition(position), mAngle(angle), mScale(scale), mPivot(pivot), mShear(0)
    {
        UpdateTransform();
    }

    Transform::Transform(const Transform& other):
        mSize(other.mSize), mPosition(other.mPosition), mAngle(other.mAngle), mScale(other.mScale),
        mPivot(other.mPivot), mShear(other.mShear), mTransform(other.mTransform),
        mNonSizedTransform(other.mNonSizedTransform)
    {}

    void Transform::OnDeserialized(const DataValue& node)
    {
        UpdateTransform();
    }

    void Transform::UpdateTransform()
    {
        mNonSizedTransform = Basis::Build(mPosition, mScale, mAngle, mShear);
        mTransform.Set(mNonSizedTransform.origin, mNonSizedTransform.xv * mSize.x, mNonSizedTransform.yv * mSize.y);
        mTransform.origin = mTransform.origin - mTransform.xv*mPivot.x - mTransform.yv*mPivot.y;
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
        mAngle = other.mAngle;
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
            Math::Equals(mAngle, other.mAngle) &&
            Math::Equals(mShear, other.mShear) &&
            mTransform == other.mTransform &&
            mNonSizedTransform == other.mNonSizedTransform;
    }

    bool Transform::operator!=(const Transform& other) const
    {
        return !operator==(other);
    }

    void Transform::SetPosition(const Vec2F& position)
    {
        mPosition = position;
        UpdateTransform();
    }

    Vec2F Transform::GetPosition() const
    {
        return mPosition;
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

    void Transform::SetSize(const Vec2F& size)
    {
        mSize = size;
        UpdateTransform();
    }

    Vec2F Transform::GetSize() const
    {
        return mSize;
    }

    void Transform::SetWidth(float width)
    {
        SetSize(Vec2F(width, GetHeight()));
    }

    float Transform::GetWidth() const
    {
        return GetSize().x;
    }

    void Transform::SetHeight(float height)
    {
        SetSize(Vec2F(GetWidth(), height));
    }

    float Transform::GetHeight() const
    {
        return GetSize().y;
    }

    void Transform::SetPivot(const Vec2F& pivot)
    {
        mPivot = pivot;
        UpdateTransform();
    }

    Vec2F Transform::GetPivot() const
    {
        return mPivot;
    }

    void Transform::SetWorldPivot(const Vec2F& pivot)
    {
        SetSizePivot(World2LocalPoint(pivot));
    }

    Vec2F Transform::GetWorldPivot() const
    {
        return Local2WorldPoint(mPivot*mSize);
    }

    void Transform::SetSizePivot(const Vec2F& relPivot)
    {
        SetPivot(relPivot / mSize);
    }

    Vec2F Transform::GetSizePivot() const
    {
        return mPivot*mSize;
    }

    void Transform::SetRect(const RectF& rect, bool bySize /*= true*/)
    {
        if (bySize)
            mSize = rect.Size()/mScale;
        else
            mScale = rect.Size()/mSize;

        mPosition = rect.LeftBottom() + Vec2F(mSize.x*mScale.x, mSize.y*mScale.y)*mPivot;

        UpdateTransform();
    }

    RectF Transform::GetRect() const
    {
        return RectF(mTransform.origin, mTransform.origin + mTransform.xv + mTransform.yv);
    }

    void Transform::SetScale(const Vec2F& scale)
    {
        mScale = scale;
        UpdateTransform();
    }

    Vec2F Transform::GetScale() const
    {
        return mScale;
    }

    void Transform::SetScaleX(float scale)
    {
        SetScale(Vec2F(scale, GetScaleY()));
    }

    float Transform::GetScaleX() const
    {
        return GetScale().x;
    }

    void Transform::SetScaleY(float scale)
    {
        SetScale(Vec2F(GetScaleX(), scale));
    }

    float Transform::GetScaleY() const
    {
        return GetScale().y;
    }

    void Transform::SetAngle(float rad)
    {
        mAngle = rad;
        UpdateTransform();
    }

    float Transform::GetAngle() const
    {
        return mAngle;
    }

    void Transform::SetAngleDegrees(float deg)
    {
        mAngle = Math::Deg2rad(deg);
        UpdateTransform();
    }

    float Transform::GetAngleDegrees() const
    {
        return Math::Rad2deg(mAngle);
    }

    void Transform::SetShear(float shear)
    {
        mShear = shear;
        UpdateTransform();
    }

    float Transform::GetShear() const
    {
        return mShear;
    }

    void Transform::SetBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        mAngle = angle;
        mSize = scale / mScale;
        mShear = shear;

        mPosition = basis.origin + basis.xv*mPivot.x + basis.yv*mPivot.y;
        UpdateTransform();
    }

    Basis Transform::GetBasis() const
    {
        return mTransform;
    }

    void Transform::SetNonSizedBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        mAngle = angle;
        mScale = scale;
        mShear = shear;

        mPosition = basis.origin + basis.xv*mPivot.x*mSize.x + basis.yv*mPivot.y*mSize.y;
        UpdateTransform();
    }

    Basis Transform::GetNonSizedBasis() const
    {
        return mNonSizedTransform;
    }

    void Transform::SetAxisAlignedRect(const RectF& rect)
    {
        RectF curRect = GetAxisAlignedRect();

        Basis curRectBasis(curRect.LeftBottom(), Vec2F::Right()*curRect.Width(), Vec2F::Up()*curRect.Height());
        Basis rectBasis(rect.LeftBottom(), Vec2F::Right()*rect.Width(), Vec2F::Up()*rect.Height());

        SetBasis(mTransform*curRectBasis.Inverted()*rectBasis);
    }

    RectF Transform::GetAxisAlignedRect() const
    {
        return mTransform.AABB();
    }

    void Transform::SetLeftTop(const Vec2F& position)
    {
        Basis transformed = mTransform;
        Vec2F lastHandleCoords = Vec2F(0.0f, 1.0f)*mTransform;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mTransform.xv);
        Vec2F frameDeltaY = delta.Project(mTransform.yv);

        transformed.origin += frameDeltaX;
        transformed.xv -= frameDeltaX;
        transformed.yv += frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetLeftTop() const
    {
        return mTransform.origin + mTransform.yv;
    }

    void Transform::SetRightTop(const Vec2F& position)
    {
        Basis transformed = mTransform;
        Vec2F lastHandleCoords = Vec2F(1.0f, 1.0f)*mTransform;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mTransform.xv);
        Vec2F frameDeltaY = delta.Project(mTransform.yv);

        transformed.xv += frameDeltaX;
        transformed.yv += frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetRightTop() const
    {
        return mTransform.origin + mTransform.yv + mTransform.xv;
    }

    void Transform::SetLeftBottom(const Vec2F& position)
    {
        Basis transformed = mTransform;
        Vec2F lastHandleCoords = Vec2F(0.0f, 0.0f)*mTransform;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mTransform.xv);
        Vec2F frameDeltaY = delta.Project(mTransform.yv);

        transformed.origin += frameDeltaX + frameDeltaY;
        transformed.xv -= frameDeltaX;
        transformed.yv -= frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetLeftBottom() const
    {
        return mTransform.origin;
    }

    void Transform::SetRightBottom(const Vec2F& position)
    {
        Basis transformed = mTransform;
        Vec2F lastHandleCoords = Vec2F(1.0f, 0.0f)*mTransform;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mTransform.xv);
        Vec2F frameDeltaY = delta.Project(mTransform.yv);

        transformed.origin += frameDeltaY;
        transformed.xv += frameDeltaX;
        transformed.yv -= frameDeltaY;

        SetBasis(transformed);
    }

    Vec2F Transform::GetRightBottom() const
    {
        return mTransform.origin + mTransform.xv;
    }

    void Transform::SetCenter(const Vec2F& position)
    {
        Vec2F translate = position - GetCenter();
        SetBasis(mTransform*Basis::Translated(translate));
    }

    Vec2F Transform::GetCenter() const
    {
        return mTransform.origin + (mTransform.xv + mTransform.yv)*0.5f;
    }

    void Transform::SetRight(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetRight().SignedAngle(dir));
        SetBasis(mTransform*transf);
    }

    Vec2F Transform::GetRight() const
    {
        return mNonSizedTransform.xv / (mSize.x*mScale.x);
    }

    void Transform::SetLeft(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetLeft().SignedAngle(dir));
        SetBasis(mTransform*transf);
    }

    Vec2F Transform::GetLeft() const
    {
        return mNonSizedTransform.xv / (-(mSize.x*mScale.x));
    }

    void Transform::SetUp(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetUp().SignedAngle(dir));
        SetBasis(mTransform*transf);
    }

    Vec2F Transform::GetUp() const
    {
        return mNonSizedTransform.yv / (mSize.y*mScale.y);
    }

    void Transform::SetDown(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetDown().SignedAngle(dir));
        SetBasis(mTransform*transf);
    }

    Vec2F Transform::GetDown() const
    {
        return mNonSizedTransform.yv / (-(mSize.y*mScale.y));
    }

    void Transform::LookAt(const Vec2F& worldPoint)
    {
        SetUp((worldPoint - mPosition).Normalized());
    }

    Vec2F Transform::World2LocalPoint(const Vec2F& worldPoint) const
    {
        Vec2F nx = mTransform.xv, ny = mTransform.yv, offs = mTransform.origin, w = worldPoint;
        float lx = (w.x*ny.y - offs.x*ny.y - w.y*ny.x + offs.y*ny.x) / (nx.x*ny.y - ny.x*nx.y);
        float ly = (w.y - offs.y - nx.y*lx) / ny.y;
        return Vec2F(lx, ly)*mSize;
    }

    Vec2F Transform::Local2WorldPoint(const Vec2F& localPoint) const
    {
        return mTransform*(localPoint / mSize);
    }

    Vec2F Transform::World2LocalDir(const Vec2F& worldDir) const
    {
        Vec2F nx = mTransform.xv / (mSize.x*mScale.x), ny = mTransform.yv / (mSize.y*mScale.y), wd = worldDir;
        float ldy = (wd.x*nx.y - wd.y*nx.x) / (nx.y*ny.x - ny.y*nx.x);
        float ldx = (wd.x - ny.x*ldy) / nx.x;
        return Vec2F(ldx, ldy);
    }

    Vec2F Transform::Local2WorldDir(const Vec2F& localDir) const
    {
        Vec2F nx = mTransform.xv / (mSize.x*mScale.x), ny = mTransform.yv / (mSize.y*mScale.y);
        return nx*localDir.x + ny*localDir.y;
    }

    bool Transform::IsPointInside(const Vec2F& point) const
    {
        Vec2F rs = mScale*mSize;
        Vec2F nx = mTransform.xv / rs.x, ny = mTransform.yv / rs.y;
        Vec2F lp = point - mTransform.origin;

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
