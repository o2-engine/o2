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

        mColorProperty = o2EditorProperties.BuildFieldType<ColorProperty>(mSpoiler, textType, "color", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mAlphaProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "transparency", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mFontProperty = o2EditorProperties.BuildFieldType<AssetProperty>(mSpoiler, textType, "fontAsset", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mTextProperty = o2EditorProperties.BuildFieldType<WStringProperty>(mSpoiler, textType, "text", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mHeightProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "height", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mVerAlignProperty = o2EditorProperties.BuildFieldType<EnumProperty>(mSpoiler, textType, "verAlign", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mHorAlignProperty = o2EditorProperties.BuildFieldType<EnumProperty>(mSpoiler, textType, "horAlign", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mWordWrapProperty = o2EditorProperties.BuildFieldType<BooleanProperty>(mSpoiler, textType, "wordWrap", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mDotsEndingsProperty = o2EditorProperties.BuildFieldType<BooleanProperty>(mSpoiler, textType, "dotsEngings", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mSymbolsDistCoefProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "symbolsDistanceCoef", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
        mLinesDistCoefProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mSpoiler, textType, "linesDistanceCoef", "", mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::Text>);
// --- META ---

DECLARE_CLASS(Editor::TextViewer, Editor__TextViewer);
// --- END META ---
