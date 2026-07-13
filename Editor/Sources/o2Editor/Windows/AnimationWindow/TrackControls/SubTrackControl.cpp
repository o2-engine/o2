#include "o2Editor/stdafx.h"
#include "SubTrackControl.h"

#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"

namespace Editor
{
    SubTrackControl::SubTrackControl(RefCounter* refCounter) :
        ITrackControl(refCounter)
    {
        InitializeHandles();
    }

    SubTrackControl::SubTrackControl(RefCounter* refCounter, const SubTrackControl& other) :
        ITrackControl(refCounter, other)
    {
        InitializeHandles();
    }

    SubTrackControl::~SubTrackControl()
    {}

    SubTrackControl& SubTrackControl::operator=(const SubTrackControl& other)
    {
        Widget::operator=(other);
        return *this;
    }

#undef DrawText
    void SubTrackControl::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        OnDrawn();

        o2Render.EnableScissorTest(mTimeline.Lock()->layout->GetWorldRect());

        DrawTrack();

        mCenterHandle->handle->Draw();
        mLeftBorderHandle->handle->Draw();
        mRightBorderHandle->handle->Draw();

        o2Render.DisableScissorTest();

        DrawDebugFrame();
    }

    void SubTrackControl::SetTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player, const String& path)
    {
        mTrackPath = path;

        if (mTrack)
            mTrack.Lock()->onKeysChanged -= THIS_FUNC(UpdateHandles);

        mTrack = DynamicCast<AnimationSubTrack>(track);
        mPlayer = DynamicCast<AnimationSubTrack::Player>(player);

        if (mTrack)
            mTrack.Lock()->onKeysChanged += THIS_FUNC(UpdateHandles);

        SetupTrackHandles();
    }

    void SubTrackControl::Initialize(const Ref<AnimationTimeline>& timeline, const Ref<KeyHandlesSheet>& handlesSheet)
    {
        mTimeline = timeline;
        mHandlesSheet = handlesSheet;
    }

    void SubTrackControl::DrawTrack()
    {
        auto track = mTrack.Lock();
        auto timeline = mTimeline.Lock();
        if (!track || !timeline)
            return;

        float bottom = layout->GetWorldBottom();
        float top = layout->GetWorldTop();

        float originTrackBegin = timeline->LocalToWorld(track->beginTime);
        float originTrackEnd = timeline->LocalToWorld(track->beginTime + track->subTrackDuration);

        float trackBegin = timeline->LocalToWorld(track->beginTime + track->subTrackBeginOffset);
        float trackEnd = timeline->LocalToWorld(track->beginTime + track->subTrackDuration - track->subTrackEndOffset);

        float trackSpriteOffset = 2;

        mTrackSprite->transparency = 0.5f;
        mTrackSprite->rect = RectF(originTrackBegin - trackSpriteOffset, bottom, originTrackEnd + trackSpriteOffset, top);
        mTrackSprite->Draw();

        mTrackSprite->transparency = 1.0f;
        mTrackSprite->rect = RectF(trackBegin - trackSpriteOffset, bottom, trackEnd + trackSpriteOffset, top);
        mTrackSprite->Draw();
    }

    void SubTrackControl::InitializeHandles()
    {
        auto localToScreenTransformFunc = [&](const Vec2F& pos)
        {
            return Vec2F(mTimeline.Lock()->LocalToWorld(pos.x), layout->GetWorldBottom());
        };

        auto screenToLocalTransformFunc = [&](const Vec2F& pos)
        {
            return Vec2F(mTimeline.Lock()->WorldToLocal(pos.x), 0.0f);
        };

        float handleSize = 10.0f;
        float snapThreshold = 0.01f;

        {
            auto handle = mmake<AnimationKeyDragHandle>();
            handle->cursorType = CursorType::SizeAll;
            handle->pixelPerfect = true;

            handle->checkPositionFunc = [&](const Vec2F& pos)
            {
                float position = pos.x;
                if (position < 0.0f)
                    position = 0.0f;

                return Vec2F(position, layout->GetHeight() * 0.5f);
            };

            handle->isPointInside = [=](const Vec2F& pos)
            {
                auto track = mTrack.Lock();
                auto timeline = mTimeline.Lock();
                if (!track || !timeline)
                    return false;

                RectF handleRect(timeline->LocalToWorld(track->beginTime), handle->GetScreenPosition().y,
                                 timeline->LocalToWorld(track->beginTime + track->subTrackDuration), handle->GetScreenPosition().y + layout->GetHeight());
                return handleRect.IsInside(pos);
            };

            handle->localToScreenTransformFunc = localToScreenTransformFunc;
            handle->screenToLocalTransformFunc = screenToLocalTransformFunc;

            mCenterHandle = mmake<KeyHandle>(2, handle);
        }

        {
            auto handle = mmake<AnimationKeyDragHandle>();
            handle->cursorType = CursorType::SizeWE;
            handle->pixelPerfect = true;

            handle->checkPositionFunc = [=](const Vec2F& pos)
            {
                float position = pos.x;
                if (position < 0.0f)
                    position = 0.0f;

                if (auto track = mTrack.Lock())
                {
                    if (Math::Abs(position - track->beginTime) < snapThreshold)
                        position = track->beginTime;
                }

                return Vec2F(position, layout->GetHeight() * 0.5f);
            };

            handle->isPointInside = [=](const Vec2F& pos)
            {
                Vec2F borderHandlesSize = Vec2F(handleSize, layout->GetHeight());
                RectF borderHandlesRect = RectF(-borderHandlesSize.x * 0.5f, 0, borderHandlesSize.x * 0.5f, -borderHandlesSize.y);
                auto localPos = handle->GetScreenPosition() - pos;
                return borderHandlesRect.IsInside(localPos);
            };

            handle->localToScreenTransformFunc = localToScreenTransformFunc;
            handle->screenToLocalTransformFunc = screenToLocalTransformFunc;

            mLeftBorderHandle = mmake<KeyHandle>(0, handle);
        }

        {
            auto handle = mmake<AnimationKeyDragHandle>();
            handle->cursorType = CursorType::SizeWE;
            handle->pixelPerfect = true;

            handle->checkPositionFunc = [=](const Vec2F& pos)
            {
                float position = pos.x;
                if (position < 0.0f)
                    position = 0.0f;

                if (auto track = mTrack.Lock())
                {
                    if (Math::Abs(position - (track->beginTime + track->subTrackDuration)) < snapThreshold)
                        position = track->beginTime + track->subTrackDuration;
                }

                return Vec2F(position, layout->GetHeight() * 0.5f);
            };

            handle->isPointInside = [=](const Vec2F& pos)
            {
                if (o2Input.IsKeyDown(VK_CONTROL))
                    return false;

                Vec2F borderHandlesSize = Vec2F(handleSize, layout->GetHeight());
                RectF borderHandlesRect = RectF(-borderHandlesSize.x * 0.5f, 0, borderHandlesSize.x * 0.5f, -borderHandlesSize.y);
                auto localPos = handle->GetScreenPosition() - pos;
                return borderHandlesRect.IsInside(localPos);
            };

            handle->localToScreenTransformFunc = localToScreenTransformFunc;
            handle->screenToLocalTransformFunc = screenToLocalTransformFunc;

            mRightBorderHandle = mmake<KeyHandle>(1, handle);
        }

        mTrackSprite = mmake<Sprite>("ui/UI4_sub_track.png");
    }

    void SubTrackControl::SetupTrackHandles()
    {
        auto track = mTrack.Lock();
        auto handlesSheet = mHandlesSheet.Lock();
        if (!track || !handlesSheet)
            return;

        {
            auto handle = mCenterHandle->handle;
            handle->SetPosition(Vec2F(track->beginTime, 0.0f));
            handle->track = track;
            handle->trackPath = mTrackPath;
            handle->trackControl = Ref(this);
            handle->isMapping = false;
            handle->SetSelectionGroup(DynamicCast<ISelectableDragHandlesGroup>(handlesSheet));
            //handle->SetSelected(selectedHandles.Contains(key.uid));

            handle->onChangedPos = [this, track](const Vec2F& pos)
            {
                mDisableHandlesUpdate = true;

                track->beginTime = pos.x;
                mLeftBorderHandle->handle->SetPosition(Vec2F(track->beginTime + track->subTrackBeginOffset, 0.0f));
                mRightBorderHandle->handle->SetPosition(Vec2F(track->beginTime + track->subTrackDuration - track->subTrackEndOffset, 0.0f));

                mDisableHandlesUpdate = false;
            };
        }

        {
            auto handle = mLeftBorderHandle->handle;
            handle->SetPosition(Vec2F(track->beginTime + track->subTrackBeginOffset, 0.0f));
            handle->track = track;
            handle->trackPath = mTrackPath;
            handle->trackControl = Ref(this);
            handle->isMapping = false;
            handle->SetSelectionGroup(DynamicCast<ISelectableDragHandlesGroup>(handlesSheet));
            //handle->SetSelected(selectedHandles.Contains(key.uid));

            handle->onChangedPos = [this, track](const Vec2F& pos)
            {
                mDisableHandlesUpdate = true;

                track->subTrackBeginOffset = pos.x - track->beginTime;

                mDisableHandlesUpdate = false;
            };
        }

        {
            auto handle = mRightBorderHandle->handle;
            handle->SetPosition(Vec2F(track->beginTime + track->subTrackDuration - track->subTrackEndOffset, 0.0f));
            handle->track = track;
            handle->trackPath = mTrackPath;
            handle->trackControl = Ref(this);
            handle->isMapping = false;
            handle->SetSelectionGroup(DynamicCast<ISelectableDragHandlesGroup>(handlesSheet));
            //handle->SetSelected(selectedHandles.Contains(key.uid));

            handle->onChangedPos = [this, track](const Vec2F& pos)
            {
                mDisableHandlesUpdate = true;

                track->subTrackEndOffset = (track->beginTime + track->subTrackDuration) - pos.x;

                mDisableHandlesUpdate = false;
            };
        }
    }

    void SubTrackControl::UpdateHandles()
    {
        if (mDisableHandlesUpdate)
            return;

        auto track = mTrack.Lock();
        if (!track)
            return;

        mLeftBorderHandle->handle->SetPosition(Vec2F(track->beginTime + track->subTrackBeginOffset, 0.0f));
        mRightBorderHandle->handle->SetPosition(Vec2F(track->beginTime + track->subTrackDuration - track->subTrackEndOffset, 0.0f));
        mCenterHandle->handle->SetPosition(Vec2F(track->beginTime, 0.0f));
    }

    void SubTrackControl::SerializeKey(UInt64 keyUid, DataValue& data, float relativeTime)
    {
//         if (mLeftBorderHandle->keyUid == keyUid)
//             data.Set(mLeftBorderHandle
    }

    Vector<Ref<ITrackControl::KeyHandle>> SubTrackControl::GetKeyHandles() const
    {
        Vector<Ref<KeyHandle>> res = { mLeftBorderHandle, mRightBorderHandle, mCenterHandle };
        return res;
    }

    void SubTrackControl::DeleteKey(UInt64 keyUid)
    {}

    void SubTrackControl::BeginKeysDrag()
    {}

    void SubTrackControl::EndKeysDrag()
    {}

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SubTrackControl>);
// --- META ---

DECLARE_CLASS(Editor::SubTrackControl, Editor__SubTrackControl);
// --- END META ---
