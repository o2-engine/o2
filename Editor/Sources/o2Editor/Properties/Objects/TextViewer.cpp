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
    void TextViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        const Type& textType = TypeOf(Text);

        mColorProperty = o2EditorProperties.BuildFieldType<ColorProperty>(mSpoiler, textType, "color", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mAlphaProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "transparency", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mMaterialProperty = o2EditorProperties.BuildFieldType<AssetProperty>(mSpoiler, textType, "material", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mFontProperty = o2EditorProperties.BuildFieldType<AssetProperty>(mSpoiler, textType, "fontAsset", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mFontStyleProperty = o2EditorProperties.BuildFieldType<AssetProperty>(mSpoiler, textType, "fontStyleAsset", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mTextProperty = o2EditorProperties.BuildFieldType<WStringProperty>(mSpoiler, textType, "text", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mHeightProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "height", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mVerAlignProperty = o2EditorProperties.BuildFieldType<EnumProperty>(mSpoiler, textType, "verAlign", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mHorAlignProperty = o2EditorProperties.BuildFieldType<EnumProperty>(mSpoiler, textType, "horAlign", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mWordWrapProperty = o2EditorProperties.BuildFieldType<BooleanProperty>(mSpoiler, textType, "wordWrap", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mDotsEndingsProperty = o2EditorProperties.BuildFieldType<BooleanProperty>(mSpoiler, textType, "dotsEngings", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mSymbolsDistCoefProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "symbolsDistanceCoef", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
        mLinesDistCoefProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "linesDistanceCoef", "", mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::Text>);
// --- META ---

DECLARE_CLASS(Editor::TextViewer, Editor__TextViewer);
// --- END META ---
