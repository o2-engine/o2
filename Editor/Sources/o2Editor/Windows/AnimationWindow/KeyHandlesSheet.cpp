#include "o2Editor/stdafx.h"
#include "KeyHandlesSheet.h"

#include "o2/Animation/Tracks/AnimationTrack.h"
#include "o2/Animation/AnimationClip.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Windows/AnimationWindow/AnimationKeysActions.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/ITrackControl.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"

namespace Editor
{

    KeyHandlesSheet::KeyHandlesSheet(RefCounter* refCounter) :
        Widget(refCounter), SelectableDragHandlesGroup(refCounter)
    {
        mSelectionFrame = mmake<Sprite>("ui/UI4_keys_select.png");
        mSelectionFrame->enabled = false;

        InitializeHandles();
        InitializeContextMenu();
    }

    KeyHandlesSheet::KeyHandlesSheet(RefCounter* refCounter, const KeyHandlesSheet& other) :
        Widget(refCounter, other), SelectableDragHandlesGroup(refCounter), mSelectionFrame(other.mSelectionFrame->CloneAs<Sprite>())
    {
        InitializeHandles();
        InitializeContextMenu();
    }

    KeyHandlesSheet::~KeyHandlesSheet()
    {}

    KeyHandlesSheet& KeyHandlesSheet::operator=(const KeyHandlesSheet& other)
    {
        Widget::operator=(other);

        mSelectionFrame = other.mSelectionFrame->CloneAsRef<Sprite>();

        return *this;
    }

    void KeyHandlesSheet::Update(float dt)
    {
        Widget::Update(dt);
    }

#undef DrawText
    void KeyHandlesSheet::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        Widget::OnDrawn();
		CursorAreaEventsListener::OnDrawn();

