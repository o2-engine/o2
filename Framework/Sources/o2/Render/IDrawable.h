#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class Material;

    // -------------------------------
    // Basic drawable object interface
    // -------------------------------
    class IDrawable
    {
    public:
        Function<void()> onDraw; // Draw event

    public:
        virtual ~IDrawable();

        // Draws drawable
        virtual void Draw();

        // Returns true if point is under drawable
        virtual bool IsUnderPoint(const Vec2F& point);

        // Sets material for rendering. Pass nullptr for default material. Clears material asset (in derived classes that support it).
        virtual void SetMaterial(const Ref<Material>& material);

        // Returns current material (direct override; may be null)
        virtual Ref<Material> GetMaterial() const;

    protected:
        RectF         mDrawingScissorRect; // Scissor rectangle at last drawing
        Ref<Material> mMaterial;           // Direct material override

    protected:
        // Called when drawable was drawn. Storing render scissor rect, calling onDraw callback
        virtual void OnDrawn();

        // Called when material or material asset was changed (by user or code)
        virtual void OnMaterialChanged() {}
    };
}
