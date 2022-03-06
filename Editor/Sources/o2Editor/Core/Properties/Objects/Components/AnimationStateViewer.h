#pragma once

#include "o2/Animation/AnimationState.h"
#include "o2Editor/Core/Properties/Objects/DefaultObjectPropertiesViewer.h"

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

		// Returns viewing objects base type by static function
		static const Type* GetViewingObjectTypeStatic();

		IOBJECT(AnimationStateViewer);

	private:
		Toggle* mPlayPause = nullptr;
		Button* mEditBtn = nullptr;
		Toggle* mLooped = nullptr;

		HorizontalProgress* mTimeProgress = nullptr;

		AnimationPlayer* mSubscribedPlayer = nullptr;

	private:
		// Creates spoiler for properties
		Spoiler* CreateSpoiler() override;

		// It is called when viewer is refreshed
		void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

		// This is called when the viewer is freed
		void OnFree() override;

		// It is called when play pause toggled
		void OnPlayPauseToggled(bool play);

		// It is called when loop toggled
		void OnLoopToggled(bool looped);

		// It is called when edit button pressed, sets animation editing
		void OnEditPressed();

		// It is called when time progress changed by user, sets subscribed player time 
		void OnTimeProgressChanged(float value);

		// It is called when animation updates
		void OnAnimationUpdated(float time);
	};
}

CLASS_BASES_META(Editor::AnimationStateViewer)
{
	BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateViewer)
{
	FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mPlayPause);
	FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mEditBtn);
	FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mLooped);
	FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mTimeProgress);
	FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mSubscribedPlayer);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateViewer)
{

	typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

	FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
	FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
	FUNCTION().PRIVATE().SIGNATURE(Spoiler*, CreateSpoiler);
	FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp1);
	FUNCTION().PRIVATE().SIGNATURE(void, OnFree);
	FUNCTION().PRIVATE().SIGNATURE(void, OnPlayPauseToggled, bool);
	FUNCTION().PRIVATE().SIGNATURE(void, OnLoopToggled, bool);
	FUNCTION().PRIVATE().SIGNATURE(void, OnEditPressed);
	FUNCTION().PRIVATE().SIGNATURE(void, OnTimeProgressChanged, float);
	FUNCTION().PRIVATE().SIGNATURE(void, OnAnimationUpdated, float);
}
END_META;
