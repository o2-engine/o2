#include "o2Editor/stdafx.h"
#include "ITrackControl.h"

#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"

namespace Editor
{

    ITrackControl::ITrackControl(RefCounter* refCounter):
        Widget(refCounter)
    {}

    ITrackControl::ITrackControl(RefCounter* refCounter, const ITrackControl& other):
        Widget(refCounter, other)
    {}

    void ITrackControl::Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet)
    {}

    void ITrackControl::SetTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player, const String& path)
    {}

    void ITrackControl::UpdateHandles()
    {}

    Vector<Ref<ITrackControl::KeyHandle>> ITrackControl::GetKeyHandles() const
    {
        return {};
    }

    Ref<Widget> ITrackControl::GetTreePartControls() const
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

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ITrackControl>);
// --- META ---

DECLARE_CLASS(Editor::ITrackControl, Editor__ITrackControl);
// --- END META ---
