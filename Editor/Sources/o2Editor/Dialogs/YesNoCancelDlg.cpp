#include "o2Editor/stdafx.h"
#include "YesNoCancelDlg.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::YesNoCancelDlg);

namespace Editor
{
    YesNoCancelDlg::YesNoCancelDlg(RefCounter* refCounter):
        Singleton<YesNoCancelDlg>(refCounter), CursorEventsListener(refCounter)
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Confirmation")));

        InitializeControls();

        mWindow->Hide(true);
        *mWindow->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(350, 150));

        mWindow->GetBackCursorListener().onCursorReleased = [&](const Input::Cursor& c) { OnCursorPressedOutside(); };
        mWindow->onHide = MakeFunction(this, &YesNoCancelDlg::OnHide);
    }

    YesNoCancelDlg::~YesNoCancelDlg()
    {}

    void YesNoCancelDlg::Show(const String& message, 
                              const Function<void()>& onYes, 
                              const Function<void()>& onNo /*= Function<void()>()*/,
							  const Function<void()>& onCancel /*= Function<void()>()*/)
    {
        mInstance->mMessageLabel->SetText(message);
        mInstance->mWindow->ShowModal();
        mInstance->mOnYesCallback = onYes;
        mInstance->mOnNoCallback = onNo;
        mInstance->mOnCancelCallback = onCancel;
    }

    void YesNoCancelDlg::OnHide()
    {
        mOnCancelCallback();
    }

    void YesNoCancelDlg::InitializeControls()
    {
        auto verLayout = o2UI.CreateVerLayout();
        verLayout->spacing = 15;

        mMessageLabel = o2UI.CreateLabel("Confirmation message");
        mMessageLabel->SetHorAlign(HorAlign::Middle);
        verLayout->AddChild(mMessageLabel);

        auto horLayout = o2UI.CreateHorLayout();
        horLayout->spacing = 10;
        
        auto yesButton = o2UI.CreateButton("Yes", MakeFunction(this, &YesNoCancelDlg::OnYesPressed));
        horLayout->AddChild(yesButton);

        auto noButton = o2UI.CreateButton("No", MakeFunction(this, &YesNoCancelDlg::OnNoPressed));
        horLayout->AddChild(noButton);

        auto cancelButton = o2UI.CreateButton("Cancel", MakeFunction(this, &YesNoCancelDlg::OnCancelPressed));
        horLayout->AddChild(cancelButton);

        verLayout->AddChild(horLayout);

        mWindow->AddChild(verLayout);
    }

    void YesNoCancelDlg::OnYesPressed()
    {
        mOnYesCallback();
        mWindow->Hide();
    }

    void YesNoCancelDlg::OnNoPressed()
    {
        mOnNoCallback();
        mWindow->Hide();
    }

    void YesNoCancelDlg::OnCancelPressed()
    {
        mOnCancelCallback();
        mWindow->Hide();
    }

    void YesNoCancelDlg::OnCursorPressedOutside()
    {
        mOnCancelCallback();
        mWindow->Hide();
    }
} 