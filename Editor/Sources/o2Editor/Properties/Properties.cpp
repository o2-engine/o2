#include "o2Editor/stdafx.h"
#include "Properties.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/Attributes/GroupAttribute.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"
#include "o2/Utils/Editor/Attributes/NameAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/StringUtils.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Properties/Basic/ObjectProperty.h"
#include "o2Editor/Properties/Basic/ObjectPtrProperty.h"
#include "o2Editor/Properties/Basic/VectorProperty.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

DECLARE_SINGLETON(Editor::Properties);

namespace Editor
{
    Editor::IPropertyField::OnChangeCompletedFunc Properties::mOnPropertyCompletedChangingUndoCreateDelegate;

    Properties::Properties(RefCounter* refCounter):
        Singleton<Properties>(refCounter)
    {
        InitializeAvailablePropertiesFields();
        InitializeAvailableObjectPropertiesViewers();

        mOnPropertyCompletedChangingUndoCreateDelegate = 
            MakeFunction<ActionsList, void, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&>(
            &o2EditorApplication, &EditorApplication::DoneActorPropertyChangeAction);
    }

    Properties::~Properties()
    {}

    void Properties::InitializeAvailablePropertiesFields()
    {
        auto avaialbleTypes = TypeOf(IPropertyField).GetDerivedTypes();

        avaialbleTypes.Remove(&TypeOf(ObjectProperty));
        avaialbleTypes.Remove(&TypeOf(ObjectPtrProperty));
        avaialbleTypes.Remove(&TypeOf(VectorProperty));
        avaialbleTypes.RemoveAll([](const Type* type) { return type->GetName().Contains("TPropertyField"); });

        for (auto& x : avaialbleTypes)
        {
            if (auto valueType = x->InvokeStatic<const Type*>("GetValueTypeStatic"))
                mAvailablePropertiesFields[valueType] = x;
        }
    }

    void Properties::InitializeAvailableObjectPropertiesViewers()
    {
        auto availableTypes = TypeOf(IObjectPropertiesViewer).GetDerivedTypes();
        availableTypes.Remove(&TypeOf(IObjectPropertiesViewer));
        availableTypes.Remove(&TypeOf(DefaultObjectPropertiesViewer));

        for (auto& x : availableTypes)
        {
            if (auto objectType = x->InvokeStatic<const Type*>("GetViewingObjectTypeStatic"))
                mAvailableObjectPropertiesViewers[objectType] = x;
        }
    }

    const Type* Properties::GetClosesBasedTypeObjectViewer(const Type* type) const
    {
        Vector<const Type*> itTypes = { type };
        while (!itTypes.IsEmpty())
        {
            Vector<const Type*> nextItTypes;

            for (auto& t : itTypes)
            {
                const Type* viewerType = nullptr;
                if (mAvailableObjectPropertiesViewers.TryGetValue(t, viewerType))
                    return viewerType;

                nextItTypes.Add(t->GetBaseTypes().Convert<const Type*>([](const Type::BaseType& x) { return x.type; }));
            }

            itTypes = nextItTypes;
        }

        return nullptr;
    }

    Ref<IPropertyField> Properties::BuildField(const Ref<VerticalLayout>& layout, const FieldInfo* fieldInfo,
                                               const Ref<PropertiesContext>& context, const String& path,
                                               const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                               const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        Timer timer;

        const Type* fieldType = fieldInfo->GetType();
        String propertyName;
        
        if (auto nameAttribute = fieldInfo->GetAttribute<NameAttribute>())
            propertyName = nameAttribute->name;
        else
            propertyName = GetSmartName(fieldInfo->GetName());

        auto fieldWidget = CreateFieldProperty(fieldType, propertyName, onChangeCompleted, onChanged);
        if (!fieldWidget)
            return nullptr;

        fieldWidget->name = fieldInfo->GetName() + " : " + fieldInfo->GetType()->GetName();
        fieldWidget->SetParentContext(context);
        fieldWidget->SetValuePath(path + fieldInfo->GetName());
        fieldWidget->SetFieldInfo(fieldInfo);

        if (auto invokeOnChangeAttribute = fieldInfo->GetAttribute<InvokeOnChangeAttribute>())
        {
            fieldWidget->onChanged += [&, invokeOnChangeAttribute](const Ref<IPropertyField>&) {
                for (auto& target : context->targets) 
                {
                    auto& targetType = target.first->GetType();
                    auto& targetObjType = dynamic_cast<const ObjectType&>(targetType);
                    targetObjType.Invoke<void>(invokeOnChangeAttribute->methodName, targetObjType.DynamicCastFromIObject(target.first));
                }
            };
        }

        layout->AddChild(fieldWidget);

        context->properties.Add(fieldInfo, fieldWidget);

        //o2Debug.Log("Field " + path + "/" + fieldInfo->GetName() + " for " + (String)timer.GetDeltaTime());

        return fieldWidget;
    }

