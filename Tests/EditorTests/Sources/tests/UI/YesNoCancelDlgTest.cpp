#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"

#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<o2::Window> DialogWindow()
    {
        EnsureEditorUIRoot();
        if (!YesNoCancelDlg::IsSingletonInitialzed())
        {
            PushEditorScopeOnStack scope;
            mmake<YesNoCancelDlg>();
        }

        return EditorUIRoot.GetRootWidget()->FindChildByTypeAndName<o2::Window>("Confirmation window");
    }

    void Press(const Ref<o2::Window>& window, const String& buttonName)
    {
        auto button = window->FindChildByTypeAndName<Button>(buttonName);
        ASSERT_TRUE(button != nullptr) << buttonName;
        button->onClick();
    }
}

// An answer's callback may ask the next question: the dialog must show it, not close it with the first answer
TEST(YesNoCancelDlg, AnswerCallbackCanAskTheNextQuestion)
{
    auto window = DialogWindow();
    ASSERT_TRUE(window != nullptr);

    int secondAnswers = 0;
    bool firstCancelled = false;
    YesNoCancelDlg::ShowYesNoCancel("first", []() {},
                                    [&]() { YesNoCancelDlg::ShowYesNo("second", [&]() { secondAnswers++; }); },
                                    [&]() { firstCancelled = true; });

    Press(window, "No button");

    EXPECT_TRUE(window->IsEnabled());
    EXPECT_FALSE(firstCancelled);
    auto message = window->FindChildByType<Label>();
    ASSERT_TRUE(message != nullptr);
    EXPECT_EQ((String)message->GetText(), String("second"));

    Press(window, "Yes button");

    EXPECT_EQ(secondAnswers, 1);
    EXPECT_FALSE(window->IsEnabled());
}
