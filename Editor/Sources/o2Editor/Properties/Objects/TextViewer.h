#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Text.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Objects/TextPropertiesBlock.h"

using namespace o2;

namespace o2
{
    class Text;
    class Spoiler;
}

namespace Editor
{
    FORWARD_CLASS_REF(AssetProperty);
    FORWARD_CLASS_REF(FloatProperty);
    FORWARD_CLASS_REF(ColorProperty);

    // ------------------------------------------------------------------------------------------------------------
    // Text drawable viewer: colour, transparency and material, then the text settings as icon buttons, toggles and
    // menus (TextPropertiesBlock)
    // ------------------------------------------------------------------------------------------------------------
    class TextViewer : public TObjectPropertiesViewer<Text>
    {
    public:
        // Returns the text settings
        const Ref<TextPropertiesBlock>& GetTextBlock() const;

        IOBJECT(TextViewer);

    protected:
        Ref<ColorProperty>       mColorProperty;
        Ref<FloatProperty>       mAlphaProperty;
        Ref<AssetProperty>       mMaterialProperty;
        Ref<TextPropertiesBlock> mTextBlock; // Text settings

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Updates the rows depending on the values
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::TextViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<Text>);
}
END_META;
CLASS_FIELDS_META(Editor::TextViewer)
{
    FIELD().PROTECTED().NAME(mColorProperty);
    FIELD().PROTECTED().NAME(mAlphaProperty);
    FIELD().PROTECTED().NAME(mMaterialProperty);
    FIELD().PROTECTED().NAME(mTextBlock);
}
END_META;
CLASS_METHODS_META(Editor::TextViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Ref<TextPropertiesBlock>&, GetTextBlock);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
}
END_META;
// --- END META ---
