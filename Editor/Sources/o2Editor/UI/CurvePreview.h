#pragma once

#include "o2/Scene/UI/Widget.h"

using namespace o2;

namespace o2
{
    class Curve;
    class Sprite;
}

namespace Editor
{
    // -------------------
    // Curve preview image
    // -------------------
    class CurvePreview: public Widget
    {
    public:
        // Default constructor. Initializes image
        CurvePreview(RefCounter* refCounter);

        // Default copy-constructor
        CurvePreview(RefCounter* refCounter, const CurvePreview& other);

        // Copy operator
        CurvePreview& operator=(const CurvePreview& other);

        // Sets viewing curve
        void SetCurve(const Ref<Curve>& curve);

        // Draws widget
        void Draw() override;

        // Sets back color of image
        void SetBackColor(const Color4& color);

        // Returns back color of image
        Color4 GetBackColor() const;

        // Sets curve color on image
        void SetCurveColor(const Color4& color);

        // Returns curve color on image
        Color4 GetCurveColor() const;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(CurvePreview);
        CLONEABLE_REF(CurvePreview);

    protected:
        Ref<Curve> mCurve; // Viewing curve

        bool mNeedRedraw = false; // Need redraw flag

        Ref<Sprite> mSprite; // Image sprite, used for drawing
        Ref<Mesh>   mMesh;   // Curves range mesh, used for drawing

        Color4 mBackColor = Color4(225, 232, 232); // Background color
        Color4 mCurveColor = Color4(44, 62, 80);   // Curve color

    protected:
        // Updates layers layouts, calls after updating widget layout
        void UpdateLayersLayouts() override;

        // Redraws image texture
        void Redraw();

        // Called when curve keys changed, sets need redraw to true
        void OnCurveChanged();
    };
}
// --- META ---

CLASS_BASES_META(Editor::CurvePreview)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::CurvePreview)
{
    FIELD().PROTECTED().NAME(mCurve);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedRedraw);
    FIELD().PROTECTED().NAME(mSprite);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(225, 232, 232)).NAME(mBackColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(44, 62, 80)).NAME(mCurveColor);
}
END_META;
CLASS_METHODS_META(Editor::CurvePreview)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CurvePreview&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurve, const Ref<Curve>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBackColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetBackColor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurveColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetCurveColor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, Redraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveChanged);
}
END_META;
// --- END META ---
