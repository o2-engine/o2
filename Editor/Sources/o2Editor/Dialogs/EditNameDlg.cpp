#include "o2Editor/stdafx.h"
#include "EditNameDlg.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::NameEditDlg);

namespace Editor
{
    NameEditDlg::NameEditDlg(RefCounter* refCounter):
        Singleton<NameEditDlg>(refCounter), CursorEventsListener(refCounter)
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Enter name")));

        InitializeControls();

        mWindow->Hide(true);
        mWindow->layout->size = Vec2F(300, 100);

        mWindow->GetBackCursorListener().onCursorReleased = [&](const Input::Cursor& c) { OnCursorPressedOutside(); };
        mWindow->onHide = MakeFunction(this, &NameEditDlg::OnHide);
    }

    NameEditDlg::~NameEditDlg()
    {}

    void NameEditDlg::Show(const String& name, Function<void(const String&)> onCompleted, 
                           Function<void()> onCancelled /*= Function<void()>()*/)
    {
        mInstance->mNameEditBox->SetText(name);
        mInstance->mWindow->ShowModal();
        mInstance->mOnCompletedCallback = onCompleted;
        mInstance->mOnCancelledCallback = onCancelled;
    }

    void NameEditDlg::OnHide()
    {
        mOnCancelledCallback();
    }

    void NameEditDlg::InitializeControls()
    {
        auto verLayout = o2UI.CreateVerLayout();
        verLayout->spacing = 10;

        mNameEditBox = o2UI.CreateEditBox("singleline");
        verLayout->AddChild(mNameEditBox);

        auto horLayout =  o2UI.CreateHorLayout();
        horLayout->spacing = 10;
        
        auto okButton = o2UI.CreateButton("Ok", MakeFunction(this, &NameEditDlg::OnOkPressed));
        horLayout->AddChild(okButton);

        auto cancelButton = o2UI.CreateButton("Cancel", MakeFunction(this, &NameEditDlg::OnCancelPressed));
        horLayout->AddChild(cancelButton);

        verLayout->AddChild(horLayout);

        mWindow->AddChild(verLayout);
    }

    void NameEditDlg::OnOkPressed()
    {
        mOnCompletedCallback(mNameEditBox->GetText());
        mWindow->Hide();
    }

    void NameEditDlg::OnCancelPressed()
    {
        mOnCancelledCallback();
        mWindow->Hide();
    }

    void NameEditDlg::OnCursorPressedOutside()
    {
        mOnCancelledCallback();
        mWindow->Hide();
    }

}