		DrawDebugFrame();
    }

	void KeyHandlesSheet::DrawSelectionFrame()
	{
		o2Render.EnableScissorTest(layout->GetWorldRect());

		if (mSelectionFrame->enabled)
		{
			auto offsets = mIsFrameSelecting ? mSelectionFrameCursorOffsets : mSelectionFrameOffsets;
			auto animationWindow = mAnimationWindow.Lock();
			RectF selectionFrameRect = RectF(animationWindow->mTimeline->LocalToWorld(mSelectionRect.left) + offsets.left,
				animationWindow->mTree->GetLineWorldPosition(mSelectionRect.top) + offsets.top,
				animationWindow->mTimeline->LocalToWorld(mSelectionRect.right) + offsets.right,
				animationWindow->mTree->GetLineWorldPosition(mSelectionRect.bottom) + offsets.bottom);

			mSelectionFrame->SetRect(selectionFrameRect);
			mSelectionFrame->Draw();

			mLeftFrameDragHandle->SetDrawablesSize(Vec2F(10, selectionFrameRect.Height()));
			mRightFrameDragHandle->SetDrawablesSize(Vec2F(10, selectionFrameRect.Height()));

			if (!mIsFrameSelecting)
			{
				mCenterFrameDragHandle->Draw();
				mLeftFrameDragHandle->Draw();
				mRightFrameDragHandle->Draw();
			}
		}

		o2Render.DisableScissorTest();

		if (mNeedUpdateSelectionFrame)
			UpdateSelectionFrame();
	}

	void KeyHandlesSheet::UpdateInputDrawOrder()
    {
        if (!mResEnabledInHierarchy)
            return;

        CursorAreaEventsListener::OnDrawn();

        for (auto& handle : mHandles)
        {
            if (handle->IsEnabled())
                handle->CursorAreaEventsListener::OnDrawn();
        }

        if (mSelectionFrame->enabled)
        {
            mCenterFrameDragHandle->CursorAreaEventsListener::OnDrawn();

            if (!Math::Equals(mSelectionRect.left, mSelectionRect.right))
            {
                mLeftFrameDragHandle->CursorAreaEventsListener::OnDrawn();
                mRightFrameDragHandle->CursorAreaEventsListener::OnDrawn();
            }
        }
    }

    void KeyHandlesSheet::SetAnimation(const Ref<AnimationClip>& animation)
    {
        mHandlesGroups.Clear();
    }

    bool KeyHandlesSheet::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    void KeyHandlesSheet::RegTrackControl(const Ref<ITrackControl>& trackControl, const std::string& path)
    {
        mTrackControls.Add(trackControl);
        mTrackControlsMap.Add({ path, trackControl });
    }

    void KeyHandlesSheet::UnregTrackControl(const Ref<ITrackControl>& trackControl)
    {
        mTrackControls.Remove(trackControl);
        mTrackControlsMap.RemoveAll([=](auto& p) { return p.second == trackControl; });
    }

    void KeyHandlesSheet::UnregAllTrackControls()
    {
        mTrackControls.Clear();
        mTrackControlsMap.Clear();
        mHandles.Clear();
    }

    void KeyHandlesSheet::AddHandle(const Ref<DragHandle>& handle)
    {
        if (auto animHandle = DynamicCast<AnimationKeyDragHandle>(handle))
        {
            auto track = animHandle->track.Lock();
            if (!mHandlesGroups.ContainsKey(track))
                mHandlesGroups.Add(track, {});

            mHandlesGroups[track].Add(animHandle);
        }

        SelectableDragHandlesGroup::AddHandle(handle);
    }

    void KeyHandlesSheet::RemoveHandle(DragHandle* handle)
    {
        if (auto animHandle = dynamic_cast<AnimationKeyDragHandle*>(handle))
            mHandlesGroups[animHandle->track.Lock()].RemoveFirst([&](auto& x) { return x == animHandle; });

        SelectableDragHandlesGroup::RemoveHandle(handle);
    }

    void KeyHandlesSheet::SetSelectedKeys(const Map<String, Vector<UInt64>>& keys)
    {
        DeselectAll();

        for (auto& handlesGroup : mHandlesGroups)
        {
            for (auto& handle : handlesGroup.second)
            {
                if (keys.ContainsKey(handle->trackPath) && keys.Get(handle->trackPath).Contains(handle->keyUid))
                    SelectHandle(handle);
            }
        }
    }

    Map<String, Vector<UInt64>> KeyHandlesSheet::GetSelectedKeys() const
    {
        Map<String, Vector<UInt64>> res;
        for (auto& handlesGroup : mHandlesGroups)
        {
            for (auto& handle : handlesGroup.second)
            {
                if (handle->selected)
                    res[handle->trackPath].Add(handle->keyUid);
            }
        }

        return res;
    }

    const Ref<ContextMenu>& KeyHandlesSheet::GetContextMenu() const
    {
        return mContextMenu;
    }

    String KeyHandlesSheet::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    Ref<RefCounterable> KeyHandlesSheet::CastToRefCounterable(const Ref<KeyHandlesSheet>& ref)
    {
        return DynamicCast<Widget>(ref);
    }

    void KeyHandlesSheet::OnSelectionChanged()
    {
        mNeedUpdateSelectionFrame = true;
    }

    void KeyHandlesSheet::OnHandleCursorPressed(const Ref<DragHandle>& handle, const Input::Cursor& cursor)
    {
        mBeforeChangeKeysData.Clear();
        SerializeKeys(mBeforeChangeKeysData, GetSelectedKeys(), 0);

        for (auto& trackControl : mTrackControls)
            trackControl->BeginKeysDrag();

        if (!GetSelectedHandles().Contains(handle) && handle != mCenterFrameDragHandle &&
            handle != mLeftFrameDragHandle && handle != mRightFrameDragHandle)
        {
            if (!o2Input.IsKeyDown(VK_CONTROL))
                DeselectAll();

            SelectHandle(handle);
        }

        for (auto& handle : GetSelectedHandles())
            handle->BeginDrag(cursor.position);

        mHandleHasMoved = false;
    }

    void KeyHandlesSheet::OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor)
    {
        SelectableDragHandlesGroup::OnHandleCursorReleased(handle, cursor);

        for (auto& trackControl : mTrackControls)
            trackControl->EndKeysDrag();

        if (mSelectedHandles.Count() == 1 && !mHandleHasMoved)
            mAnimationWindow.Lock()->mTimeline->SetTimeCursor(mSelectedHandles[0]->GetPosition().x);

        DataDocument data;
        auto selectedKeys = GetSelectedKeys();
        SerializeKeys(data, selectedKeys, 0);
        mAnimationWindow.Lock()->DoneAction(mmake<AnimationKeysChangeAction>(selectedKeys, mBeforeChangeKeysData,
                                                                                           data, Ref(this)));
    }

    void KeyHandlesSheet::OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos)
    {
        auto animationWindow = mAnimationWindow.Lock();
        if (!animationWindow)
            return;

        auto animation = animationWindow->mAnimation.Lock();
        if (!animation)
            return;

        for (auto& track : animation->GetTracks())
            track->BeginKeysBatchChange();

        for (auto& kv : mHandlesGroups)
        {
            for (auto& handle : kv.second)
            {
                if (!mSelectedHandles.Contains(handle))
                    continue;

                if (handle->isMapping && kv.second.Contains([=](auto x) { return !x->isMapping && x->id == handle->id; }))
                    continue;

                handle->SetDragPosition(handle->ScreenToLocal(cursorPos) + handle->GetDraggingOffset());
                handle->onChangedPos(handle->GetPosition());
            }
        }

        for (auto& track : animation->GetTracks())
            track->CompleteKeysBatchingChange();

        mHandleHasMoved = true;
        mNeedUpdateSelectionFrame = true;
    }

    void KeyHandlesSheet::InitializeHandles()
    {
        InitializeCenterHandle();
        InitializeLeftHandle();
        InitializeRightHandle();
    }

    void KeyHandlesSheet::InitializeCenterHandle()
    {
        mCenterFrameDragHandle = mmake<DragHandle>();

        mCenterFrameDragHandle->localToScreenTransformFunc = [=](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                return Vec2F(animationWindow->mTimeline->LocalToWorld(point.x), animationWindow->mTree->GetLineWorldPosition(point.y));
            };

        mCenterFrameDragHandle->screenToLocalTransformFunc = [=](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                return Vec2F(animationWindow->mTimeline->WorldToLocal(point.x), animationWindow->mTree->GetLineNumber(point.y));
            };

        mCenterFrameDragHandle->isPointInside = [=](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                auto local = Vec2F(animationWindow->mTimeline->WorldToLocal(point.x), animationWindow->mTree->GetLineNumber(point.y));
                return local.x > mSelectionRect.left && local.x < mSelectionRect.right && local.y > mSelectionRect.top && local.y < mSelectionRect.bottom;
            };

        mCenterFrameDragHandle->checkPositionFunc = [&](const Vec2F& point) { return Vec2F(point.x, mSelectionRect.Center().y); };
        mCenterFrameDragHandle->onPressed = [&]() { OnHandleCursorPressed(mCenterFrameDragHandle, *o2Input.GetCursor());  };
        mCenterFrameDragHandle->onReleased = [&]() { OnHandleCursorReleased(mCenterFrameDragHandle, *o2Input.GetCursor()); };
        mCenterFrameDragHandle->onChangedPos = [&](const Vec2F& point) { OnHandleMoved(mCenterFrameDragHandle, o2Input.GetCursorPos()); };
        mCenterFrameDragHandle->onRightButtonReleased = [&](auto& x) { mContextMenu->Show(); };
        mCenterFrameDragHandle->cursorType = CursorType::SizeWE;
    }

    void KeyHandlesSheet::InitializeLeftHandle()
    {
        mLeftFrameDragHandle = mmake<DragHandle>();

        mLeftFrameDragHandle->SetDrawablesSizePivot(Vec2F(7, 21));

        mLeftFrameDragHandle->localToScreenTransformFunc = [&](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                return Vec2F(animationWindow->mTimeline->LocalToWorld(point.x), animationWindow->mTree->GetLineWorldPosition(point.y));
            };

        mLeftFrameDragHandle->screenToLocalTransformFunc = [&](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                return Vec2F(animationWindow->mTimeline->WorldToLocal(point.x), animationWindow->mTree->GetLineNumber(point.y));
            };

        mLeftFrameDragHandle->isPointInside = [&](const Vec2F& point)
            {
                auto r = mSelectionFrame->GetRect();
                return RectF(r.left - 5, r.bottom, r.left + 5, r.top).IsInside(point);
            };

        mLeftFrameDragHandle->checkPositionFunc = [&](const Vec2F& point) { return Vec2F(point.x, mSelectionRect.Center().y); };
        mLeftFrameDragHandle->onPressed = [&]() { OnHandleCursorPressed(mCenterFrameDragHandle, *o2Input.GetCursor()); };
        mLeftFrameDragHandle->onReleased = [&]() { OnHandleCursorReleased(mCenterFrameDragHandle, *o2Input.GetCursor()); };

        mLeftFrameDragHandle->onChangedPos = [&](const Vec2F& point)
            {
                if (Math::Equals(mSelectionRect.Width(), 0.0f))
                    return;

                auto animationWindow = mAnimationWindow.Lock();
                if (!animationWindow)
                    return;
                    
                auto animation = animationWindow->mAnimation.Lock();
                if (!animation)
                    return;
                    
                float scale = (point.x - mSelectionRect.right) / (mSelectionRect.left - mSelectionRect.right);

                for (auto& track : animation->GetTracks())
                    track->BeginKeysBatchChange();

                for (auto& handle : GetSelectedHandles())
                {
                    handle->SetPosition(Vec2F((handle->GetPosition().x - mSelectionRect.right)*scale + mSelectionRect.right, handle->GetPosition().y));
                    handle->onChangedPos(handle->GetPosition());
                }

                for (auto& track : animation->GetTracks())
                    track->CompleteKeysBatchingChange();

                mNeedUpdateSelectionFrame = true;
            };

        mLeftFrameDragHandle->onRightButtonReleased = [&](auto& x) { mContextMenu->Show(); };
        mLeftFrameDragHandle->cursorType = CursorType::SizeWE;
    }

    void KeyHandlesSheet::InitializeRightHandle()
    {
        mRightFrameDragHandle = mmake<DragHandle>();

        mRightFrameDragHandle->SetDrawablesSizePivot(Vec2F(6, 19));
        mRightFrameDragHandle->angle = Math::Deg2rad(180.0f);

        mRightFrameDragHandle->localToScreenTransformFunc = [&](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                return Vec2F(animationWindow->mTimeline->LocalToWorld(point.x), animationWindow->mTree->GetLineWorldPosition(point.y));
            };

        mRightFrameDragHandle->screenToLocalTransformFunc = [&](const Vec2F& point)
            {
                auto animationWindow = mAnimationWindow.Lock();
                return Vec2F(animationWindow->mTimeline->WorldToLocal(point.x), animationWindow->mTree->GetLineNumber(point.y));
            };

        mRightFrameDragHandle->isPointInside = [&](const Vec2F& point)
            {
                auto r = mSelectionFrame->GetRect();
                return RectF(r.right - 5, r.bottom, r.right + 5, r.top).IsInside(point);
            };

        mRightFrameDragHandle->checkPositionFunc = [&](const Vec2F& point) { return Vec2F(point.x, mSelectionRect.Center().y); };
        mRightFrameDragHandle->onPressed = [&]() { OnHandleCursorPressed(mCenterFrameDragHandle, *o2Input.GetCursor()); };
        mRightFrameDragHandle->onReleased = [&]() { OnHandleCursorReleased(mCenterFrameDragHandle, *o2Input.GetCursor()); };

        mRightFrameDragHandle->onChangedPos = [&](const Vec2F& point)
            {
                if (Math::Equals(mSelectionRect.Width(), 0.0f))
                    return;

                auto animationWindow = mAnimationWindow.Lock();
                if (!animationWindow)
                    return;
                    
                auto animation = animationWindow->mAnimation.Lock();
                if (!animation)
                    return;

                float scale = (point.x - mSelectionRect.left) / (mSelectionRect.right - mSelectionRect.left);

                for (auto& track : animation->GetTracks())
                    track->BeginKeysBatchChange();

                for (auto& handle : GetSelectedHandles())
                {
                    handle->SetPosition(Vec2F((handle->GetPosition().x - mSelectionRect.left)*scale + mSelectionRect.left, handle->GetPosition().y));
                    handle->onChangedPos(handle->GetPosition());
                }

                for (auto& track : animation->GetTracks())
                    track->CompleteKeysBatchingChange();

                mNeedUpdateSelectionFrame = true;
            };

        mRightFrameDragHandle->onRightButtonReleased = [&](auto& x) { mContextMenu->Show(); };
        mRightFrameDragHandle->cursorType = CursorType::SizeWE;
    }

    void KeyHandlesSheet::InitializeContextMenu()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();

        mContextMenu->AddItem("Copy", [&]() { CopyKeys(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_C, VK_CTRL_CMD}));
        mContextMenu->AddItem("Cut", [&]() { CopyKeys(); DeleteKeys(GetSelectedKeys()); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_X, VK_CTRL_CMD}));
        mContextMenu->AddItem("Paste", [&]() { PasteKeys(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_V, VK_CTRL_CMD}));
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Delete", [&]() { DeleteKeys(GetSelectedKeys()); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_DELETE}));
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Select all", [&]() { SelectAll(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_A, VK_CTRL_CMD}));
        mContextMenu->AddItem("Deselect all", [&]() { DeselectAll(); });
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Undo", [&]() { mAnimationWindow.Lock()->UndoAction(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_Z, VK_CTRL_CMD}));
        mContextMenu->AddItem("Redo", [&]() { mAnimationWindow.Lock()->RedoAction(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_Y, VK_CTRL_CMD}));

        AddChild(mContextMenu);

        mIsFocusable = true;
    }

    void KeyHandlesSheet::UpdateSelectionFrame()
    {
        mNeedUpdateSelectionFrame = false;

        if (!mIsFrameSelecting && mSelectedHandles.Count() > 0)
        {
            mSelectionFrame->enabled = true;

            auto animationWindow = mAnimationWindow.Lock();

            mSelectionRect.left = mSelectedHandles.First()->GetPosition().x;
            mSelectionRect.bottom = animationWindow->mTree->GetLineNumber(mSelectedHandles.First()->GetScreenPosition().y);
            mSelectionRect.right = mSelectionRect.left;
            mSelectionRect.top = mSelectionRect.bottom;

            for (auto& handle : mSelectedHandles)
            {
                float localPos = handle->GetPosition().x;
                float lineNumber = animationWindow->mTree->GetLineNumber(handle->GetScreenPosition().y);

                mSelectionRect.left = Math::Min(mSelectionRect.left, localPos);
                mSelectionRect.right = Math::Max(mSelectionRect.right, localPos);

                mSelectionRect.bottom = Math::Max(mSelectionRect.bottom, Math::Ceil(lineNumber));
                mSelectionRect.top = Math::Min(mSelectionRect.top, Math::Floor(lineNumber - 0.5f));
            }

            mCenterFrameDragHandle->position = mSelectionRect.Center();
            mLeftFrameDragHandle->position = Vec2F(mSelectionRect.left, mSelectionRect.Center().y);
            mRightFrameDragHandle->position = Vec2F(mSelectionRect.right, mSelectionRect.Center().y);
        }
        else 
            mSelectionFrame->enabled = false;
    }

    void KeyHandlesSheet::SerializeKeys(DataValue& data, const Map<String, Vector<UInt64>>& keys, float relativeTime)
    {
        Map<String, Pair<DataValue*, Vector<UInt64>>> serializedKeysUids;

        for (auto& kv : keys)
        {
            auto fnd = mTrackControlsMap.FindOrDefault([=](const Pair<String, Ref<ITrackControl>>& p) { return p.first == kv.first; });
            if (!fnd.second)
                continue;

            DataValue& trackData = data.AddMember("KeysGroup");
            trackData.AddMember("Path") = kv.first;
            DataValue& keysData = trackData.AddMember("Keys");

            for (auto& handle : fnd.second->GetKeyHandles())
            {
                if (kv.second.Contains(handle->keyUid))
                {
                    auto node = keysData.AddMember("key");
                    fnd.second->SerializeKey(handle->keyUid, node, relativeTime);
                }
            }
        }
    }

    void KeyHandlesSheet::DeserializeKeys(const DataValue& data, Map<String, Vector<UInt64>>& keys, float relativeTime,
                                          bool generateNewUid /*= true*/)
    {
        auto animationWindow = mAnimationWindow.Lock();
        if (!animationWindow)
            return;
            
        auto animation = animationWindow->mAnimation.Lock();
        if (!animation)
            return;

        for (auto& track : animation->GetTracks())
            track->BeginKeysBatchChange();

        if (data.GetMembersCount() == 1 && animationWindow->mTree->GetSelectedObjects().Count() == 1)
        {
            auto DataValue = (AnimationTree::TrackNode*)animationWindow->mTree->GetSelectedObjects()[0];
            for (auto& keyNode : data[0].GetMember("Keys"))
            {
                UInt64 uid = DataValue->trackControl->DeserializeKey(keyNode, relativeTime);
                if (uid != 0)
                    keys[DataValue->path].Add(uid);
            }
        }
        else
        {
            for (auto& pathNode : data)
            {
                String path = pathNode.GetMember("Path");
                for (auto& kv : mTrackControlsMap)
                {
                    if (kv.first != path)
                        continue;

                    for (auto& keyNode : pathNode.GetMember("Keys"))
                    {
                        UInt64 uid = kv.second->DeserializeKey(keyNode, relativeTime, generateNewUid);
                        if (uid != 0)
                            keys[path].Add(uid);
                    }
                }
            }
        }

        for (auto& track : animation->GetTracks())
            track->CompleteKeysBatchingChange();
    }

    void KeyHandlesSheet::CopyKeys()
    {
        float relativeTime = mAnimationWindow.Lock()->mTimeline->WorldToLocal(o2Input.GetCursorPos().x);
        Map<String, Vector<UInt64>> keys = GetSelectedKeys();

        DataDocument data;
        SerializeKeys(data, keys, relativeTime);

        Clipboard::SetText(data.SaveAsString());
    }

    void KeyHandlesSheet::PasteKeys()
    {
        auto animationWindow = mAnimationWindow.Lock();

        DeselectAll();
        Map<String, Vector<UInt64>> keys;

        DataDocument data;
        data.LoadFromData(Clipboard::GetText());

        DeserializeKeys(data, keys, animationWindow->mTimeline->WorldToLocal(o2Input.GetCursorPos().x));
        SetSelectedKeys(keys);

        data.Clear();
        SerializeKeys(data, keys, 0);
        animationWindow->DoneAction(mmake<AnimationAddKeysAction>(keys, data, Ref(this)));
    }

    void KeyHandlesSheet::DeleteKeys(const Map<String, Vector<UInt64>>& keys, bool createAction /*= true*/)
    {
        auto animationWindow = mAnimationWindow.Lock();

        if (createAction)
        {
            DataDocument data;
            SerializeKeys(data, keys, 0);
            animationWindow->DoneAction(mmake<AnimationDeleteKeysAction>(keys, data, Ref(this)));
        }

        auto animation = animationWindow->mAnimation.Lock();
        if (!animation)
            return;
            
        for (auto& track : animation->GetTracks())
            track->BeginKeysBatchChange();

        for (auto& handlesGroup : mHandlesGroups)
        {
            for (auto& handle : handlesGroup.second)
            {
                if (keys.ContainsKey(handle->trackPath) && keys.Get(handle->trackPath).Contains(handle->keyUid))
                {
                    handle->Deselect();
                    handle->trackControl.Lock()->DeleteKey(handle->keyUid);
                }
            }
        }

        for (auto& track : animation->GetTracks())
            track->CompleteKeysBatchingChange();
    }

    void KeyHandlesSheet::OnCursorPressed(const Input::Cursor& cursor)
    {
        auto animationWindow = mAnimationWindow.Lock();

        if (!o2Input.IsKeyDown(VK_CONTROL))
            DeselectAll();

        mBeginSelectHandles = mSelectedHandles;

        mBeginSelectPoint.x = animationWindow->mTimeline->WorldToLocal(cursor.position.x);
        mBeginSelectPoint.y = animationWindow->mTree->GetLineNumber(cursor.position.y);

        Focus();
        mContextMenu->SetItemsMaxPriority();
    }

    void KeyHandlesSheet::OnCursorReleased(const Input::Cursor& cursor)
    {
        mIsFrameSelecting = false;
        mNeedUpdateSelectionFrame = true;
    }

    void KeyHandlesSheet::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        mIsFrameSelecting = false;
        mNeedUpdateSelectionFrame = true;
    }

    void KeyHandlesSheet::OnCursorPressedOutside(const Input::Cursor& cursor)
    {

    }

    void KeyHandlesSheet::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {

    }

    void KeyHandlesSheet::OnCursorStillDown(const Input::Cursor& cursor)
    {
        auto animationWindow = mAnimationWindow.Lock();

        if (cursor.isPressed)
        {
            if (!mIsFrameSelecting)
            {
                if (cursor.delta != Vec2F())
                    mIsFrameSelecting = true;
            }


            if (mIsFrameSelecting)
            {
                mSelectionFrame->enabled = true;

                Vec2F current(animationWindow->mTimeline->WorldToLocal(cursor.position.x), animationWindow->mTree->GetLineNumber(cursor.position.y));
                mSelectionRect.left = Math::Min(mBeginSelectPoint.x, current.x);
                mSelectionRect.right = Math::Max(mBeginSelectPoint.x, current.x);
                mSelectionRect.top = Math::Floor(Math::Min(mBeginSelectPoint.y, current.y));
                mSelectionRect.bottom = Math::Ceil(Math::Max(mBeginSelectPoint.y, current.y));

                DeselectAll();

                for (auto& handle : mBeginSelectHandles)
                    SelectHandle(handle);

                for (auto& handle : mHandles)
                {
                    Vec2F handlePos(handle->GetPosition().x, animationWindow->mTree->GetLineNumber(handle->GetScreenPosition().y));
                    if (handlePos.x > mSelectionRect.left && handlePos.x < mSelectionRect.right && handlePos.y > mSelectionRect.top && handlePos.y < mSelectionRect.bottom + 0.5f)
                    {
                        SelectHandle(handle);
                    }
                }
            }
        }
    }

    void KeyHandlesSheet::OnCursorMoved(const Input::Cursor& cursor)
    {}

    void KeyHandlesSheet::OnCursorEnter(const Input::Cursor& cursor)
    {

    }

    void KeyHandlesSheet::OnCursorExit(const Input::Cursor& cursor)
    {

    }

    void KeyHandlesSheet::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        auto treeNode = mAnimationWindow.Lock()->mTree->GetTreeNodeUnderPoint(cursor.position);
        if (!treeNode)
            return;

        auto animTreeNode = DynamicCast<AnimationTreeNode>(treeNode);
        animTreeNode->OnDoubleClicked(cursor);
    }

    void KeyHandlesSheet::OnCursorRightMousePressed(const Input::Cursor& cursor)
    {
        Focus();
        mContextMenu->SetItemsMaxPriority();
        mContextMenuPressPoint = cursor.position;
    }

    void KeyHandlesSheet::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {}

    void KeyHandlesSheet::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        const float threshold = 5.0f;
        if ((cursor.position - mContextMenuPressPoint).Length() < threshold)
            mContextMenu->Show();
    }

    void KeyHandlesSheet::OnCursorMiddleMousePressed(const Input::Cursor& cursor)
    {

    }

    void KeyHandlesSheet::OnCursorMiddleMouseStayDown(const Input::Cursor& cursor)
    {

    }

    void KeyHandlesSheet::OnCursorMiddleMouseReleased(const Input::Cursor& cursor)
    {

    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::KeyHandlesSheet>);
// --- META ---

DECLARE_CLASS(Editor::KeyHandlesSheet, Editor__KeyHandlesSheet);
// --- END META ---
