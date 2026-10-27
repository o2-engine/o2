#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<o2::Window> QuestionWindow()
    {
        EnsureEditorUIRoot();
        if (!YesNoCancelDlg::IsSingletonInitialzed())
        {
            PushEditorScopeOnStack scope;
            mmake<YesNoCancelDlg>();
        }

        return EditorUIRoot.GetRootWidget()->FindChildByTypeAndName<o2::Window>("Confirmation window");
    }

    void Answer(const String& buttonName)
    {
        auto button = QuestionWindow()->FindChildByTypeAndName<Button>(buttonName);
        ASSERT_TRUE(button != nullptr) << buttonName;
        button->onClick();
    }
}

TEST(EditorTheme, NamesRoundTrip)
{
    EXPECT_EQ(EditorTheme::KindByName("dark"), EditorThemeKind::Dark);
    EXPECT_EQ(EditorTheme::KindByName("light"), EditorThemeKind::Light);
    EXPECT_EQ(EditorTheme::KindByName(""), EditorThemeKind::Light);
    EXPECT_EQ(EditorTheme::KindByName("solarized"), EditorThemeKind::Light);

    EXPECT_EQ(EditorTheme::NameOf(EditorThemeKind::Dark), "dark");
    EXPECT_EQ(EditorTheme::NameOf(EditorThemeKind::Light), "light");
}

TEST(EditorTheme, StartsLight)
{
    EXPECT_EQ(EditorTheme::GetKind(), EditorThemeKind::Light);
    EXPECT_FALSE(EditorTheme::IsDark());
    EXPECT_FALSE(EditorTheme::Get().dark);
    EXPECT_EQ(EditorTheme::StylesFolder(), "Editor UI styles");

    EditorPreferences preferences;
    EXPECT_EQ(preferences.theme, EditorThemeKind::Light);
}

// The light palette is the o2 look the code drew with before the themes
TEST(EditorTheme, LightPaletteIsTheOldLook)
{
    auto& light = EditorTheme::Of(EditorThemeKind::Light);
    EXPECT_EQ(light.text, Color4(96, 125, 139));
    EXPECT_EQ(light.ink, Color4(44, 62, 80));
    EXPECT_EQ(light.accent, Color4(0, 156, 141));
    EXPECT_EQ(light.viewBack, Color4(225, 232, 232));
    EXPECT_EQ(light.viewGrid, Color4(190, 190, 190));
    EXPECT_EQ(light.scaleLine, Color4(16, 20, 23, 64));
    EXPECT_EQ(light.frame, Color4(44, 62, 80));
    EXPECT_EQ(light.frameObject, Color4(44, 62, 80, 150));
    EXPECT_EQ(light.checkerA, Color4(1.0f, 1.0f, 1.0f, 1.0f));
    EXPECT_EQ(light.checkerB, Color4(0.7f, 0.7f, 0.7f, 1.0f));
    EXPECT_EQ(light.prototypes[0], Color4(0, 130, 220));
}

TEST(EditorTheme, DarkPaletteIsDark)
{
    auto& dark = EditorTheme::Of(EditorThemeKind::Dark);
    auto luminance = [](const Color4& c) { return 0.2126f*c.r + 0.7152f*c.g + 0.0722f*c.b; };

    EXPECT_TRUE(dark.dark);
    EXPECT_LT(luminance(dark.viewBack), 40.0f);
    EXPECT_LT(luminance(dark.checkerA), 40.0f);
    EXPECT_GT(luminance(dark.text), 200.0f);
    EXPECT_GT(luminance(dark.ink), 150.0f);
}

TEST(EditorTheme, LightKeepsTheStyleColors)
{
    for (auto& c : { Color4(96, 125, 139), Color4(0, 156, 141), Color4(225, 232, 232), Color4(0, 0, 0, 20),
                     Color4(248, 94, 72), Color4(12, 34, 56, 78) })
    {
        EXPECT_EQ(EditorTheme::StyleColor(c), c);
    }
}

TEST(EditorTheme, ConfigKeepsTheChoice)
{
    EditorConfig::GlobalConfig config;
    EXPECT_EQ(config.GetTheme(), "light");

    config.SetTheme("dark");
    DataDocument doc;
    doc = config;

    EditorConfig::GlobalConfig loaded;
    loaded = doc;
    EXPECT_EQ(loaded.GetTheme(), "dark");
}

TEST(EditorTheme, PipelineFollowsTheEditor)
{
    PipelineTheme::FollowEditor();
    EXPECT_EQ(PipelineTheme::GetKind(), PipelineThemeKind::Light);

    int version = PipelineTheme::GetVersion();
    PipelineTheme::SetKind(PipelineThemeKind::Dark);
    EXPECT_EQ(PipelineTheme::GetKind(), PipelineThemeKind::Dark);
    EXPECT_NE(PipelineTheme::GetVersion(), version);

    PipelineTheme::FollowEditor();
    EXPECT_EQ(PipelineTheme::GetKind(), PipelineThemeKind::Light);
}

// A theme takes effect after a restart: it is saved only when the question is answered yes
TEST(EditorThemeChoice, ConfirmedThemeIsSavedForTheNextStart)
{
    auto window = QuestionWindow();
    ASSERT_TRUE(window != nullptr);

    EditorThemeChoice choice;
    Vector<EditorThemeKind> saved;
    choice.onConfirmed = [&](EditorThemeKind kind) { saved.Add(kind); };
    choice.Reset(EditorThemeKind::Light);

    choice.Request(EditorThemeKind::Dark);
    EXPECT_TRUE(window->IsEnabled());
    EXPECT_TRUE(choice.IsAsking());
    auto message = window->FindChildByType<Label>();
    ASSERT_TRUE(message != nullptr);
    EXPECT_TRUE(((String)message->GetText()).Contains("restart"));
    EXPECT_TRUE(saved.IsEmpty());

    Answer("Yes button");
    EXPECT_FALSE(choice.IsAsking());
    EXPECT_EQ(choice.GetNext(), EditorThemeKind::Dark);
    EXPECT_EQ(choice.preferences.theme, EditorThemeKind::Dark);
    EXPECT_EQ(saved, Vector<EditorThemeKind>({ EditorThemeKind::Dark }));
    EXPECT_EQ(EditorTheme::GetKind(), EditorThemeKind::Light) << "the running editor keeps its theme";
}

TEST(EditorThemeChoice, DeclinedThemePutsThePreferencesBack)
{
    QuestionWindow();

    EditorThemeChoice choice;
    int saved = 0;
    choice.onConfirmed = [&](EditorThemeKind) { saved++; };
    choice.Reset(EditorThemeKind::Light);

    choice.preferences.theme = EditorThemeKind::Dark;
    choice.Update();
    EXPECT_TRUE(choice.IsAsking());

    choice.Update();
    Answer("No button");
    EXPECT_FALSE(choice.IsAsking());
    EXPECT_EQ(choice.GetNext(), EditorThemeKind::Light);
    EXPECT_EQ(choice.preferences.theme, EditorThemeKind::Light);
    EXPECT_EQ(saved, 0);

    choice.Update();
    EXPECT_FALSE(choice.IsAsking());
}

TEST(EditorThemeChoice, SavedThemeAsksNothing)
{
    QuestionWindow();

    EditorThemeChoice choice;
    int saved = 0;
    choice.onConfirmed = [&](EditorThemeKind) { saved++; };
    choice.Reset(EditorThemeKind::Dark);

    choice.Request(EditorThemeKind::Dark);
    choice.Update();
    EXPECT_FALSE(choice.IsAsking());
    EXPECT_EQ(saved, 0);
}
