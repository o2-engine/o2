#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/UI/Style/EditorUIStyle.h"

using namespace o2;
using namespace Editor;

namespace
{
    const String stylesFolder = "Editor UI styles";

    // Restores the built rebuild date and leaves the complete style loaded for the next tests of the process
    struct BuiltStyleDateGuard
    {
        String path = String(GetEditorBuiltAssetsPath()) + stylesFolder + "/rebuildDate.json";
        bool existed = false;
        String backup;

        BuiltStyleDateGuard()
        {
            existed = o2FileSystem.IsFileExist(path);
            if (existed)
                backup = o2FileSystem.ReadFile(path);
        }

        ~BuiltStyleDateGuard()
        {
            if (existed)
                o2FileSystem.WriteFile(path, backup);
            else
                o2FileSystem.FileDelete(path);

            EditorUIStyleBuilder().RebuildEditorUIManager(stylesFolder, false, false);
        }

        void Write(const TimeStamp& date)
        {
            DataDocument data;
            data["generatedDate"] = date;
            data.SaveToFile(path);
        }
    };

    TimeStamp StyleSourceDate()
    {
        return o2FileSystem.GetFileInfo("../../o2/Editor/Sources/o2Editor/UI/Style/EditorUIStyle.cpp").editDate;
    }
}

// Editor assets get built before the first launch regenerates the style, so the built copy lags behind the saved one
TEST(EditorUIStyleLoading, StaleBuiltStyleIsRebuiltInsteadOfLoaded)
{
    BuiltStyleDateGuard guard;
    guard.Write(TimeStamp(0, 0, 0, 1, 1, 2000));

    EditorUIStyleBuilder().RebuildEditorUIManager(stylesFolder, false, true);

    EXPECT_TRUE(o2UI.GetWidgetStyle<Button>("menu pipeline import") != nullptr);
    for (auto& style : o2UI.GetWidgetStyles())
        EXPECT_TRUE(style->GetPath().IsEmpty()) << "stale built style was loaded: " << style->GetPath();
}

TEST(EditorUIStyleLoading, BuiltStyleOfTheCurrentSourceIsLoadedFromAssets)
{
    BuiltStyleDateGuard guard;
    guard.Write(StyleSourceDate());

    EditorUIStyleBuilder().RebuildEditorUIManager(stylesFolder, false, true);

    ASSERT_FALSE(o2UI.GetWidgetStyles().IsEmpty());
    EXPECT_FALSE(o2UI.GetWidgetStyles()[0]->GetPath().IsEmpty());
}
