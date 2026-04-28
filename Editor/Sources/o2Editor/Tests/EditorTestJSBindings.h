#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

namespace Editor::Tests
{
    class EditorTestRunner;
    struct TestContext;

    // Registers Test/EditorUI/Scene globals into the currently-active JS realm.
    // Called by EditorTestRunner before evaluating each test file.
    class EditorTestJSBindings
    {
    public:
        static void Register(EditorTestRunner& runner, TestContext& ctx);
    };
}

#endif
