#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Text.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"

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
    FORWARD_CLASS_REF(EnumProperty);
    FORWARD_CLASS_REF(WStringProperty);
    FORWARD_CLASS_REF(BooleanProperty);

    // --------------------
    // Editor text property
    // --------------------
    class TextViewer : public TObjectPropertiesViewer<Text>
    {
    public:
        IOBJECT(TextViewer);

    protected:
        Ref<ColorProperty>   mColorProperty;
        Ref<FloatProperty>   mAlphaProperty;
        Ref<AssetProperty>   mFontProperty;
        Ref<WStringProperty> mTextProperty;
        Ref<FloatProperty>   mHeightProperty;
        Ref<EnumProperty>    mHorAlignProperty;
        Ref<EnumProperty>    mVerAlignProperty;
        Ref<BooleanProperty> mWordWrapProperty;
        Ref<BooleanProperty> mDotsEndingsProperty;
        Ref<FloatProperty>   mSymbolsDistCoefProperty;
        Ref<FloatProperty>   mLinesDistCoefProperty;

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
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
    FIELD().PROTECTED().NAME(mFontProperty);
    FIELD().PROTECTED().NAME(mTextProperty);
    FIELD().PROTECTED().NAME(mHeightProperty);
    FIELD().PROTECTED().NAME(mHorAlignProperty);
    FIELD().PROTECTED().NAME(mVerAlignProperty);
    FIELD().PROTECTED().NAME(mWordWrapProperty);
    FIELD().PROTECTED().NAME(mDotsEndingsProperty);
    FIELD().PROTECTED().NAME(mSymbolsDistCoefProperty);
    FIELD().PROTECTED().NAME(mLinesDistCoefProperty);
}
END_META;
CLASS_METHODS_META(Editor::TextViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
}
END_META;
// --- END META ---
