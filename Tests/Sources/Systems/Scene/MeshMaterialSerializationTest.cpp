#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/MaterialAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/Mesh3DComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// Material of a 3D mesh component is an owned material asset instance: it must
// survive the scene save/load round trip together with its attachment formats
TEST(MeshMaterial, PrimitiveComponentMaterialAssetRoundTrip)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->SetName("mesh actor");
    auto component = actor->AddComponent<MeshPrimitiveComponent>();

    AssetRef<MaterialAsset> materialRef;
    materialRef.CreateInstance();
    materialRef->SetColorAttachmentFormats({ TextureFormat::R8G8B8A8, TextureFormat::R16G16B16A16F,
                                             TextureFormat::R16G16B16A16F });
    component->SetMaterialAsset(materialRef);

    TickFrame();

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(document);
    TickFrame();

    auto loadedActor = o2Scene.FindActor("mesh actor");
    ASSERT_TRUE(loadedActor);

    auto loadedComponent = loadedActor->GetComponent<MeshPrimitiveComponent>();
    ASSERT_TRUE(loadedComponent);

    auto loadedMaterial = loadedComponent->GetMaterialAsset();
    ASSERT_TRUE(loadedMaterial);

    Vector<TextureFormat> expectedFormats{ TextureFormat::R8G8B8A8, TextureFormat::R16G16B16A16F,
                                           TextureFormat::R16G16B16A16F };
    EXPECT_EQ(loadedMaterial->GetColorAttachmentFormats(), expectedFormats);

    EXPECT_EQ(loadedComponent->GetMaterial(), Ref<Material>(loadedMaterial.GetRef()));
}

TEST(MeshMaterial, Mesh3DComponentMaterialAssetRoundTrip)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->SetName("mesh 3d actor");
    auto component = actor->AddComponent<Mesh3DComponent>();

    AssetRef<MaterialAsset> materialRef;
    materialRef.CreateInstance();
    materialRef->SetColorAttachmentFormats({ TextureFormat::R16G16B16A16F });
    component->SetMaterialAsset(materialRef);

    TickFrame();

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(document);
    TickFrame();

    auto loadedActor = o2Scene.FindActor("mesh 3d actor");
    ASSERT_TRUE(loadedActor);

    auto loadedComponent = loadedActor->GetComponent<Mesh3DComponent>();
    ASSERT_TRUE(loadedComponent);

    auto loadedMaterial = loadedComponent->GetMaterialAsset();
    ASSERT_TRUE(loadedMaterial);

    EXPECT_EQ(loadedMaterial->GetColorAttachmentFormats(), Vector<TextureFormat>{ TextureFormat::R16G16B16A16F });
}

// Direct (non-asset) material override is runtime only: it applies to the mesh
// but must not leak into serialization
TEST(MeshMaterial, DirectMaterialIsRuntimeOnly)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->SetName("mesh actor");
    auto component = actor->AddComponent<MeshPrimitiveComponent>();

    auto material = mmake<Material>();
    component->SetMaterial(material);
    EXPECT_EQ(component->GetMaterial(), material);

    TickFrame();

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(document);
    TickFrame();

    auto loadedActor = o2Scene.FindActor("mesh actor");
    ASSERT_TRUE(loadedActor);

    auto loadedComponent = loadedActor->GetComponent<MeshPrimitiveComponent>();
    ASSERT_TRUE(loadedComponent);
    EXPECT_EQ(loadedComponent->GetMaterial(), nullptr);
    EXPECT_FALSE(loadedComponent->GetMaterialAsset());
}
