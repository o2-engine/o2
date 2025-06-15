#pragma once
#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

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
    class AnimationViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(AnimationViewer);

    private:
        Ref<Button>        mEditButton;       // Edit button, opens animation editor window
        Ref<AnimationClip> mCurrentAnimation; // Current animation

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

CLASS_BASES_META(Editor::AnimationViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationViewer)
{
    FIELD().PRIVATE().NAME(mEditButton);
    FIELD().PRIVATE().NAME(mCurrentAnimation);
}
END_META;
CLASS_METHODS_META(Editor::AnimationViewer)
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
