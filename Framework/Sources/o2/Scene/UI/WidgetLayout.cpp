#include "o2/stdafx.h"
#include "WidgetLayout.h"

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"

namespace o2
{
    WidgetLayout::WidgetLayout() :
        ActorTransform(Vec2F(), Vec2F(), 0.0f, Vec2F(1.0f, 1.0f), Vec2F())
    {
        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
    }

    WidgetLayout::WidgetLayout(const WidgetLayout& other) :
        ActorTransform(Vec2F(), Vec2F(), 0.0f, Vec2F(1.0f, 1.0f), Vec2F())
    {
        CopyFrom(other);
        mCheckMinMaxFunc = other.mCheckMinMaxFunc;
    }

    WidgetLayout::WidgetLayout(const Vec2F& anchorMin, const Vec2F& anchorMax,
                               const Vec2F& offsetMin, const Vec2F& offsetMax) :
        ActorTransform(Vec2F(), Vec2F(), 0.0f, Vec2F(1.0f, 1.0f), Vec2F())
    {
        mAnchorMin = anchorMin;
        mAnchorMax = anchorMax;
        mOffsetMin = offsetMin;
        mOffsetMax = offsetMax;

        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
    }

    WidgetLayout::WidgetLayout(float anchorLeft, float anchorTop, float anchorRight, float anchorBottom,
                               float offsetLeft, float offsetTop, float offsetRight, float offsetBottom) :
        ActorTransform(Vec2F(), Vec2F(), 0.0f, Vec2F(1.0f, 1.0f), Vec2F())
    {
        mAnchorMin.Set(anchorLeft, anchorBottom);
        mAnchorMax.Set(anchorRight, anchorTop);
        mOffsetMin.Set(offsetLeft, offsetBottom);
        mOffsetMax.Set(offsetRight, offsetTop);

        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
    }

    WidgetLayout& WidgetLayout::operator=(const WidgetLayout& other)
    {
        CopyFrom(other);
        SetDirty();

        return *this;
    }

    bool WidgetLayout::operator==(const WidgetLayout& other) const
    {
        return mAnchorMin == other.mAnchorMin &&
            mAnchorMax == other.mAnchorMax &&
            mOffsetMin == other.mOffsetMin &&
            mOffsetMax == other.mOffsetMax;
    }

    void WidgetLayout::SetPosition2D(const Vec2F& position)
    {
        Vec2F delta = position - GetPosition2D();
        mOffsetMin += delta;
        mOffsetMax += delta;

        SetDirty();
    }

    void WidgetLayout::SetSize2D(const Vec2F& size)
    {
        RectF parentRect = GetParentRectangle();
        RectF rectangle(mOffsetMin + mAnchorMin*parentRect.Size(),
                        mOffsetMax + mAnchorMax*parentRect.Size());

        Vec2F szDelta = size - rectangle.Size();
        mOffsetMax += szDelta*(Vec2F::One() - mPivot.XY());
        mOffsetMin -= szDelta*mPivot.XY();

        SetDirty();
    }

    void WidgetLayout::SetWidth(float value)
    {
        RectF parentRect = GetParentRectangle();
        RectF rectangle(mOffsetMin + mAnchorMin*parentRect.Size(),
                        mOffsetMax + mAnchorMax*parentRect.Size());

        float szDelta = value - rectangle.Width();
        mOffsetMax.x += szDelta*(1.0f - mPivot.x);
        mOffsetMin.x -= szDelta*mPivot.x;

        SetDirty();
    }

    void WidgetLayout::SetHeight(float value)
    {
        RectF parentRect = GetParentRectangle();
        RectF rectangle(mOffsetMin + mAnchorMin*parentRect.Size(),
                        mOffsetMax + mAnchorMax*parentRect.Size());

        float szDelta = value - rectangle.Height();
        mOffsetMax.y += szDelta*mPivot.y;
        mOffsetMin.y -= szDelta*(1.0f - mPivot.y);

        SetDirty();
    }

    Vec2F WidgetLayout::GetSize2D() const
    {
        return Vec2F(GetWidth(), GetHeight());
    }

    float WidgetLayout::GetWidth() const
    {
        return Math::Clamp(mSize.x, mMinSize.x, mMaxSize.x);
    }

    float WidgetLayout::GetHeight() const
    {
        return Math::Clamp(mSize.y, mMinSize.y, mMaxSize.y);
    }

