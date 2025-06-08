#pragma once

#include "o2/Scene/SceneLayerRef.h"
#include "o2Editor/Properties/IPropertyField.h"

namespace o2
{
    class SceneLayer;
    class DropDown;
}

namespace Editor
{
    // -------------------------
    // Editor layer property box
    // -------------------------
    class SceneLayerRefProperty: public TPropertyField<Ref<SceneLayer>>
    {
    public:
        Function<void()> onSelectedInheritedValue; // Called when selected "Inherit from parent" value

    public:
        // Default constructor
        SceneLayerRefProperty(RefCounter* refCounter);

        // Copy constructor
        SceneLayerRefProperty(RefCounter* refCounter, const SceneLayerRefProperty& other);

        // Copy operator
        SceneLayerRefProperty& operator=(const SceneLayerRefProperty& other);

        // Sets selected "Inherit from parent" value
        void SetSelectedInheritedValue(bool inherited);

        // Returns is selected "Inherit from parent" value
        bool IsSelectedInheritedValue() const;

        // Sets use "Inherit from parent" value
        void SetUseInheritedValue(bool use);

        // Returns is used "Inherit from parent" value
        bool IsUseInheritedValue() const;

        SERIALIZABLE(SceneLayerRefProperty);
        CLONEABLE_REF(SceneLayerRefProperty);

    protected:               
        Ref<DropDown> mDropDown;              // Layer name dropdown
        bool          mUpdatingValue = false; // Is dropdown value updating and we don't we don't check selection

        bool mUseInheritedValue = false;      // Is used "Inherit from parent" value
        bool mSelectedInheritedValue = false; // Is selected "Inherit from parent" value

        String mInheritFromParentName = "Inherit from parent"; // Name of "Inherit from parent" value

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Updates layer list to actual list of layers on scene
        void UpdateLayersList();

        // Selects layer
        void SelectLayer(const WString& name);

        // Is required refresh view every time
        bool IsAlwaysRefresh() const override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneLayerRefProperty)
{
    BASE_CLASS(Editor::TPropertyField<Ref<SceneLayer>>);
}
END_META;
CLASS_FIELDS_META(Editor::SceneLayerRefProperty)
{
    FIELD().PUBLIC().NAME(onSelectedInheritedValue);
    FIELD().PROTECTED().NAME(mDropDown);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUpdatingValue);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUseInheritedValue);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelectedInheritedValue);
    FIELD().PROTECTED().DEFAULT_VALUE("Inherit from parent").NAME(mInheritFromParentName);
}
END_META;
CLASS_METHODS_META(Editor::SceneLayerRefProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneLayerRefProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedInheritedValue, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSelectedInheritedValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUseInheritedValue, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUseInheritedValue);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersList);
    FUNCTION().PROTECTED().SIGNATURE(void, SelectLayer, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsAlwaysRefresh);
}
END_META;
// --- END META ---
