#include "o2Editor/stdafx.h"
#include "AnimationGraphTransitionViewer.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"

namespace Editor
{
    const Type* AnimationGraphTransitionViewer::GetViewingObjectType() const
    {
        if (mRealObjectType)
            return mRealObjectType;

        return GetViewingObjectTypeStatic();
    }

    const Type* AnimationGraphTransitionViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(AnimationStateGraphEditor::StateTransition);
    }

	void AnimationGraphTransitionViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
	{
		PushEditorScopeOnStack scope;

		DefaultObjectPropertiesViewer::RebuildProperties(targetObjets);

		// Create custom duration widget
		mDurationWidget = mmake<Widget>();
		*mDurationWidget->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 80.0f);
		mDurationWidget->layout->minHeight = 80.0f;
		mDurationWidget->onDraw = THIS_FUNC(DrawDurationWidget);
		mSpoiler->AddChild(mDurationWidget);

		// Create begin time range handle
		mBeginTimeRangeHandle = mmake<DragHandle>();
		mBeginTimeRangeHandle->SetRegularDrawable(mmake<Sprite>("ui/UI4_Right_icn.png"));
		mBeginTimeRangeHandle->SetHoverDrawable(mmake<Sprite>("ui/UI4_Right_icn_select.png"));
		mBeginTimeRangeHandle->SetPressedDrawable(mmake<Sprite>("ui/UI4_Right_icn_pressed.png"));
		mBeginTimeRangeHandle->angle = -Math::PI() / 2.0f;

		mBeginTimeRangeHandle->onChangedPos = [this](const Vec2F& pos) { 
			if (auto transition = mTransition.Lock())
				transition->transition->beginTimeRange = Math::Clamp(pos.x / mSourceDuration, 0.0f, transition->transition->endTimeRange);

			UpdateHandlesPositions();
		};

		mBeginTimeRangeHandle->localToScreenTransformFunc = [this](const Vec2F& pos) { 
			return Vec2F(TimeToPosition(pos.x), mDurationWidget->layout->worldTop - 1);
		};

		mBeginTimeRangeHandle->screenToLocalTransformFunc = [this](const Vec2F& pos) { 
			return Vec2F(PositionToTime(pos.x), 0);
		};

		// Create end time range handle
		mEndTimeRangeHandle = mmake<DragHandle>();
		mEndTimeRangeHandle->SetRegularDrawable(mmake<Sprite>("ui/UI4_Right_icn.png"));
		mEndTimeRangeHandle->SetHoverDrawable(mmake<Sprite>("ui/UI4_Right_icn_select.png"));
		mEndTimeRangeHandle->SetPressedDrawable(mmake<Sprite>("ui/UI4_Right_icn_pressed.png"));
		mEndTimeRangeHandle->angle = -Math::PI() / 2.0f;

		mEndTimeRangeHandle->onChangedPos = [this](const Vec2F& pos) { 
			if (auto transition = mTransition.Lock())
				transition->transition->endTimeRange = Math::Clamp(pos.x / mSourceDuration, transition->transition->beginTimeRange, 1.0f);

			UpdateHandlesPositions();
		};

		mEndTimeRangeHandle->localToScreenTransformFunc = mBeginTimeRangeHandle->localToScreenTransformFunc;
		mEndTimeRangeHandle->screenToLocalTransformFunc = mBeginTimeRangeHandle->screenToLocalTransformFunc;

		// Create duration handle
		mDurationHandle = mmake<DragHandle>();
		mDurationHandle->SetRegularDrawable(mmake<Sprite>("ui/UI4_Right_icn.png"));
		mDurationHandle->SetHoverDrawable(mmake<Sprite>("ui/UI4_Right_icn_select.png"));
		mDurationHandle->SetPressedDrawable(mmake<Sprite>("ui/UI4_Right_icn_pressed.png"));
		mDurationHandle->angle = Math::PI() / 2.0f;

		mDurationHandle->onChangedPos = [this](const Vec2F& pos) { 
			if (auto transition = mTransition.Lock())
				transition->transition->duration = Math::Clamp(pos.x - transition->transition->beginTimeRange * mSourceDuration, 0.0f, mDestinationDuration);

			UpdateHandlesPositions();
		};

		mDurationHandle->localToScreenTransformFunc = [this](const Vec2F& pos) { 
			return Vec2F(TimeToPosition(pos.x), mDurationWidget->layout->worldTop - mDurationBarHeight*2.0f - 9);
		};

		mDurationHandle->screenToLocalTransformFunc = [this](const Vec2F& pos) {
			return Vec2F(PositionToTime(pos.x), 0);
		};

		// Initialize sprites
		mSourceRangeSprite = mmake<Sprite>("ui/UI4_animation_bar.png");
		mDestinationRangeSprite = mmake<Sprite>("ui/UI4_animation_bar.png");

		mRangeSprite = mmake<Sprite>();
		mRangeSprite->SetColor(Color4(255, 255, 255, 125));

		// Transition mesh
		//  [0]--------[1]
		//   |          | \
		//  [2]--------[3] [4]--------[5] 
		//                \ |          |
		//                 [6]--------[7]
		mTransitionMesh = mmake<Mesh>(TextureRef(), 8, 6);
		mTransitionMesh->vertexCount = 8;
		mTransitionMesh->polyCount = 6;
		mTransitionMesh->indexes[0] = 0; mTransitionMesh->indexes[1] = 1; mTransitionMesh->indexes[2] = 2;
		mTransitionMesh->indexes[3] = 1; mTransitionMesh->indexes[4] = 3; mTransitionMesh->indexes[5] = 2;
		mTransitionMesh->indexes[6] = 1; mTransitionMesh->indexes[7] = 4; mTransitionMesh->indexes[8] = 3;
		mTransitionMesh->indexes[9] = 3; mTransitionMesh->indexes[10] = 4; mTransitionMesh->indexes[11] = 6;
		mTransitionMesh->indexes[12] = 4; mTransitionMesh->indexes[13] = 5; mTransitionMesh->indexes[14] = 6;
		mTransitionMesh->indexes[15] = 6; mTransitionMesh->indexes[16] = 5; mTransitionMesh->indexes[17] = 7;

		// State names texts
		mSourceStateName = mmake<Text>("stdFont.ttf");
		mSourceStateName->height = 12;
		mSourceStateName->verAlign = VerAlign::Middle;
		mSourceStateName->horAlign = HorAlign::Left;

		mDestinationStateName = mmake<Text>("stdFont.ttf");
		mDestinationStateName->height = 12;
		mDestinationStateName->verAlign = VerAlign::Middle;
		mDestinationStateName->horAlign = HorAlign::Left;
	}

	void AnimationGraphTransitionViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjects)
    {
        if (targetObjects.IsEmpty())
			return;

		mSpoiler->AddChild(mDurationWidget);

        auto transitionWrapper = dynamic_cast<AnimationStateGraphEditor::StateTransition*>(targetObjects.Last().first);
        if (!transitionWrapper)
            return;

        mTransition = Ref(transitionWrapper);

		mSourceDuration      = 1.0f; 
		mDestinationDuration = 1.0f; 

		auto sourceStateWrapper = transitionWrapper->owner.Lock();
		if (sourceStateWrapper)
		{
			mSourceDuration = 0.0f;

			bool animationNameUpdated = false;
			for (auto& animation : sourceStateWrapper->animations)
			{
				if (auto state = animation->state.Lock())
				{
					mSourceDuration = Math::Max(mSourceDuration, state->GetDuration());

					if (!animationNameUpdated && animation->animation)
					{
						mSourceStateName->text = animation->animation.Lock()->name;
						animationNameUpdated = true;
					}
				}
			}
		}

		auto destinationStateWrapper = transitionWrapper->destination.Lock();
		if (destinationStateWrapper)
		{
			mDestinationDuration = 0.0f;

			bool animationNameUpdated = false;
			for (auto& animation : destinationStateWrapper->animations)
			{
				if (auto state = animation->state.Lock())
				{
					mDestinationDuration = Math::Max(mDestinationDuration, state->GetDuration());

					if (!animationNameUpdated && animation->animation)
					{
						mDestinationStateName->text = animation->animation.Lock()->name;
						animationNameUpdated = true;
					}
				}
			}
		}

        // Update handles positions
        UpdateHandlesPositions();
    }

    void AnimationGraphTransitionViewer::DrawDurationWidget()
    {
        if (!mTransition || mSourceDuration < Math::Epsilon || mDestinationDuration < Math::Epsilon)
            return;

		float durationBarsSpace = 1.0f;
		float widgetTopDownBorders = 5;

		Color4 solidBarColor(255, 255, 255, 255);
		Color4 transparentBarColor(255, 255, 255, 0);

		RectF widgetRect = mDurationWidget->layout->GetWorldRect();
		widgetRect.top -= widgetTopDownBorders;
		widgetRect.bottom += widgetTopDownBorders;

		float sourceRangeBeginTime = 0.0f;
		float sourceRangeEndTime = mSourceDuration;

		float destinationRangeBeginTime = mSourceDuration * mTransition.Lock()->transition->beginTimeRange;
		float destinationRangeEndTime = destinationRangeBeginTime + mDestinationDuration;

		float sourceRangeBeginPosition = TimeToPosition(sourceRangeBeginTime);
		float sourceRangeEndPosition = TimeToPosition(sourceRangeEndTime);
		float sourceRangePositionDelta = sourceRangeEndPosition - sourceRangeBeginPosition;

		float destinationRangeBeginPosition = TimeToPosition(destinationRangeBeginTime);
		float destinationRangeEndPosition = TimeToPosition(destinationRangeEndTime);
		float destinationRangePositionDelta = destinationRangeEndPosition - destinationRangeBeginPosition;

		float destinationDurationPosition = TimeToPosition(destinationRangeBeginTime + mTransition.Lock()->transition->duration);

		// Source animation duration bars
		RectF sourceRect(widgetRect.left, widgetRect.top, 
						 widgetRect.right, widgetRect.top - mDurationBarHeight);

		mSourceRangeSprite->SetCornerColors(transparentBarColor, solidBarColor, solidBarColor, transparentBarColor);
		mSourceRangeSprite->rect = RectF(sourceRect.left, sourceRect.top,
										 sourceRangeBeginPosition - durationBarsSpace, sourceRect.bottom);

        mSourceRangeSprite->Draw();

		mSourceRangeSprite->SetCornerColors(solidBarColor, solidBarColor, solidBarColor, solidBarColor);
		float sourceRangeBeginItPosition = sourceRangeBeginPosition;
		while (sourceRangeBeginItPosition + sourceRangePositionDelta < widgetRect.right)
		{
			mSourceRangeSprite->rect = RectF(sourceRangeBeginItPosition + durationBarsSpace, sourceRect.top,
											 sourceRangeBeginItPosition + sourceRangePositionDelta - durationBarsSpace, sourceRect.bottom);

			sourceRangeBeginItPosition += sourceRangePositionDelta;

			mSourceRangeSprite->Draw();
		}

		mSourceRangeSprite->SetCornerColors(solidBarColor, transparentBarColor, transparentBarColor, solidBarColor);
		mSourceRangeSprite->rect = RectF(sourceRangeBeginItPosition + durationBarsSpace, sourceRect.top,
										 sourceRangeBeginItPosition + mHandlesOffset, sourceRect.bottom);

		mSourceRangeSprite->Draw();

		// Destination animation duration bars	
		RectF destRect(widgetRect.left, widgetRect.top - mDurationBarHeight - durationBarsSpace, 
					   widgetRect.right, widgetRect.top - 2*mDurationBarHeight - durationBarsSpace);

		mDestinationRangeSprite->SetCornerColors(solidBarColor, solidBarColor, solidBarColor, solidBarColor);
		float destinationRangeBeginItPosition = destinationRangeBeginPosition;
		while (destinationRangeBeginItPosition + destinationRangePositionDelta < widgetRect.right)
		{
			mDestinationRangeSprite->rect = RectF(destinationRangeBeginItPosition + durationBarsSpace, destRect.top,
												  destinationRangeBeginItPosition + destinationRangePositionDelta - durationBarsSpace, destRect.bottom);
			destinationRangeBeginItPosition += destinationRangePositionDelta;
			mDestinationRangeSprite->Draw();
		}

		mDestinationRangeSprite->SetCornerColors(solidBarColor, transparentBarColor, transparentBarColor, solidBarColor);
		mDestinationRangeSprite->rect = RectF(destinationRangeBeginItPosition + durationBarsSpace, destRect.top,
											  destinationRangeBeginItPosition + mHandlesOffset, destRect.bottom);

		mDestinationRangeSprite->Draw();

		// Draw range sprite
		mRangeSprite->rect = RectF(destinationRangeBeginPosition, sourceRect.top,
								   mEndTimeRangeHandle->GetScreenPosition().x, sourceRect.bottom);
		mRangeSprite->Draw();

		// Transition mesh
		//  [0]--------[1]
		//   |          | \
		//  [2]--------[3] [4]--------[5] 
		//                \ |          |
		//                 [6]--------[7]
		auto transitionColor = Color4(0, 156, 141, 255).ABGR();
		mTransitionMesh->vertices[0] = Vertex(sourceRangeBeginPosition + 2, sourceRect.top - 1, transitionColor, 0, 0);
		mTransitionMesh->vertices[2] = Vertex(sourceRangeBeginPosition + 2, sourceRect.bottom + 1, transitionColor, 0, 0);

		mTransitionMesh->vertices[1] = Vertex(destinationRangeBeginPosition, sourceRect.top - 1, transitionColor, 0, 0);
		mTransitionMesh->vertices[3] = Vertex(destinationRangeBeginPosition, sourceRect.bottom + 1, transitionColor, 0, 0);

		mTransitionMesh->vertices[4] = Vertex(destinationDurationPosition, destRect.top - 1, transitionColor, 0, 0);
		mTransitionMesh->vertices[6] = Vertex(destinationDurationPosition, destRect.bottom + 1, transitionColor, 0, 0);

		float transitionEndPos = destinationRangeBeginPosition + destinationRangePositionDelta - durationBarsSpace - 1;
		mTransitionMesh->vertices[5] = Vertex(transitionEndPos, destRect.top - 1, transitionColor, 0, 0);
		mTransitionMesh->vertices[7] = Vertex(transitionEndPos, destRect.bottom + 1, transitionColor, 0, 0);
		mTransitionMesh->Draw();

		// Draw state names
		float textBorder = 5.0f;
		mSourceStateName->rect = RectF(sourceRangeBeginPosition + textBorder, sourceRect.top,
									   sourceRangeEndPosition - textBorder, sourceRect.bottom);

		mSourceStateName->Draw();

		mDestinationStateName->rect = RectF(destinationRangeBeginPosition + textBorder, destRect.top,
											destinationRangeEndPosition - textBorder, destRect.bottom);

		mDestinationStateName->Draw();

		// Draw handles
		mEndTimeRangeHandle->Draw();
        mBeginTimeRangeHandle->Draw();
        mDurationHandle->Draw();
    }

    void AnimationGraphTransitionViewer::UpdateHandlesPositions()
    {
		auto transition = mTransition.Lock();
		if (!transition)
			return;

		mBeginTimeRangeHandle->position = Vec2F(transition->transition->beginTimeRange * mSourceDuration, 0);
		mEndTimeRangeHandle->position = Vec2F(transition->transition->endTimeRange * mSourceDuration, 0);
		mDurationHandle->position = Vec2F(transition->transition->duration + transition->transition->beginTimeRange * mSourceDuration, 0);
    }

	float AnimationGraphTransitionViewer::TimeToPosition(float time) const
	{
		RectF widgetRect = mDurationWidget->layout->GetWorldRect();
		float sumDuration = mSourceDuration + mDestinationDuration;
		float durationScale = (widgetRect.Width() - mHandlesOffset * 2.0f) / sumDuration;

		return time * durationScale + widgetRect.left + mHandlesOffset;
	}

	float AnimationGraphTransitionViewer::PositionToTime(float position) const
	{
		RectF widgetRect = mDurationWidget->layout->GetWorldRect();
		float sumDuration = mSourceDuration + mDestinationDuration;
		float durationScale = (widgetRect.Width() - mHandlesOffset * 2.0f) / sumDuration;

		return (position - widgetRect.left - mHandlesOffset) / durationScale;
	}
}
// --- META ---

DECLARE_CLASS(Editor::AnimationGraphTransitionViewer, Editor__AnimationGraphTransitionViewer);
// --- END META ---
