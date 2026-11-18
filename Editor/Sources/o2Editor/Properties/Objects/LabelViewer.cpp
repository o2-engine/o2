#include "o2Editor/stdafx.h"
#include "LabelViewer.h"

#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2Editor/Properties/Properties.h"

namespace Editor
{
    const Ref<TextPropertiesBlock>& LabelViewer::GetTextBlock() const
    {
        return mTextBlock;
    }

    void LabelViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        const Type& labelType = TypeOf(Label);
        const Type* realType = targetObjets.IsEmpty() ? &labelType : &targetObjets[0].first->GetType();

        TextFieldNames names;
        names.ownFontStyle = "fontStyle";
        names.color = "color";
        names.horOverflow = "horOverflow";
        names.verOverflow = "verOverflow";
        names.expandBorder = "expandBorder";

        // The widget's own fields first, as the default viewer has them
        auto textFieldNames = { "text", "font", "fontAsset", "fontStyle", "fontStyleAsset", "height", "color", "verAlign",
                                "horAlign", "horOverflow", "verOverflow", "expandBorder", "symbolsDistanceCoef",
                                "linesDistanceCoef" };

        Vector<const FieldInfo*> otherFields;
        for (auto field : realType->GetFieldsWithBaseClasses())
        {
            bool isText = false;
            for (auto name : textFieldNames)
            {
                if (field->GetName() == name && field->GetOwnerType() == &labelType)
                    isText = true;
            }

            if (!isText)
                otherFields.Add(field);
        }

        o2EditorProperties.BuildObjectProperties(mSpoiler, otherFields, mPropertiesContext, "", mOnPropertyChangeCompleted,
                                                 mOnPropertyChanged);

        mTextBlock = mmake<TextPropertiesBlock>();
        mTextBlock->Build(mSpoiler, labelType, names, mPropertiesContext, mOnPropertyChangeCompleted, mOnPropertyChanged);
    }

    void LabelViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        TObjectPropertiesViewer<Label>::OnRefreshed(targetObjets);

        if (mTextBlock)
            mTextBlock->Refresh();
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::Label>);
// --- META ---

DECLARE_CLASS(Editor::LabelViewer, Editor__LabelViewer);
// --- END META ---
