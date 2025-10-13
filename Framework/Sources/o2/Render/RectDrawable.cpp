#include "o2/stdafx.h"
#include "RectDrawable.h"

namespace o2
{
    IRectDrawable::IRectDrawable(const Vec2F& size /*= Vec2F()*/, const Vec2F& position /*= Vec2F()*/,
                                 float angle /*= 0.0f*/, const Vec2F& scale /*= Vec2F(1.0f, 1.0f)*/,
                                 const Color4& color /*= Color4::White()*/, const Vec2F& pivot /*= Vec2F()*/):
        Transform(size, position, angle, scale, pivot), mColor(color)
    {}

    IRectDrawable::IRectDrawable(const IRectDrawable& other) :
        Transform(other.mSize, other.mPosition, other.mAngle, other.mScale, other.mPivot),
        mBlendMode(other.mBlendMode), mColor(other.mColor), mEnabled(other.mEnabled)
    {}

    IRectDrawable& IRectDrawable::operator=(const IRectDrawable& other)
    {
        Transform::operator=(other);

		mBlendMode = other.mBlendMode;
        mColor = other.mColor;
        mEnabled = other.mEnabled;

        OnColorChanged();
        OnEnableChanged();

        return *this;
    }

    bool IRectDrawable::operator==(const IRectDrawable& other) const
    {
        return Transform::operator==(other) && mColor == other.mColor && mEnabled == other.mEnabled && mBlendMode == other.mBlendMode;
    }

    bool IRectDrawable::operator!=(const IRectDrawable& other) const
    {
        return !operator==(other);
    }

    void IRectDrawable::SetColor(const Color4& color)
    {
        mColor = color;
        UpdateColor();
    }

    Color4 IRectDrawable::GetColor() const
    {
        return mColor;
    }

    void IRectDrawable::SetOverrideColor(const Color4 &color)
    {
        mOverrideColor = color;
        UpdateColor();
    }

    Color4 IRectDrawable::GetOverrideColor() const
    {
        return mOverrideColor;
    }

    void IRectDrawable::SetTransparency(float transparency)
    {
        mColor.SetAF(transparency);
        UpdateColor();
    }

    float IRectDrawable::GetTransparency() const
    {
        return mColor.AF();
    }

    void IRectDrawable::SetBlendMode(BlendMode blendMode)
    {
        mBlendMode = blendMode;
        OnBlendModeChanged();
    }

    BlendMode IRectDrawable::GetBlendMode() const
    {
        return mBlendMode;
    }

    void IRectDrawable::SetEnabled(bool enabled)
    {
        mEnabled = enabled;
        OnEnableChanged();
    }

    bool IRectDrawable::IsEnabled() const
    {
        return mEnabled;
    }

    bool IRectDrawable::IsUnderPoint(const Vec2F& point)
    {
        return mDrawingScissorRect.IsInside(point) && Transform::IsPointInside(point);
    }

    void IRectDrawable::UpdateColor()
    {
        mResultColor = mColor * mOverrideColor;
        OnColorChanged();
    }

    FunctionalRectDrawable::FunctionalRectDrawable(const Function<void(const Basis& transform, const Color4& color)>& draw, 
                                                   const Vec2F& size /*= Vec2F()*/, const Vec2F& position /*= Vec2F()*/, 
                                                   float angle /*= 0.0f*/, const Vec2F& scale /*= Vec2F(1.0f, 1.0f)*/, 
                                                   const Color4& color /*= Color4::White()*/, const Vec2F& pivot /*= Vec2F(0.5f, 0.5f)*/):
        IRectDrawable(size, position, angle, scale, color, pivot), draw(draw)
    {}

    FunctionalRectDrawable::FunctionalRectDrawable(const FunctionalRectDrawable& other):
        IRectDrawable(other), draw(other.draw)
    {}

    FunctionalRectDrawable::FunctionalRectDrawable() = default;

    void FunctionalRectDrawable::Draw()
    {
        if (!mEnabled)
            return;

        draw(GetBasis(), mColor);

        OnDrawn();
    }

}
// --- META ---

DECLARE_CLASS(o2::IRectDrawable, o2__IRectDrawable);

DECLARE_CLASS(o2::FunctionalRectDrawable, o2__FunctionalRectDrawable);
// --- END META ---
