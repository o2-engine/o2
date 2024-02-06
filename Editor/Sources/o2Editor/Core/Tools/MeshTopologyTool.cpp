#include "o2Editor/stdafx.h"
#include "MeshTopologyTool.h"

#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
	MeshTopologyTool::MeshTopologyTool():
		mSelectionSprite("ui/UI_Window_place.png")
	{
		sceneLayer.tool = this;

		mHandleSample = DragHandle(mnew Sprite("ui/CurveHandle.png"),
								   mnew Sprite("ui/CurveHandleHover.png"),
								   mnew Sprite("ui/CurveHandlePressed.png"),
								   mnew Sprite("ui/CurveHandleSelected.png"));

		typedef MeshTopologyTool thisclass;

		mTransformFrame.SetPivotEnabled(false);
		mTransformFrame.SetRotationEnabled(false);
		mTransformFrame.onTransformed = THIS_FUNC(OnTransformFrameTransformed);
		// 		mTransformFrame.onPressed = THIS_FUNC(OnTransformBegin);
		// 		mTransformFrame.onChangeCompleted = THIS_FUNC(OnTransformCompleted);
		mTransformFrame.isInputTransparent = true;
	}

	MeshTopologyTool::~MeshTopologyTool()
	{
		ClearHandles();
	}

	void MeshTopologyTool::Setup(const Function<Vector<Vec2F>()>& getPoints, const Function<void(int, Vec2F)>& setPoint,
								 const Function<Basis()>& getTransform, const Function<void(Vec2F)>& addPoint,
								 const Function<void(int)>& removePoint)
	{
		mGetPoints = getPoints;
		mSetPoint = setPoint;
		mAddPoint = addPoint;
		mRemovePoint = removePoint;
		mGetTransform = getTransform;

		InitializeHandles();
	}

	void MeshTopologyTool::Reset()
	{
		mGetPoints.Clear();
		mSetPoint.Clear();
		mAddPoint.Clear();
		mRemovePoint.Clear();
		mGetTransform.Clear();

		ClearHandles();
	}

	String MeshTopologyTool::GetPanelIcon() const
	{
		return "ui/TopologyTool.png";
	}

	void MeshTopologyTool::OnEnabled()
	{
		o2EditorSceneScreen.AddEditorLayer(&sceneLayer);
		isEnabled = true;
	}

	void MeshTopologyTool::OnDisabled()
	{
		o2EditorSceneScreen.RemoveEditorLayer(&sceneLayer);
		isEnabled = false;
	}

	bool MeshTopologyTool::IsUnderPoint(const Vec2F& point)
	{
		return true;
	}

	bool MeshTopologyTool::IsInputTransparent() const
	{
		return true;
	}

	void MeshTopologyTool::InitializeHandles()
	{
		ClearHandles();

		for (int i = 0; i < mGetPoints().Count(); i++)
		{
			mHandles.Add(mHandleSample.CloneAs<DragHandle>());
			auto& newHandle = mHandles.Last();

			newHandle->SetPosition(mGetPoints()[i]);
			newHandle->SetSelectionGroup(this);
			newHandle->onChangedPos = [=](const Vec2F& pos) { OnHandleMoved(i, pos); };
			newHandle->localToScreenTransformFunc = [&](const Vec2F& p) { return LocalToWorld(p); };
			newHandle->screenToLocalTransformFunc = [&](const Vec2F& p) { return WorldToLocal(p); };
		}
	}

	void MeshTopologyTool::ClearHandles()
	{
		for (auto& handle : mHandles)
			delete handle;

		mHandles.Clear();
	}

	void MeshTopologyTool::OnHandleMoved(int i, const Vec2F& pos)
	{
		mSetPoint(i, pos);
		UpdateTransformFrame();
	}

	Vec2F MeshTopologyTool::WorldToLocal(const Vec2F& point) const
	{
		Basis transform = mGetTransform();
		return o2EditorSceneScreen.ScreenToLocalPoint(point)*transform.Inverted();
	}

	Vec2F MeshTopologyTool::LocalToWorld(const Vec2F& point) const
	{
		Basis transform = mGetTransform();
		return o2EditorSceneScreen.LocalToScreenPoint(point*transform);
	}

	void MeshTopologyTool::DrawSelection()
	{
		if (mIsPressed)
		{
			mSelectionSprite.rect = RectF(LocalToWorld(mSelectingPressedPoint), o2Input.cursorPos);
			mSelectionSprite.Draw();
		}
	}

	void MeshTopologyTool::DrawTransformFrame()
	{
		if (!mTransformFrameVisible)
			return;

		Vec2F worldOrig = LocalToWorld(mTransformFrameBasis.origin);
		Vec2F worldXV = LocalToWorld(mTransformFrameBasis.xv + mTransformFrameBasis.origin) - worldOrig;
		Vec2F worldYV = LocalToWorld(mTransformFrameBasis.yv + mTransformFrameBasis.origin) - worldOrig;
		mTransformFrame.SetBasis(Basis(worldOrig - mTransformBasisOffet,
									   worldXV + Vec2F(mTransformBasisOffet.x*2.0f, 0),
									   worldYV + Vec2F(0, mTransformBasisOffet.y*2.0f)));

		mTransformFrame.Draw();
	}

	void MeshTopologyTool::UpdateTransformFrame()
	{
		mTransformFrameVisible = IsTransformFrameVisible();

		if (!mTransformFrameVisible || mSelectedHandles.IsEmpty())
			return;

		RectF aabb((mSelectedHandles[0])->GetPosition(), (mSelectedHandles[0])->GetPosition());

		for (auto& handle : mSelectedHandles)
		{
			aabb.left = Math::Min(handle->GetPosition().x, aabb.left);
			aabb.right = Math::Max(handle->GetPosition().x, aabb.right);
			aabb.top = Math::Max(handle->GetPosition().y, aabb.top);
			aabb.bottom = Math::Min(handle->GetPosition().y, aabb.bottom);
		}

		mTransformFrameBasis = Basis(aabb.LeftBottom(), Vec2F::Right()*aabb.Width(), Vec2F::Up()*aabb.Height());
	}

	bool MeshTopologyTool::IsTransformFrameVisible() const
	{
		return mSelectedHandles.Count() > 1;
	}

	void MeshTopologyTool::OnTransformFrameTransformed(const Basis& basis)
	{
		Vec2F localBasisOrig = WorldToLocal(basis.origin + mTransformBasisOffet);
		Vec2F localBasisXV = WorldToLocal(basis.xv - Vec2F(mTransformBasisOffet.x*2.0f, 0) + basis.origin + mTransformBasisOffet) - localBasisOrig;
		Vec2F localBasisYV = WorldToLocal(basis.yv - Vec2F(0, mTransformBasisOffet.y*2.0f) + basis.origin + mTransformBasisOffet) - localBasisOrig;
		Basis localBasis(localBasisOrig, localBasisXV, localBasisYV);

		Basis delta = mTransformFrameBasis.Inverted()*localBasis;

		if (delta.origin.Length() > 0.01f || delta.xv != Vec2F(1, 0) || delta.yv != Vec2F(0, 1))
		{
			Vector<Vec2F> newPosHandlesPositions, newSupportnHandlesPositions;

			for (int i = 0; i < mSelectedHandles.Count(); i++)
				newPosHandlesPositions.Add(mSelectedHandles[i]->GetPosition()*delta);

			for (int i = 0; i < mSelectedHandles.Count(); i++)
			{
				mSelectedHandles[i]->SetPosition(newPosHandlesPositions[i]);
				mSelectedHandles[i]->onChangedPos(mSelectedHandles[i]->GetPosition());
			}

			UpdateTransformFrame();
		}
	}

	void MeshTopologyTool::OnCursorPressed(const Input::Cursor& cursor)
	{
		mSelectingPressedPoint = WorldToLocal(cursor.position);

		if (!o2Input.IsKeyDown(VK_CONTROL))
			DeselectAll();
	}

	void MeshTopologyTool::OnCursorReleased(const Input::Cursor& cursor)
	{
		for (auto& handle : mSelectingHandlesBuf)
		{
			SetHandleSelectedState(handle, false);
			handle->SetSelected(true);
		}

		mSelectingHandlesBuf.Clear();
		UpdateTransformFrame();
	}

	void MeshTopologyTool::OnCursorStillDown(const Input::Cursor& cursor)
	{
		for (auto& handle : mSelectingHandlesBuf)
			SetHandleSelectedState(handle, false);

		mSelectingHandlesBuf.Clear();

		RectF selectionLocalRect(mSelectingPressedPoint, WorldToLocal(cursor.position));

		for (auto& handle : mHandles)
		{
			if (handle->IsEnabled() && selectionLocalRect.IsInside(handle->GetPosition()) &&
				!mSelectedHandles.Contains(handle))
			{
				mSelectingHandlesBuf.Add(handle);
				SetHandleSelectedState(handle, true);
			}
		}
	}

	void MeshTopologyTool::OnCursorDblClicked(const Input::Cursor& cursor)
	{
		auto local = WorldToLocal(cursor.position);
		mAddPoint(local);
		InitializeHandles();
	}

	void MeshTopologyTool::SceneLayer::DrawOverScene()
	{
		tool->OnDrawn();

		tool->DrawTransformFrame();

		for (auto& handle : tool->mHandles)
			handle->Draw();

		tool->DrawSelection();
	}

	void MeshTopologyTool::SceneLayer::Update(float dt)
	{

	}

	int MeshTopologyTool::SceneLayer::GetOrder() const
	{
		return 0;
	}

	bool MeshTopologyTool::SceneLayer::IsEnabled() const
	{
		return tool->isEnabled;
	}

	const String& MeshTopologyTool::SceneLayer::GetName() const
	{
		static String res("Topology");
		return res;
	}

	const String& MeshTopologyTool::SceneLayer::GetIconName() const
	{
		return String::empty;
	}
}
// --- META ---

DECLARE_CLASS(Editor::MeshTopologyTool, Editor__MeshTopologyTool);
// --- END META ---
