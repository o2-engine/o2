#include "o2/stdafx.h"
#include "ActorTransform.h"

#include "o2/Application/Input.h"
#include "o2/Scene/Actor.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    ActorTransform::ActorTransform(ActorTransformData* data):
        mData(data)
    {}

    ActorTransform::ActorTransform(const Vec2F& size /*= Vec2F()*/, const Vec2F& position /*= Vec2F()*/,
                                   float angle /*= 0.0f*/, const Vec2F& scale /*= Vec2F(1.0f, 1.0f)*/,
                                   const Vec2F& pivot /*= Vec2F(0.5f, 0.5f)*/) :
        ActorTransform(mnew ActorTransformData())
    {
        mData->dirtyFrame = 1;
        mData->updateFrame = 1;

        mData->size = size;
        mData->position = position;
        mData->angle = angle;
        mData->scale = scale;
        mData->pivot = pivot;
    }

    ActorTransform::ActorTransform(const ActorTransform& other):
        ActorTransform(mnew ActorTransformData())
    {
        mData->dirtyFrame = 1;
        mData->updateFrame = 1;

        mData->size = other.mData->size;
        mData->position = other.mData->position;
        mData->angle = other.mData->angle;
        mData->scale = other.mData->scale;
        mData->pivot = other.mData->pivot;
        mData->shear = other.mData->shear;
    }

    ActorTransform::~ActorTransform()
    {
        delete mData;
    }

    void ActorTransform::CopyFrom(const ActorTransform& other)
    {
        mData->size = other.mData->size;
        mData->position = other.mData->position;
        mData->angle = other.mData->angle;
        mData->scale = other.mData->scale;
        mData->pivot = other.mData->pivot;
        mData->shear = other.mData->shear;

        SetDirty();
    }

    ActorTransform& ActorTransform::operator=(const ActorTransform& other)
    {
        CopyFrom(other);
        return *this;
    }

