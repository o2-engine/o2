#include "o2Editor/stdafx.h"
#include "ITrackControl.h"

#include "o2Editor/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/AnimationWindow/Timeline.h"

namespace Editor
{

	void ITrackControl::Initialize(AnimationTimeline* timeline, KeyHandlesSheet* handlesSheet)
	{}

	void ITrackControl::SetTrack(IAnimationTrack* track, IAnimationTrack::IPlayer* player, const String& path)
	{}

	void ITrackControl::UpdateHandles()
	{}

	Vector<ITrackControl::KeyHandle*> ITrackControl::GetKeyHandles() const
	{
		return Vector<KeyHandle*>();
	}

	Widget* ITrackControl::GetTreePartControls() const
	{
		return nullptr;
	}

	void ITrackControl::SetCurveViewEnabled(bool enabled)
	{}

	void ITrackControl::SetCurveViewColor(const Color4& color)
	{}

	void ITrackControl::SetActive(bool active)
	{}

	void ITrackControl::InsertNewKey(float time)
	{}

	void ITrackControl::BeginKeysDrag()
	{}

	void ITrackControl::EndKeysDrag()
	{}

	void ITrackControl::SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime)
	{}

	UInt64 ITrackControl::DeserializeKey(const DataValue& data, float relativeTime, bool generateNewUid /*= true*/)
	{
		return 0;
	}

	void ITrackControl::DeleteKey(UInt64 keyUid)
	{}

	String ITrackControl::GetCreateMenuCategory()
	{
		return "UI/Editor";
	}

	bool ITrackControl::KeyHandle::operator==(const KeyHandle& other) const
	{
		return handle == other.handle;
	}
}
// --- META ---

DECLARE_CLASS(Editor::ITrackControl, Editor__ITrackControl);
// --- END META ---
