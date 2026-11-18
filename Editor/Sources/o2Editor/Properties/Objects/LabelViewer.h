#pragma once

#include "o2/Scene/UI/Widgets/Label.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Objects/TextPropertiesBlock.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------------------------------------------------
    // Label viewer: the text settings as icon buttons, toggles and menus (TextPropertiesBlock), then the rest of the
    // fields of the label's type as the default viewer shows them
    // -----------------------------------------------------------------------------------------------------------
    class LabelViewer: public TObjectPropertiesViewer<Label>
    {
    public:
        // Returns the text settings
        const Ref<TextPropertiesBlock>& GetTextBlock() const;

        IOBJECT(LabelViewer);

    protected:
        Ref<TextPropertiesBlock> mTextBlock; // Text settings

    protected:
        // Builds the text settings and the other fields
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Updates the rows depending on the values
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::LabelViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<Label>);
}
END_META;
CLASS_FIELDS_META(Editor::LabelViewer)
{
    FIELD().PROTECTED().NAME(mTextBlock);
}
END_META;
CLASS_METHODS_META(Editor::LabelViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Ref<TextPropertiesBlock>&, GetTextBlock);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
}
END_META;
// --- END META ---