    void WidgetLayout::SetRect(const RectF& rect)
    {
        RectF parentRect = GetParentRectangle();
        RectF parentAnchoredRect(parentRect.Size()*mAnchorMin,
                                 parentRect.Size()*mAnchorMax);

        RectF localRect = rect;
        if (auto parent = mOwnerWidget->mParent.Lock())
            localRect += parentRect.Size()*parent->transform->mPivot.XY();

        mOffsetMin = localRect.LeftBottom() - parentAnchoredRect.LeftBottom();
        mOffsetMax = localRect.RightTop() - parentAnchoredRect.RightTop();

        SetDirty();
    }

    RectF WidgetLayout::GetChildrenWorldRect() const
    {
        return mChildrenWorldRect;
    }

    void WidgetLayout::SetAxisAlignedRect(const RectF& rect)
    {
        ActorTransform::SetAxisAlignedRect(rect);
        UpdateOffsetsByCurrentTransform();
    }

    void WidgetLayout::SetPivot2D(const Vec2F& pivot)
    {
        mPivot.x = pivot.x;
        mPivot.y = pivot.y;
        SetDirty();
    }

    void WidgetLayout::SetBasis(const Basis& basis)
    {
        ActorTransform::SetBasis(basis);
        UpdateOffsetsByCurrentTransform();
    }

    void WidgetLayout::SetNonSizedBasis(const Basis& basis)
    {
        ActorTransform::SetNonSizedBasis(basis);
        UpdateOffsetsByCurrentTransform();
    }

    RectF WidgetLayout::GetRect() const
    {
        RectF parentRect = GetParentRectangle();

        RectF rectangle(mOffsetMin + mAnchorMin*parentRect.Size(),
                        mOffsetMax + mAnchorMax*parentRect.Size());

        return rectangle;
    }

    void WidgetLayout::SetAnchorMin(const Vec2F& min)
    {
        mAnchorMin = min;
        SetDirty();
    }

    Vec2F WidgetLayout::GetAnchorMin() const
    {
        return mAnchorMin;
    }

    void WidgetLayout::SetAnchorMax(const Vec2F& max)
    {
        mAnchorMax = max;
        SetDirty();
    }

    Vec2F WidgetLayout::GetAnchorMax() const
    {
        return mAnchorMax;
    }

