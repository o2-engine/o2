#include "o2Editor/stdafx.h"
#include "KeyEditDlg.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::KeyEditDlg);

namespace Editor
{

    KeyEditDlg::KeyEditDlg(RefCounter* refCounter):
        Singleton<KeyEditDlg>(refCounter)
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Edit key")));

        auto verLayout = o2UI.CreateVerLayout();
        verLayout->spacing = 10;
        verLayout->expandHeight = false;
        verLayout->fitByChildren = false;
        *verLayout->layout = WidgetLayout::BothStretch(0, 30, 0, 0);
        mWindow->AddChild(verLayout);

        // Position and value
        {
            auto horLayout = o2UI.CreateHorLayout();
            horLayout->spacing = 10;
            horLayout->expandHeight = false;
            horLayout->fitByChildren = true;

            {
                auto positionVerLayout = o2UI.CreateVerLayout();
                positionVerLayout->expandHeight = false;
                positionVerLayout->fitByChildren = true;
                positionVerLayout->AddChild(o2UI.CreateLabel("Position"));

                mPosition = o2UI.CreateEditBox("singleline");
                mPosition->name = "position";
                mPosition->SetFilterFloat();
                positionVerLayout->AddChild(mPosition);
                horLayout->AddChild(positionVerLayout);
            }

            {
                auto valueVerLayout = o2UI.CreateVerLayout();
                valueVerLayout->expandHeight = false;
                valueVerLayout->fitByChildren = true;
                valueVerLayout->AddChild(o2UI.CreateLabel("Value"));

                mValue = o2UI.CreateEditBox("singleline");
                mValue->SetFilterFloat();
                mValue->name = "value";
                valueVerLayout->AddChild(mValue);
                horLayout->AddChild(valueVerLayout);
            }

            verLayout->AddChild(horLayout);
        }

        // left support
        {
            auto label = o2UI.CreateLabel("Left support:");
            auto horLayout = o2UI.CreateHorLayout();
            horLayout->spacing = 10;
            horLayout->expandHeight = false;
            horLayout->fitByChildren = true;

            {
                auto positionVerLayout = o2UI.CreateVerLayout();
                positionVerLayout->expandHeight = false;
                positionVerLayout->fitByChildren = true;
                positionVerLayout->AddChild(o2UI.CreateLabel("Position"));

                mLeftSupportPosition = o2UI.CreateEditBox("singleline");
                mLeftSupportPosition->name = "leftSupportPosition";
                mLeftSupportPosition->SetFilterFloat();
                positionVerLayout->AddChild(mLeftSupportPosition);
                horLayout->AddChild(positionVerLayout);
            }

            {
                auto valueVerLayout = o2UI.CreateVerLayout();
                valueVerLayout->expandHeight = false;
                valueVerLayout->fitByChildren = true;
                valueVerLayout->AddChild(o2UI.CreateLabel("Value"));

                mLeftSupportValue = o2UI.CreateEditBox("singleline");
                mLeftSupportValue->name = "leftSupportValue";
                mLeftSupportValue->SetFilterFloat();
                valueVerLayout->AddChild(mLeftSupportValue);
                horLayout->AddChild(valueVerLayout);
            }

            verLayout->AddChild(label);
            verLayout->AddChild(horLayout);
        }

        // right support
        {
            auto label = o2UI.CreateLabel("Right support:");
            auto horLayout = o2UI.CreateHorLayout();
            horLayout->spacing = 10;
            horLayout->expandHeight = false;
            horLayout->fitByChildren = true;

            {
                auto positionVerLayout = o2UI.CreateVerLayout();
                positionVerLayout->expandHeight = false;
                positionVerLayout->fitByChildren = true;
                positionVerLayout->AddChild(o2UI.CreateLabel("Position"));

                mRightSupportPosition = o2UI.CreateEditBox("singleline");
                mRightSupportPosition->name = "rightSupportPosition";
                mRightSupportPosition->SetFilterFloat();
                positionVerLayout->AddChild(mRightSupportPosition);
                horLayout->AddChild(positionVerLayout);
            }

            {
                auto valueVerLayout = o2UI.CreateVerLayout();
                valueVerLayout->expandHeight = false;
                valueVerLayout->fitByChildren = true;
                valueVerLayout->AddChild(o2UI.CreateLabel("Value"));

                mRightSupportValue = o2UI.CreateEditBox("singleline");
                mRightSupportValue->name = "rightSupportValue";
                mRightSupportValue->SetFilterFloat();
                valueVerLayout->AddChild(mRightSupportValue);
                horLayout->AddChild(valueVerLayout);
            }

            verLayout->AddChild(label);
            verLayout->AddChild(horLayout);
        }

        {
            auto horLayout = o2UI.CreateHorLayout();
            horLayout->spacing = 10;
            horLayout->expandHeight = true;
            *horLayout->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 25);

            auto okButton = o2UI.CreateButton("Ok", MakeFunction(this, &KeyEditDlg::OnOkPressed));
            horLayout->AddChild(okButton);

            auto cancelButton = o2UI.CreateButton("Cancel", [&]() { mWindow->Hide(); });
            horLayout->AddChild(cancelButton);

            mWindow->AddChild(horLayout);
        }

        mWindow->Hide(true);
        mWindow->layout->size2D = Vec2F(200, 300);

        mWindow->GetBackCursorListener().onCursorReleased = [&](const Input::Cursor& c) { mWindow->Hide(); };
    }

    KeyEditDlg::~KeyEditDlg()
    {}

    void KeyEditDlg::Show(const Curve::Key& key, const Function<void(const Curve::Key& key)>& onClosed)
    {
        Instance().mPosition->SetText(String(key.position));
        Instance().mValue->SetText(String(key.value));
        Instance().mLeftSupportPosition->SetText(String(key.leftSupportPosition));
        Instance().mLeftSupportValue->SetText(String(key.leftSupportValue));
        Instance().mRightSupportPosition->SetText(String(key.rightSupportPosition));
        Instance().mRightSupportValue->SetText(String(key.rightSupportValue));

        Instance().mOnClosed = onClosed;

        Instance().mWindow->ShowModal();
    }

    void KeyEditDlg::OnOkPressed()
    {
        Curve::Key key;
        key.position = (float)mPosition->GetText();
        key.value = (float)mValue->GetText();
        key.leftSupportPosition = (float)mLeftSupportPosition->GetText();
        key.leftSupportValue = (float)mLeftSupportValue->GetText();
        key.rightSupportPosition = (float)mRightSupportPosition->GetText();
        key.rightSupportValue = (float)mRightSupportValue->GetText();

        mOnClosed(key);
        mWindow->Hide();
    }

}
