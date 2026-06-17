#include "o2Editor/stdafx.h"
#include "WidgetLayerLayout.h"

#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

namespace Editor
{
    WidgetLayerLayoutAction::WidgetLayerLayoutAction()
    {}

    WidgetLayerLayoutAction::WidgetLayerLayoutAction(const Vector<Ref<SceneEditableObject>>& objects,
                                                     const Vector<Layout>& doneLayouts):
        doneLayouts(doneLayouts)
    {
        objectsIds = objects.Convert<SceneUID>([](auto& x) { return x->GetID(); });
        beforeLayouts = objects.Convert<Layout>([](auto& x) { return x->GetLayout(); });
    }

    String WidgetLayerLayoutAction::GetName() const
    {
        return "Widget layer layout";
    }

    void WidgetLayerLayoutAction::Redo()
    {
        SetLayouts(doneLayouts);
    }

    void WidgetLayerLayoutAction::Undo()
    {
        SetLayouts(beforeLayouts);
    }

    void WidgetLayerLayoutAction::SetLayouts(const Vector<Layout>& layouts)
    {
        for (int i = 0; i < objectsIds.Count(); i++)
        {
            auto object = o2Scene.GetEditableObjectByID(objectsIds[i]);
            if (object)
            {
                object->SetLayout(layouts[i]);
                object->UpdateTransform();
            }
        }
    }
}
// --- META ---

DECLARE_CLASS(Editor::WidgetLayerLayoutAction, Editor__WidgetLayerLayoutAction);
// --- END META ---
