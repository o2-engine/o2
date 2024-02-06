#include "o2Editor/stdafx.h"

#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/AnimationWindow/AnimationWindow.h"
#include "o2Editor/AnimationWindow/CurvesSheet.h"
#include "o2Editor/AnimationWindow/Timeline.h"
#include "o2Editor/AnimationWindow/Tree.h"

namespace Editor
{

	CurvesSheet::CurvesSheet()
	{
		InitializeControls();
	}

	CurvesSheet::CurvesSheet(const CurvesSheet& other)
	{
		InitializeControls();
	}

	CurvesSheet::~CurvesSheet()
	{}

	CurvesSheet& CurvesSheet::operator=(const CurvesSheet& other)
	{
		delete mCurvesEditor;

		Widget::operator=(other);
		InitializeControls();

		return *this;
	}

	void CurvesSheet::SetAnimation(AnimationClip* animation)
	{
		mCurvesEditor->RemoveAllCurves();

		for (auto& track : animation->GetTracks())
		{
			if (auto floatTrack = DynamicCast<AnimationTrack<float>>(track))
				mCurvesEditor->AddCurve(track->path, &floatTrack->curve);
			else if (auto vec2Track = DynamicCast<AnimationTrack<Vec2F>>(track))
				mCurvesEditor->AddCurve(track->path, &vec2Track->timeCurve);
		}

		UpdateCurvesColors();

		mCurvesEditor->actionsListDelegate = &mAnimationWindow->mActionsList;
	}

	void CurvesSheet::UpdateCurvesColors()
	{
		Color4 curveColor(44, 62, 80);
		int colorIdx = 0;
		for (auto& track : mAnimationWindow->mAnimation->GetTracks())
		{
			if (auto floatTrack = DynamicCast<AnimationTrack<float>>(track))
			{
				mAnimationWindow->mTree->SetAnimationValueColor(track->path, curveColor);
				mCurvesEditor->SetCurveColor(&floatTrack->curve, curveColor);
				curveColor = Color4::SomeColor(colorIdx++);
			}
			else if (auto vec2Track = DynamicCast<AnimationTrack<Vec2F>>(track))
			{
				mAnimationWindow->mTree->SetAnimationValueColor(track->path, curveColor);
				mCurvesEditor->SetCurveColor(&vec2Track->timeCurve, curveColor);
				curveColor = Color4::SomeColor(colorIdx++);
			}
		}
	}

	void CurvesSheet::OnAnimationChanged()
	{
		bool changed = false;

		// Check new curves
		auto currentCurves = mCurvesEditor->GetCurves();
		Vector<Curve*> animCurves;
		for (auto& track : mAnimationWindow->mAnimation->GetTracks())
		{
			Curve* curve = nullptr;
			if (auto floatTrack = DynamicCast<AnimationTrack<float>>(track))
				curve = &floatTrack->curve;
			else if (auto vec2Track = DynamicCast<AnimationTrack<Vec2F>>(track))
				curve = &vec2Track->timeCurve;

			if (curve)
			{
				animCurves.Add(curve);

				if (!currentCurves.ContainsValue(curve))
				{
					mCurvesEditor->AddCurve(track->path, curve);
					changed = true;
				}
			}
		}

		// Check removed curves
		for (auto& curve : mCurvesEditor->GetCurves())
		{
			if (!animCurves.Contains(curve.second))
			{
				mCurvesEditor->RemoveCurve(curve.second);
				changed = true;
			}
		}

		if (changed)
			UpdateCurvesColors();
	}

	String CurvesSheet::GetCreateMenuCategory()
	{
		return "UI/Editor";
	}

	void CurvesSheet::InitializeControls()
	{
		mCurvesEditor = mnew CurvesEditor();
		*mCurvesEditor->layout = WidgetLayout::BothStretch();

		auto horScroll = o2UI.CreateHorScrollBar();
		*horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 10, 10, 10, 0);
		mCurvesEditor->SetHorScrollbar(horScroll);

		auto verScroll = o2UI.CreateVerScrollBar();
		*verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 10, 10, 10, 0);
		mCurvesEditor->SetVerScrollbar(verScroll);

		mCurvesEditor->SetTextBorder(BorderF(0, 10, 10, 0));

		mCurvesEditor->SetMainHandleImages(ImageAssetRef("ui/CurveHandle.png"),
										   ImageAssetRef("ui/CurveHandleHover.png"),
										   ImageAssetRef("ui/CurveHandlePressed.png"),
										   ImageAssetRef("ui/CurveHandleSelected.png"));

		mCurvesEditor->SetSupportHandleImages(ImageAssetRef("ui/CurveSupportHandle.png"),
											  ImageAssetRef("ui/CurveSupportHandleHover.png"),
											  ImageAssetRef("ui/CurveSupportHandlePressed.png"),
											  ImageAssetRef("ui/CurveSupportHandleSelected.png"));

		mCurvesEditor->SetSelectionSpriteImage(ImageAssetRef("ui/UI_Window_place.png"));

		mCurvesEditor->verGridEnabled = false;
		mCurvesEditor->onViewChanged += THIS_FUNC(OnEditorViewChanged);

		mCurvesEditor->GetContextMenu()->AddItem("---");
		mCurvesEditor->GetContextMenu()->AddToggleItem("Normalize curves", true, [&](bool val) { 
			mCurvesEditor->SetAdjustCurvesScale(val);
		});

		AddChild(mCurvesEditor);
	}

	void CurvesSheet::SetCameraAsTimelineView()
	{
		Camera camera = mCurvesEditor->GetCamera();
		RectF camRect = camera.GetRect();
		camRect.left = mAnimationWindow->mTimeline->WorldToLocal(mAnimationWindow->mTimeline->layout->worldLeft);
		camRect.right = mAnimationWindow->mTimeline->WorldToLocal(mAnimationWindow->mTimeline->layout->worldRight);

		camera.SetRect(camRect, false);

		mEditorViewLock = true;
		mCurvesEditor->SetCamera(camera);
		mEditorViewLock = false;
	}

	void CurvesSheet::OnEditorViewChanged()
	{
		if (mEditorViewLock || !IsEnabled())
			return;

		mAnimationWindow->mTimeline->SetViewRange(mCurvesEditor->GetCamera().GetRect().left, mCurvesEditor->GetCamera().GetRect().right);
	}

}
// --- META ---

DECLARE_CLASS(Editor::CurvesSheet, Editor__CurvesSheet);
// --- END META ---
