#include "o2Editor/stdafx.h"
#include "TextViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/ColorProperty.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/WStringProperty.h"
#include "o2Editor/Properties/Properties.h"

namespace Editor
{
    const Ref<TextPropertiesBlock>& TextViewer::GetTextBlock() const
    {
        return mTextBlock;
    }

    void TextViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        const Type& textType = TypeOf(Text);

        mColorProperty = o2EditorProperties.BuildFieldType<ColorProperty>(mSpoiler, textType, "color", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mAlphaProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "transparency", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mMaterialProperty = o2EditorProperties.BuildFieldType<AssetProperty>(mSpoiler, textType, "material", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);

        TextFieldNames names;
        names.wordWrap = "wordWrap";
        names.dotsEndings = "dotsEngings";

        mTextBlock = mmake<TextPropertiesBlock>();
        mTextBlock->Build(mSpoiler, textType, names, mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
    }

    void TextViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        TObjectPropertiesViewer<Text>::OnRefreshed(targetObjets);

        if (mTextBlock)
            mTextBlock->Refresh();
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::Text>);
// --- META ---

DECLARE_CLASS(Editor::TextViewer, Editor__TextViewer);
// --- END META ---
