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
    // -------------------------------------
    // Animation control track for aub track
    // -------------------------------------
    class SubTrackControl : public ITrackControl
    {
    public:
        PROPERTIES(SubTrackControl);

    public:
        // Default constructor
        SubTrackControl(RefCounter* refCounter);

        // Copy-constructor
        SubTrackControl(RefCounter* refCounter, const SubTrackControl& other);

        // Destructor
        ~SubTrackControl();

        // Copy-operator
        SubTrackControl& operator=(const SubTrackControl& other);

        // Sets timeline for calculating handles positions, and  handles sheet as selecting group for handles
        void Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet) override;

        // Draws handles with clipping
        void Draw() override;

        // Sets Animation track, updates and creates key handles
        void SetTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player, const String& path) override;

        // Updates handles position on timeline
        void UpdateHandles() override;

        // Serialize key with specified uid into data node
        void SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime) override;

        // Returns key handles list
        Vector<Ref<ITrackControl::KeyHandle>> GetKeyHandles() const override;

        // Removes key from track
        void DeleteKey(UInt64 keyUid) override;

        // Called when group of keys began dragged, disables keys combining
        void BeginKeysDrag() override;

        // Called when group of keys completed drag, enables keys combining
        void EndKeysDrag() override;

        SERIALIZABLE(SubTrackControl);
        CLONEABLE_REF(SubTrackControl);

    private:
        Ref<KeyHandle> mLeftBorderHandle;  // Left border handle
        Ref<KeyHandle> mRightBorderHandle; // Right border handle
        Ref<KeyHandle> mCenterHandle;      // Center handle

        bool mDisableHandlesUpdate = false; // It is true when handles are changing and combining or updating is not available

        String mTrackPath; // Path to Animation track in animation

        WeakRef<AnimationSubTrack>         mTrack;  // Editing Animation track
        WeakRef<AnimationSubTrack::Player> mPlayer; // Track player

        WeakRef<AnimationTimeline> mTimeline;     // Time line used for calculating handles positions
        WeakRef<KeyHandlesSheet>   mHandlesSheet; // Handles sheet, used for drawing and managing drag handles

        Ref<Sprite> mTrackSprite; // Track sprite

    private:
        // Initializes handles
        void InitializeHandles();

        // Initializes handles for track
        void SetupTrackHandles();

        // Draws track with offsets
        void DrawTrack();
    };
}
// --- META ---

CLASS_BASES_META(Editor::SubTrackControl)
{
    BASE_CLASS(Editor::ITrackControl);
}
END_META;
CLASS_FIELDS_META(Editor::SubTrackControl)
{
    FIELD().PRIVATE().NAME(mLeftBorderHandle);
    FIELD().PRIVATE().NAME(mRightBorderHandle);
    FIELD().PRIVATE().NAME(mCenterHandle);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mDisableHandlesUpdate);
    FIELD().PRIVATE().NAME(mTrackPath);
    FIELD().PRIVATE().NAME(mTrack);
    FIELD().PRIVATE().NAME(mPlayer);
    FIELD().PRIVATE().NAME(mTimeline);
    FIELD().PRIVATE().NAME(mHandlesSheet);
    FIELD().PRIVATE().NAME(mTrackSprite);
}
END_META;
CLASS_METHODS_META(Editor::SubTrackControl)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SubTrackControl&);
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize, const Ref<AnimationTimeline>&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&, const Ref<IAnimationTrack::IPlayer>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateHandles);
    FUNCTION().PUBLIC().SIGNATURE(void, SerializeKey, UInt64, DataValue&, float);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<ITrackControl::KeyHandle>>, GetKeyHandles);
    FUNCTION().PUBLIC().SIGNATURE(void, DeleteKey, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysDrag);
    FUNCTION().PUBLIC().SIGNATURE(void, EndKeysDrag);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, SetupTrackHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, DrawTrack);
}
END_META;
// --- END META ---
