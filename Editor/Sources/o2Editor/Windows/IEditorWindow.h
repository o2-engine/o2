#pragma once

#include "DockableWindow.h"
#include "o2/Utils/Serialization/Serializable.h"

using namespace o2;

namespace Editor
{
    // -----------------------------
    // Basic editor window interface
    // -----------------------------
    class IEditorWindow: public IObject, public RefCounterable
    {
    public:
        // Default constructor
        IEditorWindow();

        // Default constructor
        explicit IEditorWindow(RefCounter* refCounter);

        // Copy-constructor
        IEditorWindow(const IEditorWindow& other);

        // Copy-constructor
        IEditorWindow(RefCounter* refCounter, const IEditorWindow& other);

        // Virtual destructor
        virtual ~IEditorWindow();

        // Sets visibility
        virtual void SetVisible(bool visible);

        // Updates window logic
        virtual void Update(float dt);

        // Draws window stuff
        virtual void Draw();

        // Returns is window is visible
        bool IsVisible();

        // Shows window
        void Show();

        // Hides window
        void Hide();

        // Returns window
        const Ref<DockableWindow>& GetWindow() const;

        IOBJECT(IEditorWindow);

    protected:
        Ref<DockableWindow> mWindow; // Dockable UI window 

    protected:
        // Called after that all windows was created
        virtual void PostInitializeWindow() {}

        // Called when editor window has opened
        virtual void OnOpened() {}

        // Called when editor window has closed
        virtual void OnClosed() {}

        friend class WindowsManager;
        friend class WindowsLayout;
    };
}
// --- META ---

CLASS_BASES_META(Editor::IEditorWindow)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IEditorWindow)
{
    FIELD().PROTECTED().NAME(mWindow);
}
END_META;
CLASS_METHODS_META(Editor::IEditorWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const IEditorWindow&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const IEditorWindow&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVisible, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsVisible);
    FUNCTION().PUBLIC().SIGNATURE(void, Show);
    FUNCTION().PUBLIC().SIGNATURE(void, Hide);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<DockableWindow>&, GetWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, PostInitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, OnOpened);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClosed);
}
END_META;
// --- END META ---
