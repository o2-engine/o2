#pragma once

#include <gtest/gtest.h>

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"

namespace Editor::Tests
{
    void EnsureEditorUIRoot();

    // Per-test fixture: spins up EditorUIRoot once and clears its widget tree
    // between tests. Doesn't bring up WindowsManager / EditorConfig — tests
    // build their own DockWindowPlace / DockableWindow trees directly.
    class EditorWindowsFixture : public ::testing::Test
    {
    protected:
        void SetUp() override;
        void TearDown() override;
    };

    o2::Ref<DockableWindow>  MakeDockable(const o2::String& name);
    o2::Ref<DockWindowPlace> MakeDock(const o2::String& name);
}