//     ActorTransform& ActorTransform::operator=(const Transform& other)
//     {
//         SetWorldBasis(other.GetBasis());
//         return *this;
//     }

    bool ActorTransform::operator==(const ActorTransform& other) const
    {
        return Math::Equals(mData->size, other.mData->size) &&
            Math::Equals(mData->position, other.mData->position) &&
            Math::Equals(mData->angle, other.mData->angle) &&
            Math::Equals(mData->scale, other.mData->scale) &&
            Math::Equals(mData->pivot, other.mData->pivot) &&
            Math::Equals(mData->shear, other.mData->shear);
    }

    void ActorTransform::SetPosition(const Vec2F& position)
    {
        mData->position = position;
        SetDirty();
    }

    Vec2F ActorTransform::GetPosition() const
    {
        return mData->position;
    }

    void ActorTransform::SetPositionX(float value)
    {
        mData->position.x = value;
        SetDirty();
    }

    float ActorTransform::GetPositionX() const
    {
        return mData->position.x;
    }

    void ActorTransform::SetPositionY(float value)
    {
        mData->position.y = value;
        SetDirty();
    }

    float ActorTransform::GetPositionY() const
    {
        return mData->position.y;
    }

    void ActorTransform::SetSize(const Vec2F& size)
    {
        mData->size = size;
        SetDirty();
    }

    Vec2F ActorTransform::GetSize() const
    {
        return mData->size;
    }

    void ActorTransform::SetWidth(float value)
    {
        mData->size.x = value;
        SetDirty();
    }

    float ActorTransform::GetWidth() const
    {
        return mData->size.x;
    }

    void ActorTransform::SetHeight(float value)
    {
        mData->size.y = value;
        SetDirty();
    }

    float ActorTransform::GetHeight() const
    {
        return mData->size.y;
    }

    void ActorTransform::SetPivot(const Vec2F& pivot)
    {
        mData->pivot = pivot;
        SetDirty();
    }

    Vec2F ActorTransform::GetPivot() const
    {
        return mData->pivot;
    }

    void ActorTransform::SetSizePivot(const Vec2F& relPivot)
    {
        SetPivot(relPivot / mData->size);
    }

    Vec2F ActorTransform::GetSizePivot() const
    {
        return mData->pivot*mData->size;
    }

    void ActorTransform::SetRect(const RectF& rect)
    {
        mData->size = rect.Size();
        mData->position = rect.LeftBottom() + mData->size*mData->pivot;

        SetDirty();
    }

    RectF ActorTransform::GetRect() const
    {
        Vec2F leftBottom = mData->position - mData->size*mData->pivot;
        return RectF(leftBottom, leftBottom + mData->size);
    }

    void ActorTransform::SetScale(const Vec2F& scale)
    {
        mData->scale = scale;
        SetDirty();
    }

    void ActorTransform::SetScaleX(float scaleX)
    {
        mData->scale.x = scaleX;
        SetDirty();
    }

    void ActorTransform::SetScaleY(float scaleY)
    {
        mData->scale.y = scaleY;
        SetDirty();
    }

    Vec2F ActorTransform::GetScale() const
    {
        return mData->scale;
    }

    float ActorTransform::GetScaleX() const
    {
        return mData->scale.x;
    }

    float ActorTransform::GetScaleY() const
    {
        return mData->scale.y;
    }

    void ActorTransform::SetAngle(float rad)
    {
        mData->angle = rad;
        SetDirty();
    }

    float ActorTransform::GetAngle() const
    {
        return mData->angle;
    }

    void ActorTransform::SetAngleDegrees(float deg)
    {
        mData->angle = Math::Deg2rad(deg);
        SetDirty();
    }

    float ActorTransform::GetAngleDegrees() const
    {
        return Math::Rad2deg(mData->angle);
    }

    void ActorTransform::SetShear(float shear)
    {
        mData->shear = shear;
        SetDirty();
    }

    float ActorTransform::GetShear() const
    {
        return mData->shear;
    }

    void ActorTransform::SetBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        mData->angle = angle;
        mData->size = scale / mData->scale;
        mData->shear = shear;

        mData->position = basis.origin + basis.xv*mData->pivot.x + basis.yv*mData->pivot.y;
        SetDirty();
    }

    Basis ActorTransform::GetBasis() const
    {
        return mData->transform;
    }

    void ActorTransform::SetNonSizedBasis(const Basis& basis)
    {
        Vec2F offset, scale;
        float angle, shear;
        basis.Decompose(&offset, &angle, &scale, &shear);

        mData->angle = angle;
        mData->scale = scale;
        mData->shear = shear;

        mData->position = basis.origin;
        SetDirty();
    }

    Basis ActorTransform::GetNonSizedBasis() const
    {
        return mData->nonSizedTransform;
    }

    void ActorTransform::SetAxisAlignedRect(const RectF& rect)
    {
        RectF curRect = GetAxisAlignedRect();
        Basis curRectBasis(curRect.LeftBottom(), Vec2F::Right()*curRect.Width(), Vec2F::Up()*curRect.Height());
        Basis rectBasis(rect.LeftBottom(), Vec2F::Right()*rect.Width(), Vec2F::Up()*rect.Height());

        SetBasis(mData->transform*curRectBasis.Inverted()*rectBasis);
    }

    RectF ActorTransform::GetAxisAlignedRect() const
    {
        return mData->transform.AABB();
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
        SetWorldBasis(mData->transform*transf);
    }

    Vec2F ActorTransform::GetRightDir() const
    {
        return mData->nonSizedTransform.xv;
    }

    void ActorTransform::SetLeftDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetLeftDir().SignedAngle(dir));
        SetWorldBasis(mData->transform*transf);
    }

    Vec2F ActorTransform::GetLeftDir() const
    {
        return mData->nonSizedTransform.xv.Inverted();
    }

    void ActorTransform::SetUpDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetUpDir().SignedAngle(dir));
        SetWorldBasis(mData->transform*transf);
    }

    Vec2F ActorTransform::GetUpDir() const
    {
        return mData->nonSizedTransform.yv;
    }

    void ActorTransform::SetDownDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetDownDir().SignedAngle(dir));
        SetWorldBasis(mData->transform*transf);
    }

    Vec2F ActorTransform::GetDownDir() const
    {
        return mData->nonSizedTransform.yv.Inverted();
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
        return mData->owner.Lock();
    }

    bool ActorTransform::IsDirty() const
    {
        return mData->updateFrame == 0;
    }

    void ActorTransform::SetWorldPivot(const Vec2F& pivot)
    {
        Basis trasform = mData->worldTransform;
        SetSizePivot(World2LocalPoint(pivot));
        SetWorldBasis(trasform);
    }

    Vec2F ActorTransform::GetWorldPivot() const
    {
        return Local2WorldPoint(mData->pivot*mData->size);
    }

    void ActorTransform::SetWorldPosition(const Vec2F& position)
    {
        CheckParentInvTransform();
        SetPosition(position*mData->parentInvertedTransform);
    }

    Vec2F ActorTransform::GetWorldPosition() const
    {
        return mData->position*mData->parentTransform;
    }

    void ActorTransform::SetWorldRect(const RectF& rect)
    {
        CheckParentInvTransform();
        SetRect(RectF(rect.LeftBottom()*mData->parentInvertedTransform, rect.RightTop()*mData->parentInvertedTransform));
    }

    RectF ActorTransform::GetWorldRect() const
    {
        return mData->worldRectangle;
    }

    void ActorTransform::SetWorldAngle(float rad)
    {
        SetAngle(rad - mData->parentTransform.GetAngle());
    }

    float ActorTransform::GetWorldAngle() const
    {
        return mData->worldTransform.GetAngle();
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
        SetBasis(basis*mData->parentInvertedTransform);
    }

    Basis ActorTransform::GetWorldBasis() const
    {
        if (IsDirty())
            const_cast<ActorTransform*>(this)->Update();

        return mData->worldTransform;
    }

    void ActorTransform::SetWorldNonSizedBasis(const Basis& basis)
    {
        CheckParentInvTransform();
        SetNonSizedBasis(basis*mData->parentInvertedTransform);
    }

    Basis ActorTransform::GetWorldNonSizedBasis() const
    {
        return mData->worldNonSizedTransform;
    }

    void ActorTransform::SetWorldAxisAlignedRect(const RectF& rect)
    {
        CheckParentInvTransform();
        SetAxisAlignedRect(RectF(rect.LeftBottom()*mData->parentInvertedTransform, rect.RightTop()*mData->parentInvertedTransform));
    }

    RectF ActorTransform::GetWorldAxisAlignedRect() const
    {
        RectF localAARect = GetRect();
        RectF worldAARect(localAARect.LeftBottom()*mData->parentTransform, localAARect.RightTop()*mData->parentTransform);
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
        SetWorldBasis(mData->worldTransform*Basis::Translated(translate));
    }

    Vec2F ActorTransform::GetWorldCenter() const
    {
        return mData->worldTransform.origin + (mData->worldTransform.xv + mData->worldTransform.yv)*0.5f;
    }

    void ActorTransform::SetWorldRightDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetRightDir().SignedAngle(dir));
        SetWorldBasis(mData->worldTransform*transf);
    }

    Vec2F ActorTransform::GetWorldRightDir() const
    {
        return mData->worldNonSizedTransform.xv;
    }

    void ActorTransform::SetWorldLeftDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetLeftDir().SignedAngle(dir));
        SetWorldBasis(mData->worldTransform*transf);
    }

    Vec2F ActorTransform::GetWorldLeftDir() const
    {
        return mData->worldNonSizedTransform.xv.Inverted();
    }

    void ActorTransform::SetWorldUpDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetUpDir().SignedAngle(dir));
        SetWorldBasis(mData->worldTransform*transf);
    }

    Vec2F ActorTransform::GetWorldUpDir() const
    {
        return mData->worldNonSizedTransform.yv;
    }

    void ActorTransform::SetWorldDownDir(const Vec2F& dir)
    {
        Basis transf = Basis::Rotated(GetDownDir().SignedAngle(dir));
        SetWorldBasis(mData->worldTransform*transf);
    }

    Vec2F ActorTransform::GetWorldDownDir() const
    {
        return mData->worldNonSizedTransform.yv.Inverted();
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
        Vec2F nx = mData->worldTransform.xv, ny = mData->worldTransform.yv, offs = mData->worldTransform.origin, w = worldPoint;
        float lx = (w.x*ny.y - offs.x*ny.y - w.y*ny.x + offs.y*ny.x) / (nx.x*ny.y - ny.x*nx.y);
        float ly = (w.y - offs.y - nx.y*lx) / ny.y;
        return Vec2F(lx, ly)*mData->size;
    }

    Vec2F ActorTransform::Local2WorldPoint(const Vec2F& localPoint) const
    {
        return mData->worldTransform*(localPoint / mData->size);
    }

    Vec2F ActorTransform::World2LocalDir(const Vec2F& worldDir) const
    {
        Vec2F nx = mData->worldTransform.xv / (mData->size.x*mData->scale.x), ny = mData->worldTransform.yv / (mData->size.y*mData->scale.y), wd = worldDir;
        float ldy = (wd.x*nx.y - wd.y*nx.x) / (nx.y*ny.x - ny.y*nx.x);
        float ldx = (wd.x - ny.x*ldy) / nx.x;
        return Vec2F(ldx, ldy);
    }

    Vec2F ActorTransform::Local2WorldDir(const Vec2F& localDir) const
    {
        Vec2F nx = mData->worldTransform.xv / (mData->size.x*mData->scale.x), ny = mData->worldTransform.yv / (mData->size.y*mData->scale.y);
        return nx*localDir.x + ny*localDir.y;
    }

    bool ActorTransform::IsPointInside(const Vec2F& point) const
    {
        Vec2F rs = mData->scale*mData->size;
        Vec2F nx = mData->worldTransform.xv / rs.x, ny = mData->worldTransform.yv / rs.y;
        Vec2F lp = point - mData->worldTransform.origin;

        float dx = lp.Dot(nx);
        float dy = lp.Dot(ny);

        return dx >= 0.0f && dx <= rs.x && dy >= 0.0f && dy < rs.y;
    }

    void ActorTransform::SetOwner(const Ref<Actor>& actor)
    {
        mData->owner = actor;
        SetDirty();
    }

    void ActorTransform::SetDirty(bool fromParent /*= false*/)
    {
        if (o2::Time::IsSingletonInitialzed())
            mData->dirtyFrame = o2Time.GetCurrentFrame();

        mData->updateFrame = 0;

#if IS_EDITOR
        if (mData->owner && !fromParent)
            mData->owner.Lock()->OnChanged();
#endif
    }

    void ActorTransform::Update()
    {
        UpdateRectangle();
        UpdateTransform();
        UpdateWorldRectangleAndTransform();

        mData->updateFrame = mData->dirtyFrame;
        mData->owner.Lock()->OnTransformUpdated();
    }

    void ActorTransform::UpdateRectangle()
    {
        Vec2F leftBottom = mData->position - mData->size*mData->pivot;
        Vec2F rightTop = leftBottom + mData->size;
        mData->rectangle.left = leftBottom.x;
        mData->rectangle.right = rightTop.x;
        mData->rectangle.bottom = leftBottom.y;
        mData->rectangle.top = rightTop.y;
    }

    void ActorTransform::UpdateTransform()
    {
        mData->nonSizedTransform = Basis::Build(mData->position, mData->scale, mData->angle, mData->shear);
        mData->transform.Set(mData->nonSizedTransform.origin, mData->nonSizedTransform.xv * mData->size.x, mData->nonSizedTransform.yv * mData->size.y);
        mData->transform.origin = mData->transform.origin - mData->transform.xv*mData->pivot.x - mData->transform.yv*mData->pivot.y;
    }

    void ActorTransform::UpdateWorldRectangleAndTransform()
    {
        auto ownerActor = mData->owner.Lock();
        if (mData->owner && ownerActor->mParent)
        {
            auto parentData = ownerActor->mParent.Lock()->transform->mData;
            mData->parentRectangle = parentData->worldRectangle;
            mData->parentRectangePosition = mData->parentRectangle.LeftBottom() + parentData->size*parentData->pivot;
            mData->worldRectangle.left   = mData->parentRectangePosition.x + mData->rectangle.left;
            mData->worldRectangle.right  = mData->parentRectangePosition.x + mData->rectangle.right;
            mData->worldRectangle.bottom = mData->parentRectangePosition.y + mData->rectangle.bottom;
            mData->worldRectangle.top    = mData->parentRectangePosition.y + mData->rectangle.top;

            mData->parentTransform = ownerActor->mParent.Lock()->transform->mData->worldNonSizedTransform;
            mData->worldNonSizedTransform = mData->nonSizedTransform*mData->parentTransform;
            mData->worldTransform = mData->transform*mData->parentTransform;
        }
        else
        {
            mData->parentRectangle.left = 0; mData->parentRectangle.right = 0;
            mData->parentRectangle.bottom = 0; mData->parentRectangle.top = 0;

            mData->parentRectangePosition = Vec2F();
            mData->worldRectangle.left   = mData->parentRectangePosition.x + mData->rectangle.left;
            mData->worldRectangle.right  = mData->parentRectangePosition.x + mData->rectangle.right;
            mData->worldRectangle.bottom = mData->parentRectangePosition.y + mData->rectangle.bottom;
            mData->worldRectangle.top    = mData->parentRectangePosition.y + mData->rectangle.top;

            mData->parentTransform = Basis::Identity();
            mData->worldNonSizedTransform = mData->nonSizedTransform;
            mData->worldTransform = mData->transform;
        }
    }

    void ActorTransform::CheckParentInvTransform()
    {
        if (mData->parentInvTransformActualFrame == o2Time.GetCurrentFrame())
            return;

        mData->parentInvTransformActualFrame = o2Time.GetCurrentFrame();

        if (mData->owner && mData->owner.Lock()->mParent)
            mData->parentInvertedTransform = mData->owner.Lock()->mParent.Lock()->transform->mData->worldNonSizedTransform.Inverted();
        else
            mData->parentInvertedTransform = Basis::Identity();
    }

    void ActorTransform::OnSerialize(DataValue& node) const
    {
        node.Set(*mData);
    }

    void ActorTransform::OnDeserialized(const DataValue& node)
    {
        node.Get(*mData);
        SetDirty();
    }

    void ActorTransform::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        node.SetDelta(*mData, *dynamic_cast<const ActorTransform&>(origin).mData);
    }

    void ActorTransform::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        node.GetDelta(*mData, *dynamic_cast<const ActorTransform&>(origin).mData);
        SetDirty();
    }

    Vec2F ActorTransform::GetParentPosition() const
    {
        if (!mData->owner || !mData->owner.Lock()->mParent)
            return Vec2F();

        return mData->owner.Lock()->mParent.Lock()->transform->mData->worldRectangle.LeftBottom();
    }

    RectF ActorTransform::GetParentRectangle() const
    {
        if (!mData->owner || !mData->owner.Lock()->mParent)
            return RectF();

        return mData->owner.Lock()->mParent.Lock()->transform->GetWorldRect();
    }

    bool ActorTransformData::IsSerializeEnabled() const
    {
        return true;
    }

}
// --- META ---

DECLARE_CLASS(o2::ActorTransform, o2__ActorTransform);

DECLARE_CLASS(o2::ActorTransformData, o2__ActorTransformData);
// --- END META ---