    Ref<IPropertyField> Properties::BuildField(const Ref<VerticalLayout>& layout, const Type& objectType, const String& fieldName, const String& path,
                                               const Ref<PropertiesContext>& context,
                                               const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                               const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        return BuildField(layout, objectType.GetField(fieldName), context, path, onChangeCompleted, onChanged);
    }

    void Properties::BuildFields(const Ref<VerticalLayout>& layout, Vector<const FieldInfo*> fields,
                                 const Ref<PropertiesContext>& context, const String& path,
                                 const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                 const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        Timer timer; 

        const String emptyGroupName = "AFIRST";

        Map<String, Vector<const FieldInfo*>> groupedFields;
        for (auto& field : fields)
        {
            String fieldGroup = emptyGroupName;
            if (auto groupAttribute = field->GetAttribute<GroupAttribute>())
                fieldGroup = groupAttribute->name;

            groupedFields[fieldGroup].Add(field);
        }

        for (auto& group : groupedFields)
        {
            if (group.first != emptyGroupName)
            {
                auto spoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
                spoiler->SetCaption(group.first);
                spoiler->Expand();
                spoiler->borderTop = 5;
                layout->AddChild(spoiler);

                for (auto& fieldInfo : group.second)
                    BuildField(spoiler, fieldInfo, context, path, onChangeCompleted, onChanged);
            }
            else
            {
                for (auto& fieldInfo : group.second)
                    BuildField(layout, fieldInfo, context, path, onChangeCompleted, onChanged);
            }
        }

        //o2Debug.Log(">>> Fields created for " + (String)timer.GetDeltaTime());
    }

    void Properties::BuildFields(const Ref<VerticalLayout>& layout, const Type& objectType, const Vector<String>& fieldsNames,
                                 const String& path, const Ref<PropertiesContext>& context,
                                 const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/, 
                                 const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        for (auto& name : fieldsNames)
            BuildField(layout, objectType, name, path, context, onChangeCompleted, onChanged);
    }

    void Properties::SetPrivateFieldsVisible(bool visible)
    {
        mPrivateVisible = visible;
    }

    bool Properties::IsPrivateFieldsVisible() const
    {
        return mPrivateVisible;
    }

    bool Properties::IsFieldTypeSupported(const Type* type) const
    {
        if (type->GetUsage() == Type::Usage::Vector)
            return IsFieldTypeSupported(dynamic_cast<const VectorType*>(type)->GetElementType());

        if (type->IsBasedOn(TypeOf(IObject)))
            return true;

        if ((type->GetUsage() == Type::Usage::Pointer || type->GetUsage() == Type::Usage::Reference) &&
            ((PointerType*)type)->GetBaseType()->IsBasedOn((TypeOf(IObject))))
        {
            return true;
        }

        if (type->GetUsage() == Type::Usage::Enumeration)
            return true;

        if (type->GetUsage() == Type::Usage::Property)
            return IsFieldTypeSupported(dynamic_cast<const PropertyType*>(type)->GetValueType());

        if (GetFieldPropertyType(type))
            return true;

        return false;
    }

    void Properties::FreeProperties(const Ref<PropertiesContext>& context)
    {
        for (auto& prop : context->properties)
            FreeProperty(prop.second);

        context->properties.Clear();
    }

    void Properties::FreeProperty(const Ref<IPropertyField>& field)
    {
        if (!mPropertiesPool.ContainsKey(field->GetValueType()))
            mPropertiesPool.Add(field->GetValueType(), Vector<Ref<IPropertyField>>());

        mPropertiesPool[field->GetValueType()].Add(field);
        field->OnFreeProperty();
        field->SetParent(nullptr, false);
    }

    bool Properties::IsPropertyVisible(const FieldInfo* info, bool allowPrivate) const
    {
        if (info->HasAttribute<IgnoreEditorPropertyAttribute>())
            return false;

        if (info->GetProtectionSection() == ProtectSection::Public || allowPrivate)
            return true;

        if (info->HasAttribute<EditorPropertyAttribute>())
            return true;

        return false;
    }

    bool Properties::IsPropertyVisible(const FieldInfo* info) const
    {
        return IsPropertyVisible(info, mPrivateVisible);
    }

    void Properties::BuildObjectProperties(const Ref<VerticalLayout>& layout, const Type* type,
                                           const Ref<PropertiesContext>& context, const String& path,
                                           const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                           const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        BuildObjectProperties(layout, type->GetFieldsWithBaseClasses(), context, path, onChangeCompleted, onChanged);
    }

