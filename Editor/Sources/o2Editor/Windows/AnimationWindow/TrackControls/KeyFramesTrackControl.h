#pragma once

#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AnimationWindow/AnimationKeysActions.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/AnimationKeyDragHandle.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/AnimationTrackWrapper.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/ITrackControl.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Properties/Properties.h"

using namespace o2;

namespace Editor
{
    // Draws curve by points transformed to basis
    void DrawCurveInCoords(const ApproximationValue* points, int pointsCount, const RectF& pointsBounds, const Basis& drawBasis,
                           const Color4& color);

    // -------------------------------------------------
    // Animation control track for key frames animations
    // Creates handles for each keys and updates them
    // -------------------------------------------------
    template<typename AnimationTrackType>
    class KeyFramesTrackControl : public ITrackControl
    {
    public:
        // Default constructor
        KeyFramesTrackControl(RefCounter* refCounter);

        // Copy-constructor
        KeyFramesTrackControl(RefCounter* refCounter, const KeyFramesTrackControl& other);

        // Destructor
        ~KeyFramesTrackControl();

        // Copy-operator
        KeyFramesTrackControl& operator=(const KeyFramesTrackControl& other);

        // Draws handles with clipping
        void Draw() override;

        // Updates widget, checks value change
        void Update(float dt) override;

        // Sets Animation track, updates and creates key handles
        void SetTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player, const String& path) override;

        // Returns Animation track
        Ref<AnimationTrackType> GetTrack() const;

