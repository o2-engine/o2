#include "o2/stdafx.h"
#include "IDrawable.h"

#include "o2/Render/Material.h"
#include "Render.h"

namespace o2
{
    IDrawable::~IDrawable()
    {}

    void IDrawable::Draw()
    {
        OnDrawn();
    }

    bool IDrawable::IsUnderPoint(const Vec2F& point)
    {
        return false;
    }

    void IDrawable::SetMaterial(const Ref<Material>& material)
    {
        mMaterial = material;
        OnMaterialChanged();
    }

    Ref<Material> IDrawable::GetMaterial() const
    {
        return mMaterial;
    }

    void IDrawable::OnDrawn()
    {
        mDrawingScissorRect = o2Render.GetResScissorRect();
        onDraw();
    }
}