    void Properties::BuildObjectProperties(const Ref<VerticalLayout>& layout, Vector<const FieldInfo*> fields,
                                           const Ref<PropertiesContext>& context, const String& path,
                                           const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                           const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        PushEditorScopeOnStack scope;

        Vector<const FieldInfo*> regularFields = fields.FindAll(
            [&](const FieldInfo* x) { return IsPropertyVisible(x, false); });

        BuildFields(layout, regularFields, context, path, onChangeCompleted, onChanged);

        if (mPrivateVisible)
        {
            Vector<const FieldInfo*> privateFields = fields.FindAll(
                [&](const FieldInfo* x) { return IsPropertyVisible(x, true) && !regularFields.Contains(x); });

            if (!privateFields.IsEmpty())
            {
                auto privates = context->privatePropertiesSpoiler;

                if (!privates)
                    privates = layout->GetChildByType<Spoiler>("privates");

                if (!privates)
                {
                    privates = o2UI.CreateWidget<Spoiler>("expand with caption");
                    privates->name = "privates";
                    privates->SetCaption("Private");
                    layout->AddChild(privates);
                }
                else privates->SetIndexInSiblings(layout->GetChildren().Count() - 1);

                context->privatePropertiesSpoiler = privates;
                BuildFields(privates, privateFields, context, path, onChangeCompleted, onChanged);
            }
        }
        else if (context->privatePropertiesSpoiler)
        {
            context->privatePropertiesSpoiler->SetEnabled(false);
        }

        context->builtWithPrivateProperties = mPrivateVisible;
    }

    Ref<IPropertyField> Properties::CreateFieldProperty(const Type* type, const String& name, 
                                                        const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                        const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        PushEditorScopeOnStack enterScope;

        if (GetFieldPropertyType(type))
            return CreateRegularField(type, name, onChangeCompleted, onChanged);

        if (type->GetUsage() == Type::Usage::Vector)
        {
            auto elementType = dynamic_cast<const VectorType*>(type)->GetElementType();
            if (IsFieldTypeSupported(elementType))
                return CreateVectorField(type, name, onChangeCompleted, onChanged);

            return nullptr;
        }

        if (type->IsBasedOn(TypeOf(IObject)))
            return CreateObjectField(name, onChangeCompleted, onChanged);

        if (type->GetUsage() == Type::Usage::Pointer)
        {
            auto pointerType = dynamic_cast<const PointerType*>(type);
            auto baseType = pointerType->GetBaseType();
            if (baseType->IsBasedOn((TypeOf(IObject))))
            {
                return CreateObjectPtrField(dynamic_cast<const ObjectType*>(baseType), name,
                                            onChangeCompleted, onChanged);
            }

            return nullptr;
        }

        if (type->GetUsage() == Type::Usage::Reference)
        {
            auto referenceType = dynamic_cast<const ReferenceType*>(type);
            auto baseType = referenceType->GetBaseType();
            if (baseType->IsBasedOn((TypeOf(IObject))))
            {
                return CreateObjectPtrField(dynamic_cast<const ObjectType*>(baseType), name,
                                            onChangeCompleted, onChanged);
            }

            return nullptr;
        }

        if (type->GetUsage() == Type::Usage::Enumeration)
            return CreateEnumField(type, name, onChangeCompleted, onChanged);

        if (type->GetUsage() == Type::Usage::Property)
        {
            auto valueType = dynamic_cast<const PropertyType*>(type)->GetValueType();
            return CreateFieldProperty(valueType, name, onChangeCompleted, onChanged);
        }

        return nullptr;
    }

    Ref<IPropertyField> Properties::CreateRegularField(const Type* type, const String& name,
                                                       const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                       const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        const Type* fieldPropertyType = GetFieldPropertyType(type);
        if (!fieldPropertyType)
            return nullptr;

        Ref<IPropertyField> fieldProperty;

        if (mPropertiesPool.ContainsKey(type) && mPropertiesPool[type].Count() > 0)
            fieldProperty = mPropertiesPool[type].PopBack();
        else
            fieldProperty = DynamicCast<IPropertyField>(o2UI.CreateWidget(*fieldPropertyType, "with caption"));

        fieldProperty->onChanged = onChanged;
        fieldProperty->onChangeCompleted = onChangeCompleted;
        fieldProperty->SetCaption(name);

        return fieldProperty;
    }

    Ref<IPropertyField> Properties::CreateEnumField(const Type* type, const String& name,
                                                    const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                    const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        Ref<EnumProperty> fieldProperty;

        if (mPropertiesPool.ContainsKey(type) && mPropertiesPool[type].Count() > 0)
            fieldProperty = DynamicCast<EnumProperty>(mPropertiesPool[type].PopBack());
        else
            fieldProperty = o2UI.CreateWidget<EnumProperty>("with caption");

        fieldProperty->onChanged = onChanged;
        fieldProperty->onChangeCompleted = onChangeCompleted;
        fieldProperty->SetCaption(name);
        fieldProperty->SpecializeType(type);

        return fieldProperty;
    }

