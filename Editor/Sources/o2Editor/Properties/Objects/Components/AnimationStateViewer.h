#pragma once

#include "o2/Animation/AnimationState.h"
#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Toggle;
    class Button;
    class HorizontalProgress;
}

namespace Editor
{
    // ------------------------------------
    // AnimationComponent properties viewer
    // ------------------------------------
    class AnimationStateViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Creates spoiler for properties
        Ref<Spoiler> CreateSpoiler(const Ref<Widget>& parent) override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(AnimationStateViewer);

    private:
        Ref<Toggle> mPlayPause;
        Ref<Button> mEditBtn;
        Ref<Toggle> mLooped;

        Ref<HorizontalProgress> mTimeProgress;

        WeakRef<IAnimation> mSubscribedPlayer;

    private:
        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // ThCalled when the viewer is freed
        void OnFree() override;

        // Called when play pause toggled
        void OnPlayPauseToggled(bool play);

        // Called when loop toggled
        void OnLoopToggled(bool looped);

        // Called when edit button pressed, sets animation editing
        void OnEditPressed();

        // Called when time progress changed by user, sets subscribed player time 
        void OnTimeProgressChanged(float value);

        // Called when animation updates
        void OnAnimationUpdated(float time);
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationStateViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateViewer)
{
    FIELD().PRIVATE().NAME(mPlayPause);
    FIELD().PRIVATE().NAME(mEditBtn);
    FIELD().PRIVATE().NAME(mLooped);
    FIELD().PRIVATE().NAME(mTimeProgress);
    FIELD().PRIVATE().NAME(mSubscribedPlayer);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Spoiler>, CreateSpoiler, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp1);
    FUNCTION().PRIVATE().SIGNATURE(void, OnFree);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPlayPauseToggled, bool);
    FUNCTION().PRIVATE().SIGNATURE(void, OnLoopToggled, bool);
    FUNCTION().PRIVATE().SIGNATURE(void, OnEditPressed);
    FUNCTION().PRIVATE().SIGNATURE(void, OnTimeProgressChanged, float);
    FUNCTION().PRIVATE().SIGNATURE(void, OnAnimationUpdated, float);
}
END_META;
// --- END META ---
