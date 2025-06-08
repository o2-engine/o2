#include "o2Editor/stdafx.h"
#include "PropertyChange.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

namespace Editor
{
    PropertyChangeAction::PropertyChangeAction()
    {}

    PropertyChangeAction::PropertyChangeAction(const Vector<Ref<SceneEditableObject>>& objects,
                                               const String& propertyPath,
                                               const Vector<DataDocument>& beforeValues,
                                               const Vector<DataDocument>& afterValues) :
        objectsIds(objects.Convert<SceneUID>([](const auto& x) { return x->GetID(); })),
        propertyPath(propertyPath), beforeValues(beforeValues), afterValues(afterValues)
    {}

    String PropertyChangeAction::GetName() const
    {
        return "Property changed";
    }

    void PropertyChangeAction::Redo()
    {
        SetProperties(afterValues);
    }

    void PropertyChangeAction::Undo()
    {
        SetProperties(beforeValues);
    }

    void PropertyChangeAction::SetProperties(Vector<DataDocument>& values)
    {
        Vector<Ref<SceneEditableObject>> objects = objectsIds.Convert<Ref<SceneEditableObject>>([](SceneUID id) { 
            return o2Scene.GetEditableObjectByID(id); });

        const Type* componentType = nullptr;
        String finalPropertyPath = propertyPath;
        if (propertyPath.StartsWith("component:"))
        {
            int c = ((String)"component:").Length();

            String typeName = propertyPath.SubStr(c, propertyPath.Find('/', c));

            c += typeName.Length() + 1;
            finalPropertyPath.Erase(0, c);

            componentType = o2Reflection.GetType(typeName);
        }

        int i = 0;
        for (auto& object : objects)
        {
            if (!object)
                continue;

            const FieldInfo* fi = nullptr;
            void* ptr = nullptr;

            if (componentType)
            {
                if (auto actor = DynamicCast<Actor>(object))
                {
                    auto component = actor->GetComponent(componentType);
                    if (component)
                        ptr = component->GetType().GetFieldPtr(component.Get(), finalPropertyPath, fi);
                }
            }
            else
            {
                auto objectType = dynamic_cast<const ObjectType*>(&object->GetType());
                if (objectType)
                {
                    void* realTypeObject = objectType->DynamicCastFromIObject(dynamic_cast<IObject*>(object.Get()));
                    ptr = objectType->GetFieldPtr(realTypeObject, finalPropertyPath, fi);
                }
            }

            if (fi && ptr)
                fi->Deserialize(ptr, values[i]);

            object->OnChanged();

            i++;
        }
    }
}
// --- META ---

DECLARE_CLASS(Editor::PropertyChangeAction, Editor__PropertyChangeAction);
// --- END META ---