        // Sets timeline for calculating handles positions, and  handles sheet as selecting group for handles
        void Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet) override;

        // Updates handles position on timeline
        void UpdateHandles() override;

        // Returns key handles list
        Vector<Ref<KeyHandle>> GetKeyHandles() const override;

        // Returns a container of controllers that are part of a tree
        Ref<Widget> GetTreePartControls() const override;

        // Sets curves edit view mode
        void SetCurveViewEnabled(bool enabled) override;

        // Sets curves view mode color
        void SetCurveViewColor(const Color4& color) override;

        // Serialize key with specified uid into data node
        void SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime) override;

        // Deserialize key from data node and paste on track
        UInt64 DeserializeKey(const DataValue& data, float relativeTime, bool generateNewUid = true) override;

        // Removes key from track
        void DeleteKey(UInt64 keyUid) override;

        // Inserts new key at time
        void InsertNewKey(float time) override;

        SERIALIZABLE(KeyFramesTrackControl<AnimationTrackType>);
        CLONEABLE_REF(KeyFramesTrackControl<AnimationTrackType>);

    protected:
        typedef AnimationTrackWrapper<AnimationTrackType> Wrapper;
        typedef typename Wrapper::TrackPlayerType         TrackPlayerType;
        typedef typename Wrapper::ValueType               TrackValueType;

        Vector<Ref<KeyHandle>> mHandles; // List of handles, each for keys

        String mTrackPath; // Path to Animation track in animation

        Ref<Widget> mTreeControls; // Container of controllers that are part of a tree

        Ref<IPropertyField>                    mPropertyField;
        TrackValueType                         mPropertyValue = TrackValueType();
        Ref<PointerValueProxy<TrackValueType>> mPropertyValueProxy;

        Ref<Button> mAddKeyDotButton; // Dot add key button colored with curve color
        Ref<Button> mAddKeyButton;    // Add key button, enables when available to add new key

        TrackValueType mLastValue = TrackValueType();

        WeakRef<AnimationTrackType> mTrack;         // Editing Animation track
        WeakRef<TrackPlayerType>    mPlayer;        // Track player
        WeakRef<AnimationTimeline>  mTimeline;      // Time line used for calculating handles positions
        WeakRef<KeyHandlesSheet>    mHandlesSheet;  // Handles sheet, used for drawing and managing drag handles

        bool mDisableHandlesUpdate = false;  // It is true when handles are changing and combining or updating is not available

    private:
        // Initializes controls
        void InitializeControls();

        // Initializes handles
        void InitializeHandles();

        // Called when track was set
        virtual void OnSetTrack() {}

        // Called when keys were changed
        virtual void OnKeysChanged() {}

        // Creates handle
        Ref<AnimationKeyDragHandle> CreateHandle();

        // Checks if new key can be created at time. If there is a key at time, button is disabled.
        void CheckCanCreateKey(float time);

        // Called when property was changed, adds new key
        void OnPropertyChanged();
    };

    template<typename AnimationTrackType>
    KeyFramesTrackControl<AnimationTrackType>::KeyFramesTrackControl(RefCounter* refCounter) :
        ITrackControl(refCounter)
    {
        InitializeControls();
    }

    template<typename AnimationTrackType>
    KeyFramesTrackControl<AnimationTrackType>::KeyFramesTrackControl(RefCounter* refCounter, const KeyFramesTrackControl& other) :
        ITrackControl(refCounter, other)
    {
        InitializeControls();
    }

    template<typename AnimationTrackType>
    KeyFramesTrackControl<AnimationTrackType>::~KeyFramesTrackControl()
    {
        if (mTrack)
            mTrack.Lock()->onKeysChanged -= THIS_SUBSCRIPTION(UpdateHandles, []() {});

        if (mPlayer)
            mPlayer.Lock()->onUpdate -= THIS_SUBSCRIPTION(CheckCanCreateKey, []() {});
    }

    template<typename AnimationTrackType>
    KeyFramesTrackControl<AnimationTrackType>& KeyFramesTrackControl<AnimationTrackType>::operator=(const KeyFramesTrackControl& other)
    {
        Widget::operator=(other);
        return *this;
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        o2Render.EnableScissorTest(mTimeline.Lock()->layout->GetWorldRect());

        for (auto& handle : mHandles)
            handle->handle->Draw();

        o2Render.DisableScissorTest();

        OnDrawn();
        DrawDebugFrame();
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::Update(float dt)
    {
        Widget::Update(dt);

        if (mPlayer)
        {
            auto playerValue = Wrapper::GetValue(*mPlayer.Lock());
            if (!Math::Equals(mLastValue, playerValue)) 
            {
                mPropertyValue = playerValue;
                mLastValue = mPropertyValue;
                mPropertyField->Refresh();
            }
        }
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::SetTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player, const String& path)
    {
        mTrackPath = path;

        if (mTrack)
            mTrack.Lock()->onKeysChanged -= THIS_SUBSCRIPTION(UpdateHandles, []() {});

        if (mPlayer)
            mPlayer.Lock()->onUpdate -= THIS_SUBSCRIPTION(CheckCanCreateKey, []() {});

        mTrack = DynamicCast<AnimationTrackType>(track);
        mPlayer = DynamicCast<TrackPlayerType>(player);

        if (mTrack)
            mTrack.Lock()->onKeysChanged += THIS_SUBSCRIPTION(UpdateHandles, [&]() { mTrack = nullptr; });

        if (mPlayer)
            mPlayer.Lock()->onUpdate += THIS_SUBSCRIPTION(CheckCanCreateKey, [&]() { mPlayer = nullptr; });

        InitializeHandles();
        CheckCanCreateKey(mTimeline.Lock()->GetTimeCursor());

        OnSetTrack();
    }

    template<typename AnimationTrackType>
    Ref<AnimationTrackType> KeyFramesTrackControl<AnimationTrackType>::GetTrack() const
    {
        return mTrack.Lock();
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet)
    {
        mTimeline = timeline;
        mHandlesSheet = handlesSheet;
    }

    template<typename AnimationTrackType>
    Vector<Ref<ITrackControl::KeyHandle>> KeyFramesTrackControl<AnimationTrackType>::GetKeyHandles() const
    {
        return mHandles;
    }

    template<typename AnimationTrackType>
    Ref<Widget> KeyFramesTrackControl<AnimationTrackType>::GetTreePartControls() const
    {
        return mTreeControls;
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::SetCurveViewColor(const Color4& color)
    {
        mAddKeyDotButton->GetLayerDrawable<Sprite>("basic/regularBack")->SetColor(color);
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::SetCurveViewEnabled(bool enabled)
    {}

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::InitializeControls()
    {
        mTreeControls = mmake<Widget>();

        auto fieldProto = o2EditorProperties.GetFieldPropertyType(&TypeOf(TrackValueType));
        mPropertyField = DynamicCast<IPropertyField>(o2UI.CreateWidget(*fieldProto, "standard"));
        mPropertyValueProxy = mmake<PointerValueProxy<TrackValueType>>(&mPropertyValue);
        mPropertyField->SetValueProxy({ DynamicCast<IAbstractValueProxy>(mPropertyValueProxy) });
        mPropertyField->onChangeCompleted = [&](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&) { OnPropertyChanged(); };
        *mPropertyField->layout = WidgetLayout::BothStretch(0, 0, 20, 0);

        mAddKeyButton = o2UI.CreateWidget<Button>("add key");
        *mAddKeyButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F());
        mAddKeyButton->onClick = [&]() { InsertNewKey(mTimeline.Lock()->GetTimeCursor()); };

        mAddKeyDotButton = o2UI.CreateWidget<Button>("add dot key");
        *mAddKeyDotButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, 0));
        mAddKeyDotButton->GetLayerDrawable<Sprite>("basic/regularBack")->SetColor(Color4::Black());
        mAddKeyDotButton->onClick = [&]() { InsertNewKey(mTimeline.Lock()->GetTimeCursor()); };
        mAddKeyDotButton->enabled = false;

        mTreeControls->AddChildren({ mPropertyField, mAddKeyButton, mAddKeyDotButton });
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::InitializeHandles()
    {
        PushEditorScopeOnStack scope;

        Vector<UInt64> selectedHandles;
        for (auto& keyHandle : mHandles) 
        {
            if (keyHandle->handle->IsSelected())
                selectedHandles.Add(keyHandle->keyUid);
        }

        Vector<Ref<AnimationKeyDragHandle>> handlesCache = mHandles.Convert<Ref<AnimationKeyDragHandle>>(
            [&](const Ref<KeyHandle>& x) 
            {
                x->handle->SetParent(nullptr);
                x->handle->SetEnabled(false);
                x->handle->SetSelectionGroup(nullptr);
                x->handle->SetSelected(false);
                x->handle->onChangedPos.Clear();
                return x->handle;
            });

        mHandles.Clear();

        auto trackRef = mTrack.Lock();
        auto handlesSheet = mHandlesSheet.Lock();

        for (auto& key : Wrapper::GetKeys(*trackRef))
        {
            Ref<AnimationKeyDragHandle> handle;

            if (!handlesCache.IsEmpty())
                handle = handlesCache.PopBack();
            else
                handle = CreateHandle();

            handle->SetEnabled(true);
            handle->SetPosition(Vec2F(key.position, 0.0f));
            handle->track = trackRef;
            handle->trackPath = mTrackPath;
            handle->trackControl = Ref(this);
            handle->keyUid = key.uid;
            handle->isMapping = false;
            handle->SetSelectionGroup(DynamicCast<ISelectableDragHandlesGroup>(handlesSheet));
            handle->SetSelected(selectedHandles.Contains(key.uid));

            AddChild(handle);

            auto keyhandle = mmake<KeyHandle>(key.uid, handle);
            mHandles.Add(keyhandle);

            handle->onChangedPos = [=](const Vec2F& pos) 
            {
                mDisableHandlesUpdate = true;

                int keyIdx = Wrapper::FindKeyIdx(*trackRef, keyhandle->keyUid);
                auto key = Wrapper::GetKeys(*trackRef)[keyIdx];
                key.position = pos.x;

                Wrapper::RemoveKeyAt(*trackRef, keyIdx);
                Wrapper::AddKey(*trackRef, key);

                mDisableHandlesUpdate = false;
            };
        }
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::UpdateHandles()
    {
        if (!mTrack)
            return;

        if (mDisableHandlesUpdate)
            return;

        auto track = mTrack.Lock();

        if (Wrapper::GetKeys(*track).Count() != mHandles.Count())
        {
            InitializeHandles();
        }
        else 
        {
            for (auto& keyHandle : mHandles)
                keyHandle->handle->SetPosition(Vec2F(Wrapper::FindKey(*track, keyHandle->keyUid).position, 0.0f));
        }
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::CheckCanCreateKey(float time)
    {
        auto timeline = mTimeline.Lock();
        auto track = mTrack.Lock();

        time = timeline->GetTimeCursor();

        bool hasKeyAtTime = false;
        for (auto& key : Wrapper::GetKeys(*track))
        {
            if (timeline->IsSameTime(key.position, time))
            {
                hasKeyAtTime = true;
                break;
            }
        }

        mAddKeyButton->SetInteractable(!hasKeyAtTime);
    }

    template<typename AnimationTrackType>
    Ref<AnimationKeyDragHandle> KeyFramesTrackControl<AnimationTrackType>::CreateHandle()
    {
        auto handle = mmake<AnimationKeyDragHandle>(mmake<Sprite>("ui/UI4_key.png"),
                                                    mmake<Sprite>("ui/UI4_key_hover.png"),
                                                    mmake<Sprite>("ui/UI4_key_pressed.png"),
                                                    mmake<Sprite>("ui/UI4_selected_key.png"),
                                                    mmake<Sprite>("ui/UI4_selected_key_hover.png"),
                                                    mmake<Sprite>("ui/UI4_selected_key_pressed.png"));

        handle->cursorType = CursorType::SizeWE;
        handle->pixelPerfect = true;
        handle->SetDrawablesSizePivot(Vec2F(4, 1));

        handle->checkPositionFunc = [&](const Vec2F& pos)
            {
                float position = pos.x;
                if (position < 0.0f)
                    position = 0.0f;

                return Vec2F(position, layout->GetHeight() * 0.5f);
            };

        handle->localToWidgetOffsetTransformFunc = [&](const Vec2F& pos)
            {
                float worldXPos = mTimeline.Lock()->LocalToWorld(pos.x);
                float localXPos = worldXPos - layout->GetWorldLeft();

                return Vec2F(localXPos, 0);
            };

        handle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& pos)
            {
                float worldXPos = layout->GetWorldLeft() + pos.x;
                float localXPos = mTimeline.Lock()->WorldToLocal(worldXPos);

                return Vec2F(localXPos, 0);
            };

        return handle;
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::InsertNewKey(float time)
    {
        auto handlesSheet = mHandlesSheet.Lock();
        auto track = mTrack.Lock();

        typename Wrapper::KeyType key;
        key.position = time;
        key.value = Wrapper::GetValue(*track, time);

        int idx = Wrapper::AddKey(*track, key);

        InitializeHandles();
        mTimeline.Lock()->SetTimeCursor(time);

        OnKeysChanged();

        DataDocument keyData;
        Map<String, Vector<UInt64>> keys = { { mTrackPath, { Wrapper::GetKey(*track, idx).uid } } };
        handlesSheet->SerializeKeys(keyData, keys, 0);
        handlesSheet->mAnimationWindow.Lock()->mActionsList->DoneAction(mmake<AnimationAddKeysAction>(keys, keyData, handlesSheet));
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::OnPropertyChanged()
    {
        auto trackRef = mTrack.Lock();

        auto time = mTimeline.Lock()->GetTimeCursor();
        int keyIdx = -1;
        int i = 0;
        for (auto& key : Wrapper::GetKeys(*trackRef))
        {
            if (mTimeline.Lock()->IsSameTime(key.position, time))
            {
                keyIdx = i;
                break;
            }

            i++;
        }

        if (keyIdx >= 0) {
            auto key = Wrapper::GetKeys(*trackRef)[keyIdx];
            Wrapper::RemoveKeyAt(*trackRef, keyIdx);
            key.value = mPropertyValue;
            Wrapper::AddKey(*trackRef, key);
        }
        else 
        {
            typename Wrapper::KeyType key;
            key.position = time;
            key.value = mPropertyValue;
            Wrapper::AddKey(*trackRef, key);
            InitializeHandles();
        }

        OnKeysChanged();
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime)
    {
        auto key = Wrapper::FindKey(*mTrack.Lock(), keyUid);
        key.position -= relativeTime;
        data.Set(key);
    }

    template<typename AnimationTrackType>
    UInt64 KeyFramesTrackControl<AnimationTrackType>::DeserializeKey(const DataValue& data, float relativeTime,
                                                                     bool generateNewUid /*= true*/)
    {
        typename Wrapper::KeyType key;
        data.Get(key);
        key.position += relativeTime;

        if (generateNewUid)
            key.uid = Math::Random();

        Wrapper::AddKey(*mTrack.Lock(), key);

        return key.uid;
    }

    template<typename AnimationTrackType>
    void KeyFramesTrackControl<AnimationTrackType>::DeleteKey(UInt64 keyUid)
    {
        auto trackRef = mTrack.Lock();

        int idx = Wrapper::FindKeyIdx(*trackRef, keyUid);
        if (idx >= 0)
            Wrapper::RemoveKeyAt(*trackRef, idx);
    }

}
// --- META ---

META_TEMPLATES(typename AnimationTrackType)
CLASS_BASES_META(Editor::KeyFramesTrackControl<AnimationTrackType>)
{
    BASE_CLASS(Editor::ITrackControl);
}
END_META;
META_TEMPLATES(typename AnimationTrackType)
CLASS_FIELDS_META(Editor::KeyFramesTrackControl<AnimationTrackType>)
{
    FIELD().PROTECTED().NAME(mHandles);
    FIELD().PROTECTED().NAME(mTrackPath);
    FIELD().PROTECTED().NAME(mTreeControls);
    FIELD().PROTECTED().NAME(mPropertyField);
    FIELD().PROTECTED().DEFAULT_VALUE(TrackValueType()).NAME(mPropertyValue);
    FIELD().PROTECTED().NAME(mPropertyValueProxy);
    FIELD().PROTECTED().NAME(mAddKeyDotButton);
    FIELD().PROTECTED().NAME(mAddKeyButton);
    FIELD().PROTECTED().DEFAULT_VALUE(TrackValueType()).NAME(mLastValue);
    FIELD().PROTECTED().NAME(mTrack);
    FIELD().PROTECTED().NAME(mPlayer);
    FIELD().PROTECTED().NAME(mTimeline);
    FIELD().PROTECTED().NAME(mHandlesSheet);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDisableHandlesUpdate);
}
END_META;
META_TEMPLATES(typename AnimationTrackType)
CLASS_METHODS_META(Editor::KeyFramesTrackControl<AnimationTrackType>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const KeyFramesTrackControl&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&, const Ref<IAnimationTrack::IPlayer>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationTrackType>, GetTrack);
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize, const Ref<AnimationTimeline>&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateHandles);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<KeyHandle>>, GetKeyHandles);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetTreePartControls);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurveViewEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurveViewColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SerializeKey, UInt64, DataValue&, float);
    FUNCTION().PUBLIC().SIGNATURE(UInt64, DeserializeKey, const DataValue&, float, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, DeleteKey, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertNewKey, float);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeControls);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, OnSetTrack);
    FUNCTION().PRIVATE().SIGNATURE(void, OnKeysChanged);
    FUNCTION().PRIVATE().SIGNATURE(Ref<AnimationKeyDragHandle>, CreateHandle);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckCanCreateKey, float);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPropertyChanged);
}
END_META;
// --- END META ---
