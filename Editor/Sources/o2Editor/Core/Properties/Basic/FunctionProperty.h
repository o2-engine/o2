#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Core/Properties/IPropertyField.h"
#include "o2Editor/Core/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class Spoiler;
}

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);
    FORWARD_CLASS_REF(ActorProperty);

    // ------------------------
    // Editor function property
    // ------------------------
    class FunctionProperty : public IPropertyField
    {
    public:
        // Default constructor
        FunctionProperty(RefCounter* refCounter);

        // Copy constructor
        FunctionProperty(RefCounter* refCounter, const FunctionProperty& other);

        // Copy operator
        FunctionProperty& operator=(const FunctionProperty& other);

        // Destructor
        ~FunctionProperty();

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

        // Expands property fields
        void Expand();

        // Collapses property field
        void Collapse();

        // Sets properties expanding
        void SetExpanded(bool expanded);

        // Returns is properties expanded
        bool IsExpanded() const;

        SERIALIZABLE(FunctionProperty);
        CLONEABLE_REF(FunctionProperty);

    protected:
        struct FunctionInstance: public RefCounterable
        {
            Ref<HorizontalLayout> layout;
            Ref<Label>            caption;
            Ref<ActorProperty>    refProperty;
            Ref<CustomDropDown>   funcDropDown;
            Ref<Button>           removeBtn;

            Vector<Pair<IActorSubscription*, IActorSubscription*>> values;
            Vector<Pair<Ref<Component>, String>> functionsDropdownMap;

        public:
            // Called when an actor is selected, updates the functions list
            void UpdateFunctionsList(const Ref<Actor>& actor, const Ref<Component>& selectedComponent, const String& selectedMethod);

            // Called when a function is selected, remembers the link to the component and the name from the list of functionsDropdownMap
            void OnFunctionSelected(int idx);

            bool operator==(const FunctionInstance& other) const;
        };

        Ref<Spoiler>          mSpoiler;         // Properties spoiler
        Ref<HorizontalLayout> mHeaderContainer; // Count property and other controls container
        Ref<Button>           mAddButton;       // Add button, adds new element at end

        Vector<Ref<FunctionInstance>> mInstances; // List of containers with pointers to serializable fields and interfaces

        Ref<HorizontalLayout>         mWidgetSample;  // Subscription widget sample
        Vector<Ref<HorizontalLayout>> mWidgetsBuffer; // Buffer of cached widgets

        bool mIsRefreshing = false; // Is currently refreshing content. Need to prevent cycled size changing

    protected:
        // Called when property puts in buffer. Here you can release your shared resources
        void OnFreeProperty() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Updates list of instances by target values
        void RefreshInstances();

        // Creates new widget for subscription
        Ref<FunctionInstance> CreateWidget();

        // Called when add button has pressed
        void OnAddPressed();

        // Called when remove subscription pressed
        void OnRemovePressed(const Ref<FunctionInstance>& instance);

        // Called when expanding spoiler, refreshing array properties
        void OnExpand();

        // Called when some property changed, sets value via proxy
        void OnPropertyChanged(const String& path, const Vector<DataDocument>& before,
                               const Vector<DataDocument>& after);
    };
}
// --- META ---

CLASS_BASES_META(Editor::FunctionProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::FunctionProperty)
{
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().NAME(mHeaderContainer);
    FIELD().PROTECTED().NAME(mAddButton);
    FIELD().PROTECTED().NAME(mInstances);
    FIELD().PROTECTED().NAME(mWidgetSample);
    FIELD().PROTECTED().NAME(mWidgetsBuffer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsRefreshing);
}
END_META;
CLASS_METHODS_META(Editor::FunctionProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const FunctionProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetRemoveButton);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFreeProperty);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, RefreshInstances);
    FUNCTION().PROTECTED().SIGNATURE(Ref<FunctionInstance>, CreateWidget);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemovePressed, const Ref<FunctionInstance>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnExpand);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
