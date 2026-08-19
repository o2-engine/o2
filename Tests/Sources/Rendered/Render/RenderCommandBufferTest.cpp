#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/RenderCommandBuffer.h"

using namespace o2;

TEST(RenderCommandBuffer, EmplaceAndReset)
{
    RenderCommandBuffer buffer;
    EXPECT_TRUE(buffer.IsEmpty());

    buffer.Emplace().vertexCount = 7;
    buffer.Emplace().vertexCount = 9;

    EXPECT_EQ(buffer.Count(), 2);
    EXPECT_EQ(buffer.Get(0).vertexCount, 7u);
    EXPECT_EQ(buffer.Get(1).vertexCount, 9u);

    buffer.Reset();

    EXPECT_EQ(buffer.Count(), 0);
    EXPECT_TRUE(buffer.IsEmpty());
}

TEST(RenderCommandBuffer, ResetKeepsGeometryStoragePooled)
{
    RenderCommandBuffer buffer;

    auto& first = buffer.Emplace();
    first.vertexData.Resize(4096);
    first.indexData.Resize(512);

    const UInt8* vertexStorage = first.vertexData.data();
    const VertexIndex* indexStorage = first.indexData.data();

    buffer.Reset();

    // Recording a frame must not re-allocate the geometry buffers of the frame before it
    auto& reused = buffer.Emplace();
    EXPECT_EQ(reused.vertexData.data(), vertexStorage);
    EXPECT_EQ(reused.indexData.data(), indexStorage);
    EXPECT_GE(reused.vertexData.Count(), 4096);
    EXPECT_GE(reused.indexData.Count(), 512);
}

TEST(RenderCommandBuffer, ResetReleasesAssetReferences)
{
    RenderCommandBuffer buffer;

    auto material = mmake<Material>();
    auto& command = buffer.Emplace();
    command.material = material;
    command.extraRenderTargets.Add(TextureRef());

    buffer.Reset();

    EXPECT_FALSE(buffer.Get(0).material);
    EXPECT_TRUE(buffer.Get(0).extraRenderTargets.IsEmpty());
}
