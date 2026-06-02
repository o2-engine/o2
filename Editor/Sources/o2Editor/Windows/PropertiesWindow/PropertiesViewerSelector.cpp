#include "o2Editor/stdafx.h"
#include "PropertiesViewerSelector.h"

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Windows/PropertiesWindow/DefaultPropertiesViewer.h"
#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"

namespace Editor
{
    void PropertiesViewerSelector::Initialize()
    {
        auto viewersTypes = TypeOf(IPropertiesViewer).GetDerivedTypes();
        viewersTypes.Remove(&TypeOf(DefaultPropertiesViewer));

        for (auto& type : viewersTypes)
            mViewers.Add(DynamicCast<IPropertiesViewer>(type->CreateSampleRef()));

        mDefaultViewer = mmake<DefaultPropertiesViewer>();
    }

    Ref<IPropertiesViewer> PropertiesViewerSelector::Select(const Vector<IObject*>& targets) const
    {
        if (auto viewer = SelectFrom(targets, mViewers))
            return viewer;

        return mDefaultViewer;
    }

    Ref<IPropertiesViewer> PropertiesViewerSelector::SelectFrom(const Vector<IObject*>& targets,
                                                               const Vector<Ref<IPropertiesViewer>>& viewers)
    {
        if (targets.IsEmpty())
            return nullptr;

        auto type = &targets[0]->GetType();
        return viewers.FindOrDefault([&](auto& x)
        {
            auto viewingType = x->GetViewingObjectType();
            return viewingType && type->IsBasedOn(*viewingType);
        });
    }
}
