#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Pipeline/DeferredPasses.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Properties/Basic/ColorProperty.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Properties/Basic/ObjectPtrProperty.h"
#include "o2Editor/Properties/Basic/VectorProperty.h"
#include "o2Editor/Properties/Basic/SceneLayersListProperty.h"
#include "o2Editor/Properties/Basic/Vector2FloatProperty.h"
#include "o2Editor/Properties/Objects/Actors/CameraActorViewer.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    class ObjectPtrPropertyAccessor: public ObjectPtrProperty
    {
    public:
        using ObjectPtrProperty::CreateObject;
    };


    template<typename _widget_type>
    void FindAllByType(const Ref<Widget>& widget, Vector<Ref<_widget_type>>& result)
    {
        if (auto typed = DynamicCast<_widget_type>(widget))
            result.Add(typed);

        for (auto& child : widget->GetChildWidgets())
            FindAllByType(child, result);
    }

    template<typename _widget_type>
    Ref<_widget_type> FindFirstByType(const Ref<Widget>& widget)
    {
        Vector<Ref<_widget_type>> found;
        FindAllByType(widget, found);
        return found.IsEmpty() ? nullptr : found[0];
    }

    Ref<ObjectPtrProperty> FindObjectPtrProperty(const Ref<Widget>& widget)
    {
        if (auto property = DynamicCast<ObjectPtrProperty>(widget))
            return property;

        for (auto& child : widget->GetChildWidgets())
        {
            if (auto found = FindObjectPtrProperty(child))
                return found;
        }

        return nullptr;
    }
}

// Camera actor inspector must contain the render pipeline object property,
// and creating a pipeline type through it must apply to the actor
TEST(CameraPipelinePropertyUI, InspectorShowsPipelineAndTypeChangeApplies)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    TickScene();

    auto viewer = mmake<CameraActorViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false);
    viewer->Refresh({ { dynamic_cast<IObject*>(camera.Get()), nullptr } });

    auto pipelineProperty = FindObjectPtrProperty(viewer->GetSpoiler());
    ASSERT_NE(pipelineProperty, nullptr) << "camera inspector must contain the pipeline object property";

    // Create deferred pipeline through the property, like the type selection menu does
    auto deferredType = dynamic_cast<const ObjectType*>(&TypeOf(DeferredPipeline));
    ASSERT_NE(deferredType, nullptr);

    static_cast<ObjectPtrPropertyAccessor*>(pipelineProperty.Get())->CreateObject(deferredType);

    auto pipeline = camera->GetRenderPipeline();
    ASSERT_NE(pipeline, nullptr) << "creating pipeline through the property must apply to the actor";
    EXPECT_NE(DynamicCast<DeferredPipeline>(pipeline), nullptr);
}

// Expanding the pipeline in the camera inspector must build editable parameters of every pass:
// enabled flags, shadow map size etc, and user edits must apply to the pipeline object
TEST(CameraPipelinePropertyUI, PassesParametersAreBuiltAndEditable)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    auto pipeline = mmake<DeferredPipeline>();
    camera->SetRenderPipeline(pipeline);
    TickScene();

    auto viewer = mmake<CameraActorViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false);
    viewer->Refresh({ { dynamic_cast<IObject*>(camera.Get()), nullptr } });

    auto pipelineProperty = FindObjectPtrProperty(viewer->GetSpoiler());
    ASSERT_NE(pipelineProperty, nullptr);

    pipelineProperty->SetExpanded(true, true);
    pipelineProperty->Refresh(true);
    pipelineProperty->SetExpanded(true, true);

    auto passesProperty = FindFirstByType<VectorProperty>(pipelineProperty);
    ASSERT_NE(passesProperty, nullptr) << "pipeline object viewer must contain the passes vector property";

    passesProperty->Expand(true);
    passesProperty->Refresh(true);

    Vector<Ref<ObjectPtrProperty>> passProperties;
    FindAllByType(Ref<Widget>(passesProperty), passProperties);
    ASSERT_EQ(passProperties.Count(), 5) << "every deferred pipeline pass must get an object property";

    for (auto& passProperty : passProperties)
    {
        passProperty->SetExpanded(true, true);

        Vector<Ref<IPropertyField>> passFields;
        FindAllByType(DynamicCast<Widget>(passProperty), passFields);
        // The pass object property itself is an IPropertyField, so nested fields make the count > 1
        EXPECT_GT(passFields.Count(), 1) << "pass must expose at least one editable parameter";
    }

    // Toggle 'enabled' of the first pass (ShadowMapPass) through the property
    auto shadowPassProperty = passProperties[0];
    auto enabledProperty = FindFirstByType<BooleanProperty>(shadowPassProperty);
    ASSERT_NE(enabledProperty, nullptr) << "pass enabled flag must be editable";

    EXPECT_TRUE(pipeline->GetPasses()[0]->IsEnabled());
    enabledProperty->SetValue(false, true);
    EXPECT_FALSE(pipeline->GetPasses()[0]->IsEnabled());

    // Change shadow map size through the property
    auto sizeProperty = FindFirstByType<IntegerProperty>(shadowPassProperty);
    ASSERT_NE(sizeProperty, nullptr) << "shadow map size must be editable";

    sizeProperty->SetValue(1024, true);
    auto shadowPass = DynamicCast<ShadowMapPass>(pipeline->GetPasses()[0]);
    ASSERT_NE(shadowPass, nullptr);
    EXPECT_EQ(shadowPass->GetShadowMapSize(), 1024);
}
