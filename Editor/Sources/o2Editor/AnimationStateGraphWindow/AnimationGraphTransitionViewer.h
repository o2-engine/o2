#pragma once

#include "o2/Animation/AnimationStateGraph.h"
#include "o2/Events/EventSystem.h"
#include "o2Editor/AnimationStateGraphWindow/AnimationStateGraphEditor.h"
#include "o2Editor/Core/Properties/Objects/DefaultObjectPropertiesViewer.h"

namespace o2
{
    class Widget;
    class Spoiler;
    class DragHandle;
}

namespace Editor
{
    // ---------------------------------------
    // AnimationGraphTransition properties viewer
    // ---------------------------------------
    class AnimationGraphTransitionViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(AnimationGraphTransitionViewer);

	private:
		WeakRef<AnimationStateGraphEditor::StateTransition> mTransition; // Current transition

        Ref<Widget> mDurationWidget; // Widget for displaying durations

		Ref<Sprite> mSourceRangeSprite;      // Source range sprite
		Ref<Sprite> mDestinationRangeSprite; // Destination range sprite

		Ref<DragHandle> mBeginTimeRangeHandle; // Handle for begin time range
		Ref<DragHandle> mEndTimeRangeHandle;   // Handle for end time range 
		Ref<DragHandle> mDurationHandle;       // Handle for duration

		float mWidgetWidth = 0.0f; // Width of duration widget

		float mSourceAnimationBeginPosition = 0.0f; // Source animation begin position in widget
		float mSourceAnimationEndPosition = 0.0f;   // Source animation end position in widget

		float mDestinationAnimationBeginPosition = 0.0f; // Destination animation begin position in widget
		float mDestinationAnimationEndPosition = 0.0f;   // Destination animation end position in widget

		float mSourceDuration = 1.0f;      // Source animation duration
		float mDestinationDuration = 1.0f; // Destination animation duration

	private:
		// Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
		void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjects) override;

        // Draw duration widget
        void DrawDurationWidget();

        // Update handle positions
        void UpdateHandlesPositions();

        // Handle position changed callbacks
        void OnBeginTimeRangeChanged(const Vec2F& position);
        void OnEndTimeRangeChanged(const Vec2F& position);
        void OnDurationChanged(const Vec2F& position);
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationGraphTransitionViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationGraphTransitionViewer)
{
    FIELD().PRIVATE().NAME(mTransition);
    FIELD().PRIVATE().NAME(mDurationWidget);
    FIELD().PRIVATE().NAME(mSourceRangeSprite);
    FIELD().PRIVATE().NAME(mDestinationRangeSprite);
    FIELD().PRIVATE().NAME(mBeginTimeRangeHandle);
    FIELD().PRIVATE().NAME(mEndTimeRangeHandle);
    FIELD().PRIVATE().NAME(mDurationHandle);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mWidgetWidth);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mSourceAnimationBeginPosition);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mSourceAnimationEndPosition);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mDestinationAnimationBeginPosition);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mDestinationAnimationEndPosition);
    FIELD().PRIVATE().DEFAULT_VALUE(1.0f).NAME(mSourceDuration);
    FIELD().PRIVATE().DEFAULT_VALUE(1.0f).NAME(mDestinationDuration);
}
END_META;
CLASS_METHODS_META(Editor::AnimationGraphTransitionViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PRIVATE().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PRIVATE().SIGNATURE(void, DrawDurationWidget);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateHandlesPositions);
    FUNCTION().PRIVATE().SIGNATURE(void, OnBeginTimeRangeChanged, const Vec2F&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnEndTimeRangeChanged, const Vec2F&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDurationChanged, const Vec2F&);
}
END_META;
// --- END META ---
