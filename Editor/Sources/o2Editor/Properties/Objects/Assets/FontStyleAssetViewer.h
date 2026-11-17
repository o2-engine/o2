#pragma once

#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class FontStyleAsset;
}

namespace Editor
{
    FORWARD_CLASS_REF(FontSampleView);

    // ------------------------------------------------------------------------------------------
    // Font style asset viewer: lines of text of a few sizes drawn with the style, over its effects
    // ------------------------------------------------------------------------------------------
    class FontStyleAssetViewer: public DefaultObjectPropertiesViewer
    {
    public:
        // Destructor
        ~FontStyleAssetViewer();

        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        // Returns the font the samples of the styles are drawn with
        static Ref<Font> GetSampleFont();

        IOBJECT(FontStyleAssetViewer);

    protected:
        Ref<FontSampleView> mSample; // Lines of text drawn with the style
        WeakRef<FontStyleAsset> mStyle;  // Shown style

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when the effects of the style changed: an effect that paints the glyphs changes the colour of the lines
        void OnStyleChanged();

        // Stops listening to the shown style
        void Unsubscribe();
    };
}
// --- META ---

CLASS_BASES_META(Editor::FontStyleAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::FontStyleAssetViewer)
{
    FIELD().PROTECTED().NAME(mSample);
    FIELD().PROTECTED().NAME(mStyle);
}
END_META;
CLASS_METHODS_META(Editor::FontStyleAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<Font>, GetSampleFont);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStyleChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, Unsubscribe);
}
END_META;
// --- END META ---
