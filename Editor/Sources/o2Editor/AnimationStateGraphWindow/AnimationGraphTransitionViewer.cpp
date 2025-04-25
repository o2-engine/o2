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

		// Create drag handles
		mBeginTimeRangeHandle = mmake<DragHandle>();
		mBeginTimeRangeHandle->SetRegularDrawable(mmake<Sprite>("ui/UI2_handle_regular.png"));
		mBeginTimeRangeHandle->SetHoverDrawable(mmake<Sprite>("ui/UI2_handle_select.png"));
		mBeginTimeRangeHandle->SetPressedDrawable(mmake<Sprite>("ui/UI2_handle_pressed.png"));
		mBeginTimeRangeHandle->onChangedPos = THIS_FUNC(OnBeginTimeRangeChanged);

		mEndTimeRangeHandle = mmake<DragHandle>();
		mEndTimeRangeHandle->SetRegularDrawable(mmake<Sprite>("ui/UI2_handle_regular.png"));
		mEndTimeRangeHandle->SetHoverDrawable(mmake<Sprite>("ui/UI2_handle_select.png"));
		mEndTimeRangeHandle->SetPressedDrawable(mmake<Sprite>("ui/UI2_handle_pressed.png"));
		mEndTimeRangeHandle->onChangedPos = THIS_FUNC(OnEndTimeRangeChanged);

		mDurationHandle = mmake<DragHandle>();
		mDurationHandle->SetRegularDrawable(mmake<Sprite>("ui/UI2_handle_regular.png"));
		mDurationHandle->SetHoverDrawable(mmake<Sprite>("ui/UI2_handle_select.png"));
		mDurationHandle->SetPressedDrawable(mmake<Sprite>("ui/UI2_handle_pressed.png"));
		mDurationHandle->onChangedPos = THIS_FUNC(OnDurationChanged);

		// Initialize sprites
		mSourceRangeSprite = mmake<Sprite>("ui/UI_Window_place.png");
		mSourceRangeSprite->SetColor(Color4(50, 120, 220, 255));

		mDestinationRangeSprite = mmake<Sprite>("ui/UI_Window_place.png");
		mDestinationRangeSprite->SetColor(Color4(220, 120, 50, 255));
	}

	void AnimationGraphTransitionViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjects)
    {
        if (targetObjects.IsEmpty())
			return;

		mSpoiler->AddChild(mDurationWidget);
		mDurationWidget->SetIndexInSiblings(mSpoiler->GetChildren().Count() - 1);

        auto transition = dynamic_cast<AnimationStateGraphEditor::StateTransition*>(targetObjects.Last().first);
        if (!transition)
            return;

        mTransition = Ref(transition);

//         // Get source and destination animations durations
//         if (auto sourceState = transition->GetSourceState())
//         {
//             mSourceDuration = 1.0f; // Default value
// 
//             // Try to get animation component and find animation duration
//             if (auto graph = sourceState->GetGraph().Lock())
//             {
//                 if (auto component = graph->GetComponent().Lock())
//                 {
//                     if (auto animationComponent = component->GetAnimationComponent())
//                     {
//                         for (auto& animation : sourceState->GetAnimations())
//                         {
//                             auto state = animationComponent->GetState(animation->name);
//                             if (state && state->GetDuration() > 0)
//                             {
//                                 mSourceDuration = state->GetDuration();
//                                 break;
//                             }
//                         }
//                     }
//                 }
//             }
//         }
// 
//         if (auto destState = transition->GetDestinationState().Get())
//         {
//             mDestinationDuration = 1.0f; // Default value
// 
//             // Try to get animation component and find animation duration
//             if (auto graph = destState->GetGraph().Lock())
//             {
//                 if (auto component = graph->GetComponent().Lock())
//                 {
//                     if (auto animationComponent = component->GetAnimationComponent())
//                     {
//                         for (auto& animation : destState->GetAnimations())
//                         {
//                             auto state = animationComponent->GetState(animation->name);
//                             if (state && state->GetDuration() > 0)
//                             {
//                                 mDestinationDuration = state->GetDuration();
//                                 break;
//                             }
//                         }
//                     }
//                 }
//             }
//         }

        // Update handles positions
        UpdateHandlesPositions();
    }

    void AnimationGraphTransitionViewer::DrawDurationWidget()
    {
        if (!mTransition)
            return;

		float durationBarHeight = 25.0f;
		float durationBarsSpace = 4.0f;
		float handlesOffset = 10.0f;

		Color4 solidBarColor(50, 120, 220, 255);
		Color4 transparentBarColor(50, 120, 220, 0);

		RectF widgetRect = mDurationWidget->layout->GetWorldRect();

		// Source animation duration bars
		RectF sourceRect(widgetRect.left, widgetRect.top - handlesOffset, widgetRect.right, widgetRect.top - handlesOffset - durationBarHeight);

        float sourceDurationLeft = Math::Lerp(widgetRect.left, widgetRect.right, 0.1f);
		float sourceDurationRight = Math::Lerp(widgetRect.left, widgetRect.right, 0.9f);

		mSourceRangeSprite->SetCornerColors(transparentBarColor, solidBarColor, solidBarColor, transparentBarColor);
		mSourceRangeSprite->rect = RectF(sourceRect.left, sourceRect.top,
										 sourceDurationLeft - durationBarsSpace/2.0f, sourceRect.bottom);

        mSourceRangeSprite->Draw();


		mSourceRangeSprite->SetCornerColors(solidBarColor, solidBarColor, solidBarColor, solidBarColor);
		mSourceRangeSprite->rect = RectF(sourceDurationLeft + durationBarsSpace/2.0f, sourceRect.top,
										 sourceDurationRight - durationBarsSpace/2.0f, sourceRect.bottom);

		mSourceRangeSprite->Draw();

		mSourceRangeSprite->SetCornerColors(solidBarColor, transparentBarColor, transparentBarColor, solidBarColor);
		mSourceRangeSprite->rect = RectF(sourceDurationRight + durationBarsSpace/2.0f, sourceRect.top,
										 sourceRect.right, sourceRect.bottom);

		mSourceRangeSprite->Draw();

        
//         // Draw source animation duration bar
//         float sourceHeight = 30.0f;
//         RectF sourceRect(widgetRect.left, widgetRect.top, widgetRect.right, widgetRect.top + sourceHeight);
//         o2Render.DrawRect(sourceRect, Color4(50, 120, 220, 255));
// 
//         // Draw destination animation duration bar
//         float destHeight = 30.0f;
//         RectF destRect(widgetRect.left, widgetRect.top + sourceHeight + 10.0f, 
//                        widgetRect.right, widgetRect.top + sourceHeight + 10.0f + destHeight);
//         o2Render.DrawRect(destRect, Color4(220, 120, 50, 255));
// 
//         // Draw time markers
//         float beginPos = TimeToPosition(mTransition->beginTimeRange, true);
//         float endPos = TimeToPosition(mTransition->endTimeRange, true);
//         float durationPos = TimeToPosition(mTransition->duration, false);
// 
//         // Draw time range connection
//         o2Render.DrawLine(
//             Vec2F(widgetRect.left + beginPos, sourceRect.bottom), 
//             Vec2F(widgetRect.left + durationPos, destRect.top),
//             Color4(150, 150, 150, 200), 1.0f
//         );
// 
//         o2Render.DrawLine(
//             Vec2F(widgetRect.left + endPos, sourceRect.bottom), 
//             Vec2F(widgetRect.left + durationPos + 8.0f, destRect.top),
//             Color4(150, 150, 150, 200), 1.0f
//         );

        // Draw handles
        mBeginTimeRangeHandle->Draw();
        mEndTimeRangeHandle->Draw();
        mDurationHandle->Draw();
    }

    void AnimationGraphTransitionViewer::UpdateHandlesPositions()
    {
        if (!mTransition)
            return;

        RectF widgetRect = mDurationWidget->layout->GetWorldRect();
        
        // Set positions for handles
//         float beginPos = TimeToPosition(mTransition->beginTimeRange, true);
//         float endPos = TimeToPosition(mTransition->endTimeRange, true);
//         float durationPos = TimeToPosition(mTransition->duration, false);
// 
//         mBeginTimeRangeHandle->position = Vec2F(widgetRect.left + beginPos, widgetRect.top + 15.0f);
//         mEndTimeRangeHandle->position = Vec2F(widgetRect.left + endPos, widgetRect.top + 15.0f);
//         mDurationHandle->position = Vec2F(widgetRect.left + durationPos, widgetRect.top + 50.0f);
    }

    void AnimationGraphTransitionViewer::OnBeginTimeRangeChanged(const Vec2F& position)
    {
        if (!mTransition)
            return;

//         auto widgetRect = mDurationWidget->layout->GetWorldRect();
//         float relativeX = position.x - widgetRect.left;
//         float newTime = PositionToTime(relativeX, true);
// 
//         // Clamp to valid range
//         newTime = Math::Clamp(newTime, 0.0f, Math::Min(mTransition->endTimeRange, mSourceDuration));
//         
//         mTransition->beginTimeRange = newTime;
        UpdateHandlesPositions();
    }

    void AnimationGraphTransitionViewer::OnEndTimeRangeChanged(const Vec2F& position)
    {
        if (!mTransition)
            return;

//         auto widgetRect = mDurationWidget->layout->GetWorldRect();
//         float relativeX = position.x - widgetRect.left;
//         float newTime = PositionToTime(relativeX, true);
// 
//         // Clamp to valid range
//         newTime = Math::Clamp(newTime, Math::Max(mTransition->beginTimeRange, 0.0f), mSourceDuration);
//         
//         mTransition->endTimeRange = newTime;
        UpdateHandlesPositions();
    }

    void AnimationGraphTransitionViewer::OnDurationChanged(const Vec2F& position)
    {
        if (!mTransition)
            return;

//         auto widgetRect = mDurationWidget->layout->GetWorldRect();
//         float relativeX = position.x - widgetRect.left;
//         float newTime = PositionToTime(relativeX, false);
// 
//         // Clamp to valid range
//         newTime = Math::Clamp(newTime, 0.0f, mDestinationDuration);
//         
//         mTransition->duration = newTime;
        UpdateHandlesPositions();
    }
}
// --- META ---

DECLARE_CLASS(Editor::AnimationGraphTransitionViewer, Editor__AnimationGraphTransitionViewer);
// --- END META ---
