#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Default layer =====

TEST(SceneLayer, DefaultLayerExists)
{
    EXPECT_TRUE(o2Scene.GetDefaultLayer());
}

TEST(SceneLayer, DefaultLayerIsInLayersList)
{
    auto def = o2Scene.GetDefaultLayer();
    EXPECT_TRUE(o2Scene.GetLayers().Contains(def));
}

// ===== AddLayer =====

TEST(SceneLayer, AddLayerCreatesNewLayer)
{
    SceneCleanGuard guard;
    int countBefore = o2Scene.GetLayers().Count();
    auto layer = o2Scene.AddLayer("layer_test_unique_42");

    ASSERT_TRUE(layer);
    EXPECT_EQ(layer->GetName(), "layer_test_unique_42");
    EXPECT_EQ(o2Scene.GetLayers().Count(), countBefore + 1);
}

TEST(SceneLayer, HasLayerReturnsTrueForExisting)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("layer_has_test_42");
    EXPECT_TRUE(o2Scene.HasLayer("layer_has_test_42"));
    EXPECT_FALSE(o2Scene.HasLayer("layer_does_not_exist_42"));
}

TEST(SceneLayer, GetLayerByNameReturnsLayer)
{
    SceneCleanGuard guard;
    auto created = o2Scene.AddLayer("layer_get_test_42");
    auto fetched = o2Scene.GetLayer("layer_get_test_42");
    EXPECT_EQ(fetched, created);
}

// ===== RemoveLayer =====

TEST(SceneLayer, RemoveLayerByNameRemoves)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("layer_remove_42");
    EXPECT_TRUE(o2Scene.HasLayer("layer_remove_42"));

    o2Scene.RemoveLayer("layer_remove_42");
    EXPECT_FALSE(o2Scene.HasLayer("layer_remove_42"));
}

TEST(SceneLayer, RemoveLayerByRefRemoves)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("layer_remove_ref_42");
    EXPECT_TRUE(o2Scene.HasLayer("layer_remove_ref_42"));

    o2Scene.RemoveLayer(layer);
    EXPECT_FALSE(o2Scene.HasLayer("layer_remove_ref_42"));
}

// ===== Layer order =====

TEST(SceneLayer, SetLayerOrderMovesLayer)
{
    SceneCleanGuard guard;
    auto a = o2Scene.AddLayer("layer_order_a_42");
    auto b = o2Scene.AddLayer("layer_order_b_42");
    auto c = o2Scene.AddLayer("layer_order_c_42");

    o2Scene.SetLayerOrder(c, 0);

    auto idxC = o2Scene.GetLayers().IndexOf(c);
    auto idxA = o2Scene.GetLayers().IndexOf(a);
    EXPECT_LT(idxC, idxA);
}

// ===== GetLayersNames / GetLayersMap =====

TEST(SceneLayer, GetLayersNamesIncludesNewLayer)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("layer_names_42");
    auto names = o2Scene.GetLayersNames();
    EXPECT_TRUE(names.Contains("layer_names_42"));
}

TEST(SceneLayer, GetLayersMapHasNewLayer)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("layer_map_42");
    const auto& map = o2Scene.GetLayersMap();
    auto it = map.find("layer_map_42");
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second.Lock(), layer);
}

// ===== Actor <-> Layer =====

TEST(SceneLayer, SetLayerByRefAssignsToActor)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("layer_actor_ref_42");
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetLayer(layer);

    EXPECT_EQ(a->GetLayer(), layer);
}

TEST(SceneLayer, SetLayerByNameAssignsToActor)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("layer_actor_name_42");
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetLayer(String("layer_actor_name_42"));

    EXPECT_EQ(a->GetLayer(), layer);
}
