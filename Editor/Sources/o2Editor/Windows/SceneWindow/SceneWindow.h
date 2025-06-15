#pragma once

#include "o2Editor/Windows/IEditorWindow.h"

namespace o2
{
    class PopupWidget;
}

namespace Editor
{
    FORWARD_CLASS_REF(LayersPopup);
    FORWARD_CLASS_REF(SceneEditScreen);

    // --------------------
    // Scene editing window
    // --------------------
    class SceneWindow: public IEditorWindow
    {
    public:
        IOBJECT(SceneWindow);

    protected:
        Ref<SceneEditScreen> mEditWidget; // Scene editing widget

        Ref<Widget> mUpPanel; // Upper panel

        Ref<Button>      mLayersButton; // Layers button
        Ref<LayersPopup> mLayersPopup;  // Layers popup

        Ref<PopupWidget> mGizomsView; // Gizoms view

    public:
        // Default constructor
        SceneWindow();

        // Copy constructor
        SceneWindow(const SceneWindow& other);

        // Destructor
        ~SceneWindow();

    protected:
        // Initializes window and controls
        void InitializeWindow();

        // Called after that all windows was created
        void PostInitializeWindow() override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneWindow)
{
    BASE_CLASS(Editor::IEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::SceneWindow)
{
    FIELD().PROTECTED().NAME(mEditWidget);
    FIELD().PROTECTED().NAME(mUpPanel);
    FIELD().PROTECTED().NAME(mLayersButton);
    FIELD().PROTECTED().NAME(mLayersPopup);
    FIELD().PROTECTED().NAME(mGizomsView);
}
END_META;
CLASS_METHODS_META(Editor::SceneWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SceneWindow&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, PostInitializeWindow);
}
END_META;
// --- END META ---
