#pragma once

#include "o2/Assets/Types/AnimationStateGraphAsset.h"
#include "o2Editor/Core/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Button;
}

namespace Editor
{
    // -----------------------
    // Editor animation viewer
    // -----------------------
    class AnimationStateGraphViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(AnimationStateGraphViewer);

    private:
        Ref<Button> mEditButton; // Edit button, opens animation state graph editor window

        Ref<AnimationStateGraphAsset> mCurrentAnimation; // Current animation

    private:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when edit button has pressed, opens animation editor window
        void OnEditPressed();
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationStateGraphViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateGraphViewer)
{
    FIELD().PRIVATE().NAME(mEditButton);
    FIELD().PRIVATE().NAME(mCurrentAnimation);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PRIVATE().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PRIVATE().SIGNATURE(void, OnEditPressed);
}
END_META;
// --- END META ---
