#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Core/Properties/IPropertyField.h"
#include "o2Editor/Core/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class Button;
    class ContextMenu;
    class Label;
    class Spoiler;
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);

    // ------------------------------
    // Editor object pointer property
    // ------------------------------
    class ObjectPtrProperty: public IPropertyField
    {
    public:
        // Default constructor
        ObjectPtrProperty(RefCounter* refCounter);

        // Copy constructor
        ObjectPtrProperty(RefCounter* refCounter, const ObjectPtrProperty& other);

        // Copy operator
        ObjectPtrProperty& operator=(const ObjectPtrProperty& other);

        // Sets fields
        void SetValueAndPrototypeProxy(const TargetsVec& targets) override;

        // Updates and checks value
        void Refresh(bool forcible = false) override;

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        // Specializes field info, processing attributes
        void SetFieldInfo(const FieldInfo* fieldInfo) override;

        // Sets property caption
        void SetCaption(const WString& text) override;

        // Returns property caption
        WString GetCaption() const override;

        // Adds remove button
        Ref<Button> GetRemoveButton() override;

        // Sets basic object type, used in create
        void SetBasicType(const ObjectType* type);

        // Expands property fields
        void Expand();

        // Collapses property field
        void Collapse();

        // Sets properties expanding
        void SetExpanded(bool expanded);

        // Returns is properties expanded
        bool IsExpanded() const;

        SERIALIZABLE(ObjectPtrProperty);
        CLONEABLE_REF(ObjectPtrProperty);

    protected:
        const ObjectType* mBasicObjectType = nullptr;   // Base object type, used for create
        const ObjectType* mCurrentObjectType = nullptr; // Type of target objects
        const ObjectType* mBuiltObjectType = nullptr;   // Type of built object properties

        bool mAvailableMultipleTypes = false; // Is multiple types creation available

        bool mDontDeleteEnabled = false; // When it is true, delete button is disabled
        bool mNoHeader = false;          // Is no header attribute exists
        bool mExpanded = false;          // True when must be expanded after creating object viewer

        Ref<IObjectPropertiesViewer> mObjectViewer; // Object viewer

        Ref<Label>            mCaption;              // Property caption, used when object is empty and there are no spoiler
        Ref<HorizontalLayout> mHeaderContainer;      // Type caption and create/delete button container widget, placed on spoiler head
        Ref<Button>           mTypeButton;           // Caption that shows type of object or nullptr, opens context menu when pressed
        Ref<Button>           mCreateOrRemoveButton; // Create and delete button. Create - when value is nullptr, delete - when not
        Ref<ContextMenu>      mCreateMenu;           // Create object context menu. Initializes with types derived from mObjectType 
                                                     // when this type changing and create button were pressed

        bool mContextInitialized = false;    // True when context menu initialized with available types of objects. 
                                             // Context menu initializes when type changed and create button pressed

        bool mImmediateCreateObject = false; // True when no reason to show context menu, because there is only one available type

    protected:
        // Called when property puts in buffer. Here you can release your shared resources
        void OnFreeProperty() override;

        // Enable viewer event function
        void OnPropertyEnabled() override;

        // Disable viewer event function
        void OnPropertyDisabled() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Updates viewer header caption and header container 
        void UpdateViewerHeader();

        // Called when create or remove button pressed
        void OnCreateOrRemovePressed();

        // Called when create button pressed
        void OnCreatePressed();

        // Removes object from property
        void RemoveObject();

        // Creates object by type
        void CreateObject(const ObjectType* type);

        // Stores values to data
        void StoreValues(Vector<DataDocument>& data) const override;

        // Converts proxy to IObject property, gets value and returns
        IObject* GetProxy(const Ref<IAbstractValueProxy>& proxy) const;

        // Converts proxy to IObject proxy, then sets value via proxy
        void SetProxy(const Ref<IAbstractValueProxy>& proxy, IObject* object);

        // Updates available types in create context menu
        void CheckCreateContextMenu();
    };
}
// --- META ---

CLASS_BASES_META(Editor::ObjectPtrProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::ObjectPtrProperty)
{
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mBasicObjectType);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mCurrentObjectType);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mBuiltObjectType);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mAvailableMultipleTypes);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDontDeleteEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNoHeader);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mExpanded);
    FIELD().PROTECTED().NAME(mObjectViewer);
    FIELD().PROTECTED().NAME(mCaption);
    FIELD().PROTECTED().NAME(mHeaderContainer);
    FIELD().PROTECTED().NAME(mTypeButton);
    FIELD().PROTECTED().NAME(mCreateOrRemoveButton);
    FIELD().PROTECTED().NAME(mCreateMenu);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mContextInitialized);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mImmediateCreateObject);
}
END_META;
CLASS_METHODS_META(Editor::ObjectPtrProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ObjectPtrProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetRemoveButton);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBasicType, const ObjectType*);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFreeProperty);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateViewerHeader);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCreateOrRemovePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCreatePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveObject);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateObject, const ObjectType*);
    FUNCTION().PROTECTED().SIGNATURE(void, StoreValues, Vector<DataDocument>&);
    FUNCTION().PROTECTED().SIGNATURE(IObject*, GetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetProxy, const Ref<IAbstractValueProxy>&, IObject*);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCreateContextMenu);
}
END_META;
// --- END META ---
