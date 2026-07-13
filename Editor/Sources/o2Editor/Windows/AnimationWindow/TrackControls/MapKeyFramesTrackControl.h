#pragma once

#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/ITrackControl.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/AnimationTrackWrapper.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------
    // Animation control track for mapped keys from children Animation tracks
    // ---------------------------------------------------------------------
    class MapKeyFramesTrackControl : public ITrackControl
    {
    public:
        // Default constructor
        MapKeyFramesTrackControl(RefCounter* refCounter);

        // Copy-constructor
        MapKeyFramesTrackControl(RefCounter* refCounter, const MapKeyFramesTrackControl& other);

        // Destructor
        ~MapKeyFramesTrackControl();

        // Copy-operator
        MapKeyFramesTrackControl& operator=(const MapKeyFramesTrackControl& other);

        // Sets timeline for calculating handles positions, and  handles sheet as selecting group for handles
        void Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet) override;

        // Draws handles with clipping
        void Draw() override;

        // Updates handles position on timeline
        void UpdateHandles() override;

        // Serialize key with specified uid into data node
        void SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime) override;

        // Returns key handles list
        Vector<Ref<ITrackControl::KeyHandle>> GetKeyHandles() const override;

        // Removes key from track
        void DeleteKey(UInt64 keyUid) override;

        // Sets mapped Animation tracks. Creates handles
        void SetMappedTracks(const AnimationTree::TrackNode& valueNode);

        // Updates handles positions for specified Animation track
        void UpdateHandlesForTrack(const Ref<IAnimationTrack>& track);

        // Called when group of keys began dragged, disables keys combining
        void BeginKeysDrag() override;

        // Called when group of keys completed drag, enables keys combining
        void EndKeysDrag() override;

        SERIALIZABLE(MapKeyFramesTrackControl);
        CLONEABLE_REF(MapKeyFramesTrackControl);

    private:
        struct KeyHandle : public ITrackControl::KeyHandle
        {
            WeakRef<IAnimationTrack> track;

            Function<void(KeyHandle& keyHandle)> updateFunc;

        public:
            KeyHandle();
            KeyHandle(UInt64 keyUid, const Ref<AnimationKeyDragHandle>& handle, const Ref<IAnimationTrack>& track,
                      const Function<void(KeyHandle& keyHandle)>& updateFunc);

            bool operator==(const KeyHandle& other) const;
        };

        struct IHandlesGroup: public RefCounterable
        {
            String                 trackPath;
            Vector<Ref<KeyHandle>> handles;

            WeakRef<MapKeyFramesTrackControl> trackControl;

        public:
            virtual ~IHandlesGroup();

            virtual void InitializeHandles(const Ref<IAnimationTrack>& itrack, const String& trackPath) = 0;
            virtual void CreateHandles() = 0;
            virtual void OnHandleChangedPos(const Ref<KeyHandle>& keyHandle, const Vec2F& pos) = 0;
            virtual void UpdateHandles() = 0;
            virtual bool SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime) = 0;
            virtual void DeleteKey(UInt64 keyUid) = 0;
            void CacheHandles();
        };

        template<typename TrackType>
        struct HandlesGroup : public IHandlesGroup
        {
            typedef AnimationTrackWrapper<TrackType> Wrapper;

            WeakRef<TrackType> track;

        public:
            ~HandlesGroup();

            void InitializeHandles(const Ref<IAnimationTrack>& itrack, const String& trackPath) override;
            void CreateHandles() override;
            void OnHandleChangedPos(const Ref<KeyHandle>& keyHandle, const Vec2F& pos) override;
            void UpdateHandles() override;
            bool SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime) override;
            void DeleteKey(UInt64 keyUid) override;
        };

    private:
        Map<WeakRef<IAnimationTrack>, Ref<IHandlesGroup>> mHandlesGroups; // List of handles, each for keys
        Vector<WeakRef<IAnimationTrack>>                  mTracks;        // Editing Animation tracks
        WeakRef<AnimationTimeline>                        mTimeline;      // Time line used for calculating handles positions
        WeakRef<KeyHandlesSheet>                          mHandlesSheet;  // Handles sheet, used for drawing and managing drag handles

        Vector<Ref<AnimationKeyDragHandle>> mHandlesCache; // Cached drag handles, can be reused

        bool mDisableHandlesUpdate = false;  // It is true when handles are changing and combining or updating is not available

    private:
        void CacheHandles();
        void CleanupCachedHandles();
        void InitializeNodeHandles(const AnimationTree::TrackNode& valueNode);

        Ref<AnimationKeyDragHandle> CreateHandle();

        Vector<Ref<KeyHandle>> FindHandlesAtPosition(float position) const;
    };

    template<typename TrackType>
    MapKeyFramesTrackControl::HandlesGroup<TrackType>::~HandlesGroup()
    {
        if (track)
            track.Lock()->onKeysChanged -= MakeFunction(this, &HandlesGroup<TrackType>::UpdateHandles);
    }

    template<typename TrackType>
    void MapKeyFramesTrackControl::HandlesGroup<TrackType>::InitializeHandles(const Ref<IAnimationTrack>& itrack,
                                                                              const String& trackPath)
    {
        this->trackPath = trackPath;

        track = DynamicCast<TrackType>(itrack);
        track.Lock()->onKeysChanged += MakeFunction(this, &HandlesGroup<TrackType>::UpdateHandles);

        trackControl.Lock()->mTracks.Add(itrack);

        CreateHandles();
    }

    template<typename TrackType>
    void MapKeyFramesTrackControl::HandlesGroup<TrackType>::CreateHandles()
    {
        PushEditorScopeOnStack scope;

        auto trackControlRef = trackControl.Lock();
        auto trackRef = track.Lock();
        auto handlesSheet = trackControlRef->mHandlesSheet.Lock();

        // Handles bound to another track are unregistered while the old track is still
        // set, so the sheet removes them from the correct track group before rebinding
        Vector<Ref<DragHandle>> staleBoundHandles;
        for (auto& handle : trackControlRef->mHandlesCache)
        {
            if (handle->GetSelectionGroup() && handle->track.Lock().Get() != trackRef.Get())
                staleBoundHandles.Add(handle);
        }

        if (!staleBoundHandles.IsEmpty())
            handlesSheet->RemoveHandles(staleBoundHandles);

        Vector<Ref<DragHandle>> sheetHandles;
        for (auto& key : Wrapper::GetKeys(*trackRef))
        {
            Ref<AnimationKeyDragHandle> handle;

            if (!trackControlRef->mHandlesCache.IsEmpty())
                handle = trackControlRef->mHandlesCache.PopBack();
            else
                handle = trackControlRef->CreateHandle();

            handle->SetEnabled(true);
            handle->SetPosition(Vec2F(key.position, 0.0f));
            handle->track = trackRef;
            handle->trackPath = trackPath;
            handle->trackControl = trackControlRef;
            handle->keyUid = key.uid;
            handle->isMapping = true;
            sheetHandles.Add(handle);

            auto updatePosFunc = [=](KeyHandle& keyHandle)
                {
                    keyHandle.handle->SetPosition(Vec2F(Wrapper::FindKey(*trackRef, handle->keyUid).position, 0.0f));
                };

            auto keyHandle = mmake<KeyHandle>(key.uid, handle, trackRef, updatePosFunc);
            handles.Add(keyHandle);

            handle->onChangedPos = [=](const Vec2F& pos) { OnHandleChangedPos(keyHandle, pos); };
            handle->onPressed = [=]()
                {
                    if (!o2Input.IsKeyDown(VK_CONTROL))
                        handle->GetSelectionGroup()->DeselectAll();

                    trackControlRef->FindHandlesAtPosition(keyHandle->handle->GetPosition().x)
                        .ForEach([](auto& keyHandle) { keyHandle->handle->SetSelected(true); });
                };
            handle->onReleased = [&]() { UpdateHandles(); };
        }

        handlesSheet->AddHandles(sheetHandles);
    }

    template<typename TrackType>
    void MapKeyFramesTrackControl::HandlesGroup<TrackType>::UpdateHandles()
    {
        if (trackControl.Lock()->mDisableHandlesUpdate)
            return;

        if (Wrapper::GetKeys(*track.Lock()).Count() != handles.Count())
        {
            Vector<UInt64> selectedHandles;
            for (auto& keyHandle : handles)
            {
                if (keyHandle->handle->IsSelected())
                    selectedHandles.Add(keyHandle->keyUid);
            }

            CacheHandles();
            CreateHandles();
            trackControl.Lock()->CleanupCachedHandles();

            for (auto& keyHandle : handles)
                keyHandle->handle->SetSelected(selectedHandles.Contains(keyHandle->keyUid));
        }
        else
        {
            for (auto& keyHandle : handles)
                keyHandle->updateFunc(*keyHandle);
        }
    }

    template<typename TrackType>
    void MapKeyFramesTrackControl::HandlesGroup<TrackType>::OnHandleChangedPos(const Ref<KeyHandle>& keyHandle, const Vec2F& pos)
    {
        auto trackRef = track.Lock();

        trackControl.Lock()->mDisableHandlesUpdate = true;

        int keyIdx = Wrapper::FindKeyIdx(*trackRef, keyHandle->keyUid);
        auto key = Wrapper::GetKeys(*trackRef)[keyIdx];

        key.position = pos.x;
        Wrapper::RemoveKeyAt(*trackRef, keyIdx);
        Wrapper::AddKey(*trackRef, key);

        trackControl.Lock()->mDisableHandlesUpdate = false;
    }

    template<typename TrackType>
    bool MapKeyFramesTrackControl::HandlesGroup<TrackType>::SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime)
    {
        auto trackRef = track.Lock();

        int idx = Wrapper::FindKeyIdx(*trackRef, keyUid);
        if (idx < 0)
            return false;

        auto key = Wrapper::GetKey(*trackRef, idx);
        key.position -= relativeTime;
        data.Set(key);

        return true;
    }

    template<typename TrackType>
    void MapKeyFramesTrackControl::HandlesGroup<TrackType>::DeleteKey(UInt64 keyUid)
    {
        auto trackRef = track.Lock();

        int idx = Wrapper::FindKeyIdx(*trackRef, keyUid);
        if (idx >= 0)
            Wrapper::RemoveKeyAt(*trackRef, idx);
    }
}
// --- META ---