    void WidgetLayout::SetAnchorLeft(float value)
    {
        mAnchorMin.x = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorLeft() const
    {
        return mAnchorMin.x;
    }

    void WidgetLayout::SetAnchorRight(float value)
    {
        mAnchorMax.x = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorRight() const
    {
        return mAnchorMax.x;
    }

    void WidgetLayout::SetAnchorBottom(float value)
    {
        mAnchorMin.y = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorBottom() const
    {
        return mAnchorMin.y;
    }

    void WidgetLayout::SetAnchorTop(float value)
    {
        mAnchorMax.y = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorTop() const
    {
        return mAnchorMax.y;
    }

    void WidgetLayout::SetOffsetMin(const Vec2F& min)
    {
        mOffsetMin = min;
        SetDirty();
    }

    Vec2F WidgetLayout::GetOffsetMin() const
    {
        return mOffsetMin;
    }

    void WidgetLayout::SetOffsetMax(const Vec2F& max)
    {
        mOffsetMax = max;
        SetDirty();
    }

    Vec2F WidgetLayout::GetOffsetMax() const
    {
        return mOffsetMax;
    }

    void WidgetLayout::SetOffsetLeft(float value)
    {
        mOffsetMin.x = value;
        SetDirty();
    }

    float WidgetLayout::GetOffsetLeft() const
    {
        return mOffsetMin.x;
    }

    void WidgetLayout::SetoffsetRight(float value)
    {
        mOffsetMax.x = value;
        SetDirty();
    }

    float WidgetLayout::GetoffsetRight() const
    {
        return mOffsetMax.x;
    }

    void WidgetLayout::SetOffsetBottom(float value)
    {
        mOffsetMin.y = value;
        SetDirty();
    }

    float WidgetLayout::GetOffsetBottom() const
    {
        return mOffsetMin.y;
    }

    void WidgetLayout::SetOffsetTop(float value)
    {
        mOffsetMax.y = value;
        SetDirty();
    }

    float WidgetLayout::GetOffsetTop() const
    {
        return mOffsetMax.y;
    }

    void WidgetLayout::SetMinimalSize(const Vec2F& minSize)
    {
        mMinSize = minSize;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    Vec2F WidgetLayout::GetMinimalSize() const
    {
        return mMinSize;
    }

    void WidgetLayout::SetMinimalWidth(float value)
    {
        mMinSize.x = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMinWidth() const
    {
        return mMinSize.x;
    }

    void WidgetLayout::SetMinimalHeight(float value)
    {
        mMinSize.y = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMinHeight() const
    {
        return mMinSize.y;
    }

    void WidgetLayout::SetMaximalSize(const Vec2F& maxSize)
    {
        mMaxSize = maxSize;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    Vec2F WidgetLayout::GetMaximalSize() const
    {
        return mMaxSize;
    }

    void WidgetLayout::SetMaximalWidth(float value)
    {
        mMaxSize.x = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMaxWidth() const
    {
        return mMaxSize.x;
    }

    void WidgetLayout::SetMaximalHeight(float value)
    {
        mMaxSize.y = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMaxHeight() const
    {
        return mMaxSize.y;
    }

    void WidgetLayout::DisableSizeChecks()
    {
        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
        SetDirty();
    }

    void WidgetLayout::EnableSizeChecks()
    {
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
    }

    void WidgetLayout::SetWeight(const Vec2F& weight)
    {
        mWeight = weight;
        SetDirty();
    }

    Vec2F WidgetLayout::GetWeight() const
    {
        return mWeight;
    }

    void WidgetLayout::SetWidthWeight(float widthWeigth)
    {
        mWeight.x = widthWeigth;
        SetDirty();
    }

    float WidgetLayout::GetWidthWeight()
    {
        return mWeight.x;
    }

    void WidgetLayout::SetHeightWeight(float heigthWeigth)
    {
        mWeight.y = heigthWeigth;
        SetDirty();
    }

    float WidgetLayout::GetHeightWeight()
    {
        return mWeight.y;
    }

    WidgetLayout WidgetLayout::BothStretch(float borderLeft, float borderBottom, float borderRight, float borderTop)
    {
        WidgetLayout res;
        res.mAnchorMin = Vec2F(0, 0);
        res.mAnchorMax = Vec2F(1, 1);
        res.mOffsetMin = Vec2F(borderLeft, borderBottom);
        res.mOffsetMax = Vec2F(-borderRight, -borderTop);
        return res;
    }

    WidgetLayout WidgetLayout::Based(BaseCorner corner, const Vec2F& size, const Vec2F& offset)
    {
        WidgetLayout res;
        switch (corner)
        {
        case BaseCorner::Left:
        res.mAnchorMin = Vec2F(0.0f, 0.5f);
        res.mAnchorMax = Vec2F(0.0f, 0.5f);
        res.mOffsetMin = Vec2F(0.0f, -size.y*0.5f) + offset;
        res.mOffsetMax = Vec2F(size.x, size.y*0.5f) + offset;
        break;

        case BaseCorner::Right:
        res.mAnchorMin = Vec2F(1.0f, 0.5f);
        res.mAnchorMax = Vec2F(1.0f, 0.5f);
        res.mOffsetMin = Vec2F(-size.x, -size.y*0.5f) + offset;
        res.mOffsetMax = Vec2F(0.0f, size.y*0.5f) + offset;
        break;
        case BaseCorner::Top:
        res.mAnchorMin = Vec2F(0.5f, 1.0f);
        res.mAnchorMax = Vec2F(0.5f, 1.0f);
        res.mOffsetMin = Vec2F(-size.x*0.5f, -size.y) + offset;
        res.mOffsetMax = Vec2F(size.x*0.5f, 0.0f) + offset;
        break;

        case BaseCorner::Bottom:
        res.mAnchorMin = Vec2F(0.5f, 0.0f);
        res.mAnchorMax = Vec2F(0.5f, 0.0f);
        res.mOffsetMin = Vec2F(-size.x*0.5f, 0.0f) + offset;
        res.mOffsetMax = Vec2F(size.x*0.5f, size.y) + offset;
        break;

        case BaseCorner::Center:
        res.mAnchorMin = Vec2F(0.5f, 0.5f);
        res.mAnchorMax = Vec2F(0.5f, 0.5f);
        res.mOffsetMin = Vec2F(-size.x*0.5f, -size.y*0.5f) + offset;
        res.mOffsetMax = Vec2F(size.x*0.5f, size.y*0.5f) + offset;
        break;

        case BaseCorner::LeftBottom:
        res.mAnchorMin = Vec2F(0.0f, 0.0f);
        res.mAnchorMax = Vec2F(0.0f, 0.0f);
        res.mOffsetMin = Vec2F(0.0f, 0.0f) + offset;
        res.mOffsetMax = Vec2F(size.x, size.y) + offset;
        break;

        case BaseCorner::LeftTop:
        res.mAnchorMin = Vec2F(0.0f, 1.0f);
        res.mAnchorMax = Vec2F(0.0f, 1.0f);
        res.mOffsetMin = Vec2F(0.0f, -size.y) + offset;
        res.mOffsetMax = Vec2F(size.x, 0.0f) + offset;
        break;

        case BaseCorner::RightBottom:
        res.mAnchorMin = Vec2F(1.0f, 0.0f);
        res.mAnchorMax = Vec2F(1.0f, 0.0f);
        res.mOffsetMin = Vec2F(-size.x, 0.0f) + offset;
        res.mOffsetMax = Vec2F(0.0f, size.y) + offset;
        break;

        case BaseCorner::RightTop:
        res.mAnchorMin = Vec2F(1.0f, 1.0f);
        res.mAnchorMax = Vec2F(1.0f, 1.0f);
        res.mOffsetMin = Vec2F(-size.x, -size.y) + offset;
        res.mOffsetMax = Vec2F(0.0f, 0.0f) + offset;
        break;
        }

        return res;
    }

    WidgetLayout WidgetLayout::VerStretch(HorAlign align, float top, float bottom, float width, float offsX)
    {
        WidgetLayout res;
        res.mAnchorMin.y = 0.0f;
        res.mAnchorMax.y = 1.0f;
        res.mOffsetMin.y = bottom;
        res.mOffsetMax.y = -top;

        switch (align)
        {
        case HorAlign::Left:
        res.mAnchorMin.x = 0.0f;
        res.mAnchorMax.x = 0.0f;
        res.mOffsetMin.x = offsX;
        res.mOffsetMax.x = offsX + width;
        break;

        case HorAlign::Middle:
        res.mAnchorMin.x = 0.5f;
        res.mAnchorMax.x = 0.5f;
        res.mOffsetMin.x = offsX - width*0.5f;
        res.mOffsetMax.x = offsX + width*0.5f;
        break;

        case HorAlign::Right:
        res.mAnchorMin.x = 1.0f;
        res.mAnchorMax.x = 1.0f;
        res.mOffsetMin.x = -offsX - width;
        res.mOffsetMax.x = -offsX;
        break;

        default:
        break;
        }

        return res;
    }

    WidgetLayout WidgetLayout::HorStretch(VerAlign align, float left, float right, float height, float offsY)
    {
        WidgetLayout res;
        res.mAnchorMin.x = 0.0f;
        res.mAnchorMax.x = 1.0f;
        res.mOffsetMin.x = left;
        res.mOffsetMax.x = -right;

        switch (align)
        {
        case VerAlign::Top:
        res.mAnchorMin.y = 1.0f;
        res.mAnchorMax.y = 1.0f;
        res.mOffsetMin.y = -offsY - height;
        res.mOffsetMax.y = -offsY;
        break;

        case VerAlign::Middle:
        res.mAnchorMin.y = 0.5f;
        res.mAnchorMax.y = 0.5f;
        res.mOffsetMin.y = offsY - height*0.5f;
        res.mOffsetMax.y = offsY + height*0.5f;
        break;

        case VerAlign::Bottom:
        res.mAnchorMin.y = 0.0f;
        res.mAnchorMax.y = 0.0f;
        res.mOffsetMin.y = offsY;
        res.mOffsetMax.y = offsY + height;
        break;

        case VerAlign::Both:
        break;
        }

        return res;
    }

    void WidgetLayout::SetOwner(const Ref<Actor>& actor)
    {
        ActorTransform::SetOwner(actor);
        mOwnerWidget = (Widget*)actor.Get();
        SetDirty();
    }

    void WidgetLayout::SetDirty(bool fromParent /*= false*/)
    {
        if (!fromParent && mDrivenByParent && mOwnerWidget)
        {
            if (auto parent = mOwnerWidget->mParent.Lock())
                parent->transform->SetDirty(fromParent);
        }

        ActorTransform::SetDirty(fromParent);
    }

    RectF WidgetLayout::GetParentRectangle() const
    {
        if (auto parentWidget = mOwnerWidget->mParentWidget.Lock())
            return parentWidget->GetLayoutData().mChildrenWorldRect;
        else if (auto parent = mOwnerWidget->mParent.Lock())
            return parent->transform->mWorldBox.ToRect();

        return RectF();
    }

    void WidgetLayout::Update()
    {
        RectF parentWorldRect;
        Vec2F parentWorldPosition;

        if (auto parentWidget = mOwnerWidget->mParentWidget.Lock())
        {
            parentWorldRect = parentWidget->GetLayoutData().mChildrenWorldRect;

            RectF notWidgetWorldRect = parentWidget->transform->mWorldBox.ToRect();
            parentWorldPosition = notWidgetWorldRect.LeftBottom() +
                parentWidget->transform->mPivot.XY()*notWidgetWorldRect.Size();
        }
        else if (auto parent = mOwnerWidget->mParent.Lock())
        {
            parentWorldRect = parent->transform->mWorldBox.ToRect();

            parentWorldPosition = parentWorldRect.LeftBottom() +
                parent->transform->mPivot.XY()*parentWorldRect.Size();
        }

        RectF worldRectangle(parentWorldRect.LeftBottom() + mOffsetMin + mAnchorMin*parentWorldRect.Size(),
                             parentWorldRect.LeftBottom() + mOffsetMax + mAnchorMax*parentWorldRect.Size());

        mSize.x = worldRectangle.Width();
        mSize.y = worldRectangle.Height();

        Vec2F position = worldRectangle.LeftBottom() - parentWorldPosition + mSize.XY()*mPivot.XY();
        mPosition.x = position.x;
        mPosition.y = position.y;

        (this->*mCheckMinMaxFunc)();

        // editor UI stays pixel-perfect always; scene widgets follow the rounding switch
        if (mSceneLayoutsRounding || !mOwnerWidget || !mOwnerWidget->IsOnScene())
            FloorRectangle();

        UpdateLocalBox();
        UpdateTransform();
        UpdateWorldBoxAndTransform();

        mUpdateFrame = mDirtyFrame;

        if (mOwnerWidget)
        {
            mOwnerWidget->SetChildrenWorldRect(mWorldBox.ToRect());
            mOwnerWidget->OnTransformUpdated();
        }
    }

    bool WidgetLayout::mSceneLayoutsRounding = true;

    void WidgetLayout::SetSceneLayoutsRounding(bool enabled)
    {
        mSceneLayoutsRounding = enabled;
    }

    bool WidgetLayout::IsSceneLayoutsRounding()
    {
        return mSceneLayoutsRounding;
    }

    Basis WidgetLayout::GetLayoutToWorldBasis() const
    {
        RectF rect = mWorldBox.ToRect();
        Basis world = mWorldTransform.ToBasis();
        Vec2F size = rect.Size();

        // a zero-sized axis keeps unit direction, the world vector carries no information there
        Vec2F xv = size.x > FLT_EPSILON ? world.xv/size.x : Vec2F(1, 0);
        Vec2F yv = size.y > FLT_EPSILON ? world.yv/size.y : Vec2F(0, 1);
        Vec2F origin = world.origin - xv*rect.left - yv*rect.bottom;

        return Basis(origin, xv, yv);
    }

    Vec2F WidgetLayout::WorldToLayoutPoint(const Vec2F& worldPoint) const
    {
        if (IsWorldTransformPlain())
            return worldPoint;

        Basis basis = GetLayoutToWorldBasis();
        float det = basis.xv.x*basis.yv.y - basis.xv.y*basis.yv.x;
        if (Math::Abs(det) < FLT_EPSILON) // collapsed axis (zero scale): nothing maps back
            return Vec2F(FLT_MAX, FLT_MAX);

        return basis.Inverted().Transform(worldPoint);
    }

    bool WidgetLayout::IsPointInside(const Vec2F& point) const
    {
        return mWorldBox.ToRect().IsInside(WorldToLayoutPoint(point));
    }

    bool WidgetLayout::IsWorldTransformPlain() const
    {
        RectF rect = mWorldBox.ToRect();
        Basis world = mWorldTransform.ToBasis();
        const float eps = 0.001f;

        return Math::Equals(world.origin.x, rect.left, eps) && Math::Equals(world.origin.y, rect.bottom, eps) &&
            Math::Equals(world.xv.x, rect.Width(), eps) && Math::Equals(world.xv.y, 0.0f, eps) &&
            Math::Equals(world.yv.x, 0.0f, eps) && Math::Equals(world.yv.y, rect.Height(), eps);
    }

    void WidgetLayout::FloorRectangle()
    {
        mSize.x = Math::Round(mSize.x);
        mSize.y = Math::Round(mSize.y);
        mPosition.x = Math::Round(mPosition.x);
        mPosition.y = Math::Round(mPosition.y);
    }

    void WidgetLayout::UpdateOffsetsByCurrentTransform()
    {
        Vec2F offs;

        if (auto parentWidget = mOwnerWidget->mParentWidget.Lock())
        {
            offs = parentWidget->GetLayoutData().mChildrenWorldRect.LeftBottom() -
                parentWidget->GetLayoutData().mWorldBox.ToRect().LeftBottom();
        }

        SetRect(ActorTransform::GetRect() - offs);
    }

    void WidgetLayout::CopyFrom(const ActorTransform& other)
    {
        const WidgetLayout* otherLayout = dynamic_cast<const WidgetLayout*>(&other);

        if (otherLayout)
        {
            mAnchorMin = otherLayout->mAnchorMin;
            mAnchorMax = otherLayout->mAnchorMax;
            mOffsetMin = otherLayout->mOffsetMin;
            mOffsetMax = otherLayout->mOffsetMax;
            mMinSize = otherLayout->mMinSize;
            mMaxSize = otherLayout->mMaxSize;
            mWeight = otherLayout->mWeight;

            mCheckMinMaxFunc = otherLayout->mCheckMinMaxFunc;
        }

        ActorTransform::CopyFrom(other);
    }

    void WidgetLayout::CheckMinMax()
    {
        Vec2F resSize = mSize.XY();
        Vec2F minSizeWithChildren(mOwnerWidget->GetMinWidthWithChildren(), mOwnerWidget->GetMinHeightWithChildren());

        Vec2F clampSize(Math::Clamp(resSize.x, minSizeWithChildren.x, mMaxSize.x),
                        Math::Clamp(resSize.y, minSizeWithChildren.y, mMaxSize.y));

        Vec2F szDelta = clampSize - resSize;

        if (szDelta != Vec2F())
        {
            mSize.x += szDelta.x;
            mSize.y += szDelta.y;
        }
    }

    void WidgetLayout::DontCheckMinMax()
    {}

    bool WidgetLayout::IsSerializeEnabled() const
    {
        return false;
    }

    void WidgetLayout::OnSerialize(DataValue& node) const
    {
        ActorTransform::OnSerialize(node);

        auto serialize = [&node](const char* name, const Vec2F& value, const Vec2F& defaultValue) {
            if (value != defaultValue)
                node.AddMember(name).Set(value);
        };

        serialize("anchorMin", mAnchorMin, Vec2F(0, 0));
        serialize("anchorMax", mAnchorMax, Vec2F(0, 0));
        serialize("offsetMin", mOffsetMin, Vec2F(0, 0));
        serialize("offsetMax", mOffsetMax, Vec2F(10, 10));
        serialize("minSize", mMinSize, Vec2F(0, 0));
        serialize("maxSize", mMaxSize, Vec2F(10000, 10000));
        serialize("weight", mWeight, Vec2F(1, 1));
    }

    void WidgetLayout::OnDeserialized(const DataValue& node)
    {
        auto deserialize = [&node](const char* name, Vec2F& value) {
            if (auto member = node.FindMember(name))
                member->Get(value);
        };

        deserialize("anchorMin", mAnchorMin);
        deserialize("anchorMax", mAnchorMax);
        deserialize("offsetMin", mOffsetMin);
        deserialize("offsetMax", mOffsetMax);
        deserialize("minSize", mMinSize);
        deserialize("maxSize", mMaxSize);
        deserialize("weight", mWeight);

        ActorTransform::OnDeserialized(node);
    }

    void WidgetLayout::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        ActorTransform::OnSerializeDelta(node, origin);

        auto& other = dynamic_cast<const WidgetLayout&>(origin);
        auto serialize = [&node](const char* name, const Vec2F& value, const Vec2F& originValue) {
            if (!EqualsForDeltaSerialize(value, originValue))
                node.AddMember(name).Set(value);
        };

        serialize("anchorMin", mAnchorMin, other.mAnchorMin);
        serialize("anchorMax", mAnchorMax, other.mAnchorMax);
        serialize("offsetMin", mOffsetMin, other.mOffsetMin);
        serialize("offsetMax", mOffsetMax, other.mOffsetMax);
        serialize("minSize", mMinSize, other.mMinSize);
        serialize("maxSize", mMaxSize, other.mMaxSize);
        serialize("weight", mWeight, other.mWeight);
    }

    void WidgetLayout::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        auto& other = dynamic_cast<const WidgetLayout&>(origin);
        auto deserialize = [&node](const char* name, Vec2F& value, const Vec2F& originValue) {
            if (auto member = node.FindMember(name); member && !member->IsNull())
                member->Get(value);
            else
                value = originValue;
        };

        deserialize("anchorMin", mAnchorMin, other.mAnchorMin);
        deserialize("anchorMax", mAnchorMax, other.mAnchorMax);
        deserialize("offsetMin", mOffsetMin, other.mOffsetMin);
        deserialize("offsetMax", mOffsetMax, other.mOffsetMax);
        deserialize("minSize", mMinSize, other.mMinSize);
        deserialize("maxSize", mMaxSize, other.mMaxSize);
        deserialize("weight", mWeight, other.mWeight);

        ActorTransform::OnDeserializedDelta(node, origin);
    }

#if IS_SCRIPTING_SUPPORTED
    void WidgetLayout::Set(const WidgetLayout& other)
    {
        *this = other;
    }
#endif

    Vector<float> CalculateExpandedSize(Vector<Ref<Widget>>& widgets, bool horizontal, float availableWidth, float spacing)
    {
        Vector<float> minSizes; minSizes.Reserve(widgets.Count());
        Vector<float> maxSizes; maxSizes.Reserve(widgets.Count());
        Vector<float> weights; weights.Reserve(widgets.Count());

        float minSizesSum = 0;
        float weightsSum = 0;

        for (auto itChild = widgets.begin(); itChild != widgets.end();)
        {
            if (!(*itChild)->IsEnabledInHierarchy())
                itChild = widgets.erase(itChild);
            else
            {
                auto child = *itChild;

                float minWidth = horizontal ? child->layout->GetMinWidth() : child->layout->GetMinHeight();
                float maxWidth = horizontal ? child->layout->GetMaxWidth() : child->layout->GetMaxHeight();
                float weight = horizontal ? child->layout->GetWidthWeight() : child->layout->GetHeightWeight();

                minSizesSum += minWidth;

                if (minWidth < maxWidth)
                    weightsSum += weight;

                minSizes.Add(minWidth);
                maxSizes.Add(maxWidth);
                weights.Add(weight);

                ++itChild;
            }
        }

    Vector<float> widths = minSizes;

    int childCount = widgets.Count();

    availableWidth -= (spacing*(float)Math::Max(0, widgets.Count() - 1));
    float expandWidth = availableWidth - minSizesSum;
    while (expandWidth > 0)
    {
        float currentExpand = expandWidth;
        float invWeightsSum = 1.0f/weightsSum;

        for (int i = 0; i < childCount; i++)
        {
            if (widths[i] < maxSizes[i])
            {
                float expand = currentExpand*weights[i]*invWeightsSum;
                float maxExpand = maxSizes[i] - widths[i];

                if (expand > maxExpand)
                {
                    float coef = maxExpand/expand;
                    currentExpand *= coef;
                }
            }
        }

        for (int i = 0; i < childCount; i++)
        {
            if (widths[i] < maxSizes[i])
            {
                widths[i] += currentExpand*weights[i]*invWeightsSum;

                if (widths[i] >= maxSizes[i])
                    weightsSum -= weights[i];
            }
        }

        expandWidth -= currentExpand;
    }

    return widths;
}

}
// --- META ---

DECLARE_CLASS(o2::WidgetLayout, o2__WidgetLayout);
// --- END META ---
