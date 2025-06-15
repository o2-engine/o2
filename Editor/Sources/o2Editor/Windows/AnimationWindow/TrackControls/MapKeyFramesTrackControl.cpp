#include "o2Editor/stdafx.h"
#include "MapKeyFramesTrackControl.h"

#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"

namespace Editor
{
    MapKeyFramesTrackControl::MapKeyFramesTrackControl(RefCounter* refCounter) :
        ITrackControl(refCounter)
    {}

    MapKeyFramesTrackControl::MapKeyFramesTrackControl(RefCounter* refCounter, const MapKeyFramesTrackControl& other) :
        ITrackControl(refCounter, other)
    {}

    MapKeyFramesTrackControl::~MapKeyFramesTrackControl()
    {}

    MapKeyFramesTrackControl& MapKeyFramesTrackControl::operator=(const MapKeyFramesTrackControl& other)
    {
        Widget::operator=(other);
        return *this;
    }

#undef DrawText
    void MapKeyFramesTrackControl::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        OnDrawn();

        o2Render.EnableScissorTest(mTimeline.Lock()->layout->GetWorldRect());

        for (auto& child : mChildrenInheritedDepth)
            child->Draw();

        o2Render.DisableScissorTest();

        DrawDebugFrame();
    }

    void MapKeyFramesTrackControl::SetMappedTracks(const AnimationTree::TrackNode& valueNode)
    {
        CacheHandles();
        mTracks.Clear();

        InitializeNodeHandles(valueNode);
    }

    void MapKeyFramesTrackControl::Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet)
    {
        mTimeline = timeline;
        mHandlesSheet = handlesSheet;
    }

    void MapKeyFramesTrackControl::CacheHandles()
    {
        for (auto& kv : mHandlesGroups)
            kv.second->CacheHandles();

        mHandlesGroups.Clear();
    }

    void MapKeyFramesTrackControl::InitializeNodeHandles(const AnimationTree::TrackNode& valueNode)
    {
        if (valueNode.track)
        {
            Ref<IHandlesGroup> newGroup;

            if (auto animatedValue = DynamicCast<AnimationTrack<float>>(valueNode.track))
                newGroup = mmake<HandlesGroup<AnimationTrack<float>>>();
            else if (auto animatedValue = DynamicCast<AnimationTrack<bool>>(valueNode.track))
                newGroup = mmake<HandlesGroup<AnimationTrack<bool>>>();
            else if (auto animatedValue = DynamicCast<AnimationTrack<Vec2F>>(valueNode.track))
                newGroup = mmake<HandlesGroup<AnimationTrack<Vec2F>>>();
            else if (auto animatedValue = DynamicCast<AnimationTrack<Color4>>(valueNode.track))
                newGroup = mmake<HandlesGroup<AnimationTrack<Color4>>>();

            if (newGroup)
            {
                newGroup->trackControl = Ref(this);
                newGroup->InitializeHandles(valueNode.track, valueNode.path);

                mHandlesGroups.Add(valueNode.track, newGroup);
            }
        }

        for (auto& childNode : valueNode.children)
            InitializeNodeHandles(*childNode);
    }

    void MapKeyFramesTrackControl::UpdateHandles()
    {
        if (mDisableHandlesUpdate)
            return;

        for (auto& kv : mHandlesGroups)
            kv.second->UpdateHandles();
    }

    void MapKeyFramesTrackControl::SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime)
    {
        for (auto& kv : mHandlesGroups)
        {
            if (kv.second->SerializeKey(keyUid, data, relativeTime))
                break;
        }
    }

    Vector<Ref<ITrackControl::KeyHandle>> MapKeyFramesTrackControl::GetKeyHandles() const
    {
        Vector<Ref<ITrackControl::KeyHandle>> res;
        for (auto& kv : mHandlesGroups) {
            res.Add(DynamicCastVector<ITrackControl::KeyHandle>(kv.second->handles));
        }

        return res;
    }

    void MapKeyFramesTrackControl::DeleteKey(UInt64 keyUid)
    {
        for (auto& kv : mHandlesGroups)
            kv.second->DeleteKey(keyUid);
    }

    void MapKeyFramesTrackControl::UpdateHandlesForTrack(const Ref<IAnimationTrack>& animatedValue)
    {
        if (mHandlesGroups.ContainsKey(animatedValue))
            mHandlesGroups[animatedValue]->UpdateHandles();
    }

    void MapKeyFramesTrackControl::BeginKeysDrag()
    {}

    void MapKeyFramesTrackControl::EndKeysDrag()
    {}

    Ref<AnimationKeyDragHandle> MapKeyFramesTrackControl::CreateHandle()
    {
        auto handle = mmake<AnimationKeyDragHandle>(mmake<Sprite>("ui/UI4_map_key.png"),
                                                    mmake<Sprite>("ui/UI4_map_key_hover.png"),
                                                    mmake<Sprite>("ui/UI4_map_key_pressed.png"),
                                                    mmake<Sprite>("ui/UI4_selected_map_key.png"),
                                                    mmake<Sprite>("ui/UI4_selected_map_key_hover.png"),
                                                    mmake<Sprite>("ui/UI4_selected_map_key_pressed.png"));

        handle->cursorType = CursorType::SizeWE;
        handle->pixelPerfect = true;
        handle->SetDrawablesSizePivot(Vec2F(4, 1));

        handle->checkPositionFunc = [&](const Vec2F& pos) {
            float position = pos.x;
            if (position < 0.0f)
                position = 0.0f;

            return Vec2F(position, layout->GetHeight()*0.5f);
        };

        handle->localToWidgetOffsetTransformFunc = [&](const Vec2F& pos) {
            float worldXPos = mTimeline.Lock()->LocalToWorld(pos.x);
            float localXPos = worldXPos - layout->GetWorldLeft();

            return Vec2F(localXPos, 0);
        };

        handle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& pos) {
            float worldXPos = layout->GetWorldLeft() + pos.x;
            float localXPos = mTimeline.Lock()->WorldToLocal(worldXPos);

            return Vec2F(localXPos, 0);
        };

        return handle;
    }

    Vector<Ref<MapKeyFramesTrackControl::KeyHandle>> MapKeyFramesTrackControl::FindHandlesAtPosition(float position) const
    {
        Vector<Ref<KeyHandle>> res;

        auto timeline = mTimeline.Lock();
        for (auto& kv : mHandlesGroups)
        {
            for (auto& keyHandle : kv.second->handles)
            {
                if (timeline->IsSameTime(keyHandle->handle->GetPosition().x, position))
                    res.Add(keyHandle);
            }
        }

        return res;
    }

    MapKeyFramesTrackControl::KeyHandle::KeyHandle()
    {}

    MapKeyFramesTrackControl::KeyHandle::KeyHandle(UInt64 keyUid, const Ref<AnimationKeyDragHandle>& handle, 
                                                   const Ref<IAnimationTrack>& track,
                                                   const Function<void(KeyHandle& keyHandle)>& updateFunc) :
        ITrackControl::KeyHandle(keyUid, handle), track(track), updateFunc(updateFunc)
    {}

    bool MapKeyFramesTrackControl::KeyHandle::operator==(const KeyHandle& other) const
    {
        return handle == other.handle;
    }

    MapKeyFramesTrackControl::IHandlesGroup::~IHandlesGroup()
    {}

    void MapKeyFramesTrackControl::IHandlesGroup::CacheHandles()
    {
        auto trackControlRef = trackControl.Lock();

        for (auto& keyHandle : handles)
        {
            keyHandle->handle->SetParent(nullptr);
            keyHandle->handle->SetEnabled(false);
            keyHandle->handle->SetSelected(false);
            keyHandle->handle->SetSelectionGroup(nullptr);
            trackControlRef->mHandlesCache.Add(keyHandle->handle);
        }

        handles.clear();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::MapKeyFramesTrackControl>);
// --- META ---

DECLARE_CLASS(Editor::MapKeyFramesTrackControl, Editor__MapKeyFramesTrackControl);
// --- END META ---