    Ref<IPropertyField> Properties::CreateObjectField(const String& name,
                                                      const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                      const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        Ref<ObjectProperty> fieldProperty;

        const Type* objectType = &TypeOf(IObject);
        if (mPropertiesPool.ContainsKey(objectType) && mPropertiesPool[objectType].Count() > 0)
            fieldProperty = DynamicCast<ObjectProperty>(mPropertiesPool[objectType].PopBack());
        else
            fieldProperty = mmake<ObjectProperty>();

        fieldProperty->onChanged = onChanged;
        fieldProperty->onChangeCompleted = onChangeCompleted;
        fieldProperty->SetCaption(name);

        return fieldProperty;
    }

    Ref<IPropertyField> Properties::CreateObjectPtrField(const ObjectType* basicType, const String& name,
                                                         const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                         const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        Ref<ObjectPtrProperty> fieldProperty;

        const Type* objectType = &TypeOf(IObject*);
        if (mPropertiesPool.ContainsKey(objectType) && mPropertiesPool[objectType].Count() > 0)
            fieldProperty = DynamicCast<ObjectPtrProperty>(mPropertiesPool[objectType].PopBack());
        else
            fieldProperty = mmake<ObjectPtrProperty>();

        fieldProperty->onChanged = onChanged;
        fieldProperty->onChangeCompleted = onChangeCompleted;
        fieldProperty->SetCaption(name);
        fieldProperty->SetBasicType(basicType);

        return fieldProperty;
    }

    Ref<IPropertyField> Properties::CreateVectorField(const Type* type, const String& name,
                                                      const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                      const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        Ref<VectorProperty> fieldProperty;

        if (mPropertiesPool.ContainsKey(type) && mPropertiesPool[type].Count() > 0)
            fieldProperty = DynamicCast<VectorProperty>(mPropertiesPool[type].PopBack());
        else 
            fieldProperty = mmake<VectorProperty>();

        fieldProperty->onChanged = onChanged;
        fieldProperty->onChangeCompleted = onChangeCompleted;
        fieldProperty->SpecializeType(type);
        fieldProperty->SetCaption(name);

        return fieldProperty;
    }

    Ref<IObjectPropertiesViewer> Properties::CreateObjectViewer(const Type* type, const String& path, 
                                                                const IPropertyField::OnChangeCompletedFunc& onChangeCompleted /*= mOnPropertyCompletedChangingUndoCreateDelegate*/,
                                                                const IPropertyField::OnChangedFunc& onChanged /*= IPropertyField::OnChangedFunc::empty*/)
    {
        auto viewerType = GetClosesBasedTypeObjectViewer(type);
        if (!viewerType)
            viewerType = &TypeOf(DefaultObjectPropertiesViewer);

        Ref<IObjectPropertiesViewer> viewer;

        if (mObjectPropertiesViewersPool.ContainsKey(type))
        {
            if (!mObjectPropertiesViewersPool[type].IsEmpty())
                viewer = mObjectPropertiesViewersPool[type].PopBack();
        }

        if (!viewer)
            viewer = DynamicCast<IObjectPropertiesViewer>(viewerType->CreateSampleRef());

        viewer->path = path;
        viewer->onChanged = onChanged;
        viewer->onChangeCompleted = onChangeCompleted;

        return viewer;
    }

    void Properties::FreeObjectViewer(const Ref<IObjectPropertiesViewer>& viewer)
    {
        auto type = viewer->GetViewingObjectType();

        if (!mObjectPropertiesViewersPool.ContainsKey(type))
            mObjectPropertiesViewersPool.Add(type, Vector<Ref<IObjectPropertiesViewer>>());

        mObjectPropertiesViewersPool[type].Add(viewer);
        viewer->OnFree();
        viewer->GetSpoiler()->SetParent(nullptr);
    }

    const Type* Properties::GetFieldPropertyType(const Type* valueType) const
    {
        for (auto& kv : mAvailablePropertiesFields)
        {
            if (valueType->GetUsage() == Type::Usage::Pointer && kv.first->GetUsage() == Type::Usage::Pointer)
            {
                if (((PointerType*)valueType)->GetBaseType()->IsBasedOn(*((PointerType*)kv.first)->GetBaseType()))
                    return kv.second;
            }
            else if (valueType->IsBasedOn(*kv.first))
                return kv.second;
        }

        return nullptr;
    }
}
