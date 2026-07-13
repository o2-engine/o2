#include "o2Editor/stdafx.h"
#include "CurveEditorDlg.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/UI/CurveEditor/CurvesEditor.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::CurveEditorDlg);

namespace Editor
{
    CurveEditorDlg::CurveEditorDlg(RefCounter* refCounter):
        Singleton<CurveEditorDlg>(refCounter), CursorEventsListener(refCounter)
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Curve editor")));

        InitializeControls();

        mWindow->Hide(true);
        mWindow->layout->size2D = Vec2F(600, 500);

        mWindow->GetBackCursorListener().onCursorReleased = [&](const Input::Cursor& c) { OnCursorPressedOutside(); };
        mWindow->onHide = MakeFunction(this, &CurveEditorDlg::OnHide);
    }

    CurveEditorDlg::~CurveEditorDlg()
    {}

    void CurveEditorDlg::OnHide()
    {
        mOnChangedCallback();
        mOnChangeCompletedCallback();
    }

    void CurveEditorDlg::InitializeControls()
    {
        mEditorWidget = mmake<CurvesEditor>();
        *mEditorWidget->layout = WidgetLayout::BothStretch(0, 5, 5, 0);

        auto horScroll = o2UI.CreateHorScrollBar();
        *horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 10, -10);
        mEditorWidget->SetHorScrollbar(horScroll);

        auto verScroll = o2UI.CreateVerScrollBar();
        *verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 10, -10);
        mEditorWidget->SetVerScrollbar(verScroll);

        mEditorWidget->SetMainHandleImages(AssetRef<ImageAsset>("ui/CurveHandle.png"),
                                           AssetRef<ImageAsset>("ui/CurveHandleHover.png"),
                                           AssetRef<ImageAsset>("ui/CurveHandlePressed.png"),
                                           AssetRef<ImageAsset>("ui/CurveHandleSelected.png"));

        mEditorWidget->SetSupportHandleImages(AssetRef<ImageAsset>("ui/CurveSupportHandle.png"),
                                              AssetRef<ImageAsset>("ui/CurveSupportHandleHover.png"),
                                              AssetRef<ImageAsset>("ui/CurveSupportHandlePressed.png"),
                                              AssetRef<ImageAsset>("ui/CurveSupportHandleSelected.png"));

        mEditorWidget->SetRangeHandleImages(AssetRef<ImageAsset>("ui/UI2_handle_side_regular.png"),
                                              AssetRef<ImageAsset>("ui/UI2_handle_side_select.png"),
                                              AssetRef<ImageAsset>("ui/UI2_handle_side_pressed.png"),
                                              AssetRef<ImageAsset>("ui/UI2_handle_side_select.png"));

        mEditorWidget->SetSelectionSpriteImage(AssetRef<ImageAsset>("ui/UI_Window_place.png"));

		mWindow->onFocused += [&]() { mEditorWidget->OnFocusedByRoot(); };
		mWindow->onUnfocused += [&]() { mEditorWidget->OnUnfocusedByRoot(); };

        mWindow->AddChild(mEditorWidget);
    }

    void CurveEditorDlg::Show(const Function<void()>& onChanged, const Function<void()>& onCompleted /*= Function<void()>()*/)
    {
        mInstance->mWindow->ShowModal();
        mInstance->mOnChangedCallback = onChanged;
        mInstance->mOnChangeCompletedCallback = onCompleted;
        mInstance->mEditorWidget->RemoveAllCurves();
    }

    void CurveEditorDlg::AddEditingCurve(const String& id, const Ref<Curve>& curve, const Color4& color /*= Color4::Green()*/)
    {
        mInstance->mEditorWidget->AddCurve(id, curve, color);
    }

    void CurveEditorDlg::RemoveEditingCurve(const Ref<Curve>& curve)
    {
        mInstance->mEditorWidget->RemoveCurve(curve);
    }

    void CurveEditorDlg::RemoveEditingCurve(const String& id)
    {
        mInstance->mEditorWidget->RemoveCurve(id);
    }

    void CurveEditorDlg::RemoveAllEditingCurves()
    {
        mInstance->mEditorWidget->RemoveAllCurves();
    }

    void CurveEditorDlg::OnCursorPressedOutside()
    {
        mOnChangedCallback();
        mWindow->Hide();
    }

}