CLASS_BASES_META(Editor::MapKeyFramesTrackControl)
{
    BASE_CLASS(Editor::ITrackControl);
}
END_META;
CLASS_FIELDS_META(Editor::MapKeyFramesTrackControl)
{
    FIELD().PRIVATE().NAME(mHandlesGroups);
    FIELD().PRIVATE().NAME(mTracks);
    FIELD().PRIVATE().NAME(mTimeline);
    FIELD().PRIVATE().NAME(mHandlesSheet);
    FIELD().PRIVATE().NAME(mHandlesCache);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mDisableHandlesUpdate);
}
END_META;
CLASS_METHODS_META(Editor::MapKeyFramesTrackControl)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const MapKeyFramesTrackControl&);
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize, const Ref<AnimationTimeline>&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateHandles);
    FUNCTION().PUBLIC().SIGNATURE(void, SerializeKey, UInt64, DataValue&, float);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<ITrackControl::KeyHandle>>, GetKeyHandles);
    FUNCTION().PUBLIC().SIGNATURE(void, DeleteKey, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMappedTracks, const AnimationTree::TrackNode&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateHandlesForTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysDrag);
    FUNCTION().PUBLIC().SIGNATURE(void, EndKeysDrag);
    FUNCTION().PRIVATE().SIGNATURE(void, CacheHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, CleanupCachedHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeNodeHandles, const AnimationTree::TrackNode&);
    FUNCTION().PRIVATE().SIGNATURE(Ref<AnimationKeyDragHandle>, CreateHandle);
    FUNCTION().PRIVATE().SIGNATURE(Vector<Ref<KeyHandle>>, FindHandlesAtPosition, float);
}
END_META;
// --- END META ---
