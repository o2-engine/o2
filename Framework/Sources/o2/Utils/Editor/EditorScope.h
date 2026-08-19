#pragma once

namespace o2
{
    // -------------------------------------------------------------------------------------------------------------------------------
    // This separates editor's stuff from editing scene. When entering in editor scope the counter increases, on exiting it decreases.
    // Zero depth means that you are working on the scene. By default depth is zero
    // -------------------------------------------------------------------------------------------------------------------------------
    class EditorScope
    {
    public:
        // Entering in editor's scope. After you can work with editor UI, events and other. Increases depth
        static void Enter(int count = 1);

        // Exiting from editor's scope. Decreases depth. When depth is zero - you working with scene
        static void Exit(int count = 1);

        // Returns editor scope entered depth. When depth is greater than zero you working with editor's stuff - UI, event, etc
        static int GetDepth() { return mDepth; }

        // Returns is you are in editor scope
        static bool IsInScope() { return mDepth > 0; }

    private:
        static int mDepth; // Scope entered depth
    };

    // ---------------------------------------------------------------
    // Pushing editor's scope here, and pop after stack frame finished
    // ---------------------------------------------------------------
    class PushEditorScopeOnStack
    {
    public:
        // Default constructor, calls Scope::Enter
        PushEditorScopeOnStack(int count = 1);

        // Destructor, calls Scope::Exit
        ~PushEditorScopeOnStack();

    private:
        int mDepth;
    };

    // ---------------------------------------------------------------
    // Sets not editor in this scope, then restores edtiro scope depth
    // ---------------------------------------------------------------
    class ForcePopEditorScopeOnStack
    {
    public:
        // Default constructor, calls Scope::Exit and stores depth
        ForcePopEditorScopeOnStack();

        // Destructor, calls Scope::Exit
        ~ForcePopEditorScopeOnStack();

    private:
        int mDepth;
    };
}
