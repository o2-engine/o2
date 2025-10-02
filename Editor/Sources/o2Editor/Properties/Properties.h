#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Properties/IPropertyField.h"
#include "o2Editor/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class VerticalLayout;
    class Widget;
}

// Editor properties window accessor macros
#define o2EditorProperties Editor::Properties::Instance()

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);

    // ------------------------------------------------------------------------------
    // Editor properties building utility. Can cache, build and reuse property fields
    // ------------------------------------------------------------------------------
    class Properties : public Singleton<Properties>
    {
    public:
        Function<void(const Ref<IPropertyField>&)> onFieldChanged; // Some field changed event

    public:
        // Default constructor. Initializes properties samples and fills pools
        Properties(RefCounter* refCounter);

        // Destructor
        ~Properties();

        // Sets private field viewing state
        void SetPrivateFieldsVisible(bool visible);

        // Returns is private fields visible
        bool IsPrivateFieldsVisible() const;

        // Free properties and put in cache
        void FreeProperties(const Ref<PropertiesContext>& context);

        // Free property field and put in cache
        void FreeProperty(const Ref<IPropertyField>& field);

        // Builds layout viewer by type for objects
        void BuildObjectProperties(const Ref<VerticalLayout>& layout, const Type* type, const Ref<PropertiesContext>& context, const String& path,
                                   const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                   const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Builds layout viewer by fields
        void BuildObjectProperties(const Ref<VerticalLayout>& layout, Vector<const FieldInfo*> fields, const Ref<PropertiesContext>& context, const String& path,
                                   const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                   const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Build layout viewer for field
        Ref<IPropertyField> BuildField(const Ref<VerticalLayout>& layout, const FieldInfo* fieldInfo, const Ref<PropertiesContext>& context, const String& path,
                                       const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                       const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Build layout viewer for field
        Ref<IPropertyField> BuildField(const Ref<VerticalLayout>& layout, const Type& objectType, const String& fieldName, const String& path,
                                       const Ref<PropertiesContext>& context,
                                       const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                       const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Build layout viewer for fields
        void BuildFields(const Ref<VerticalLayout>& layout, const Type& objectType, const Vector<String>& fieldsNames, const String& path,
                         const Ref<PropertiesContext>& context,
                         const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                         const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Build layout viewer for field
        template<typename PropertyFieldType>
        Ref<PropertyFieldType> BuildFieldType(const Ref<VerticalLayout>& layout, const FieldInfo* fieldInfo, const Ref<PropertiesContext>& context, const String& path,
                                              const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = mOnPropertyCompletedChangingUndoCreateDelegate,
                                              const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Build layout viewer for field
        template<typename PropertyFieldType>
        Ref<PropertyFieldType> BuildFieldType(const Ref<VerticalLayout>& layout, const Type& objectType, const String& fieldName, const String& path,
                                              const Ref<PropertiesContext>& context,
                                              const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = mOnPropertyCompletedChangingUndoCreateDelegate,
                                              const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Checks that property with type can be created
        bool IsFieldTypeSupported(const Type* type) const;

        // Checks is property visible in properties window: 
        // checking protection section, ignore and editor attributes
        bool IsPropertyVisible(const FieldInfo* info, bool allowPrivate) const;

        // Checks is property visible in properties window using current private visibility state: 
        // checking protection section, ignore and editor attributes
        bool IsPropertyVisible(const FieldInfo* info) const;

        // Returns available field by type
        const Type* GetFieldPropertyType(const Type* valueType) const;

        // Creates field property by type 
        Ref<IPropertyField> CreateFieldProperty(const Type* type, const String& name,
                                                const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                                const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Creates regular primitive property field
        Ref<IPropertyField> CreateRegularField(const Type* type, const String& name,
                                               const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                               const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Creates regular enumerate primitive property field
        Ref<IPropertyField> CreateEnumField(const Type* type, const String& name,
                                            const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                            const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Creates object field
        Ref<IPropertyField> CreateObjectField(const String& name,
                                              const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                              const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Creates object pointer field
        Ref<IPropertyField> CreateObjectPtrField(const ObjectType* basicType, const String& name,
                                                 const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                                 const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Creates vector field
        Ref<IPropertyField> CreateVectorField(const Type* type, const String& name,
                                              const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                              const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Returns object properties viewer
        Ref<IObjectPropertiesViewer> CreateObjectViewer(const Type* type, const String& path,
                                                        const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                                                        const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);

        // Free object viewer, store in pool for reuse
        void FreeObjectViewer(const Ref<IObjectPropertiesViewer>& viewer);

    protected:
        typedef Map<const Type*, const Type*> PropertiesFieldsMap;
        typedef Map<const Type*, Vector<Ref<IPropertyField>>> TypePropertyMap;

        typedef Map<const Type*, const Type*> IObjectPropertiesViewersMap;
        typedef Map<const Type*, Vector<Ref<IObjectPropertiesViewer>>> TypeObjectPropertiesViewerMap;

        int  mPropertyFieldsPoolStep = 5; // Field properties pools resize step                            
        bool mPrivateVisible = false;     // Is private fields visible

        PropertiesFieldsMap         mAvailablePropertiesFields;        // Available properties fields samples
        IObjectPropertiesViewersMap mAvailableObjectPropertiesViewers; // Available object properties viewers samples

        TypePropertyMap               mPropertiesPool;              // Pool of properties, grouped by property type
        TypeObjectPropertiesViewerMap mObjectPropertiesViewersPool; // Pool of object properties viewers, grouped by object type

    protected:
        // Initializes available properties fields samples
        void InitializeAvailablePropertiesFields();

        // Initializes available object properties viewers samples
        void InitializeAvailableObjectPropertiesViewers();

        // Returns type of object viewer, which is closest ty type by based types hierarchy depth
        const Type* GetClosesBasedTypeObjectViewer(const Type* type) const;

        // Builds layout viewer by fields without filtering
        void BuildFields(const Ref<VerticalLayout>& layout, Vector<const FieldInfo*> fields, const Ref<PropertiesContext>& context, const String& path,
                         const IPropertyField::OnChangeCompletedFunc& onChangeCompleted = IPropertyField::OnChangeCompletedFunc::empty,
                         const IPropertyField::OnChangedFunc& onChanged = IPropertyField::OnChangedFunc::empty);
    };

    template<typename PropertyFieldType>
    Ref<PropertyFieldType> Properties::BuildFieldType(const Ref<VerticalLayout>& layout, const FieldInfo* fieldInfo,
                                                      const Ref<PropertiesContext>& context, const String& path,
                                                      const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= IPropertyField::OnChangeCompletedFunc::empty*/,
                                                      const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        return DynamicCast<PropertyFieldType>(BuildField(layout, fieldInfo, context, path, onChangeCompleted, onChanged));
    }

    template<typename PropertyFieldType>
    Ref<PropertyFieldType> Properties::BuildFieldType(const Ref<VerticalLayout>& layout, const Type& objectType, const String& fieldName,
                                                      const String& path, const Ref<PropertiesContext>& context,
                                                      const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= IPropertyField::OnChangeCompletedFunc::empty*/,
                                                      const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        return DynamicCast<PropertyFieldType>(BuildField(layout, objectType, fieldName, path, context, onChangeCompleted, onChanged));
    }

}
