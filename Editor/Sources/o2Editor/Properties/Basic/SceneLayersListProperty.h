#pragma once

#include "o2/Scene/SceneLayersList.h"
#include "o2Editor/Properties/IPropertyField.h"

namespace o2
{
    class Button;
    class ContextMenu;
    class EditBox;
    class Widget;
}

namespace Editor
{
    // ---------------------------------
    // Editor scene layers list property
    // // ------------------------------
    class SceneLayersListProperty: public TPropertyField<SceneLayersList>
    {
    public:
        // Default constructor
        SceneLayersListProperty(RefCounter* refCounter);

        // Copy constructor
        SceneLayersListProperty(RefCounter* refCounter, const SceneLayersListProperty& other);

        // Copy operator
        SceneLayersListProperty& operator=(const SceneLayersListProperty& other);

        SERIALIZABLE(SceneLayersListProperty);
        CLONEABLE_REF(SceneLayersListProperty);

    protected:
        Ref<Button> mPropertyButton; // Property box with layers' names
        Ref<Text>   mPropertyText;   // Text layer inside property box

        Ref<ContextMenu> mLayersContext; // Layers list toggles context

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Updates context layers list and opens
        void OpenContext();
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneLayersListProperty)
{
    BASE_CLASS(Editor::TPropertyField<SceneLayersList>);
}
END_META;
CLASS_FIELDS_META(Editor::SceneLayersListProperty)
{
    FIELD().PROTECTED().NAME(mPropertyButton);
    FIELD().PROTECTED().NAME(mPropertyText);
    FIELD().PROTECTED().NAME(mLayersContext);
}
END_META;
CLASS_METHODS_META(Editor::SceneLayersListProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneLayersListProperty&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenContext);
}
END_META;
// --- END META ---
