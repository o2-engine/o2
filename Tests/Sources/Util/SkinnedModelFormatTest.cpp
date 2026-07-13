#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/SkinnedModelFormat.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    void Append(Vector<UInt8>& out, const void* data, int size)
    {
        const UInt8* bytes = (const UInt8*)data;
        for (int i = 0; i < size; i++)
            out.Add(bytes[i]);
    }

    // Two-bone chain: root at origin, child at (0, 1, 0); a quad where the bottom
    // vertices are bound to the root and the top ones to the child. The "move" clip
    // translates the child from (0,1,0) to (0,3,0) over 1 second; the "step" clip
    // does the same with STEP interpolation
    Vector<UInt8> BuildTestGlb()
    {
        Vector<UInt8> bin;

        // positions: 4 * vec3 float, offset 0
        float positions[12] = { 0, 0, 0,  0, 1, 0,  1, 0, 0,  1, 1, 0 };
        Append(bin, positions, sizeof(positions));

        // joints: 4 * 4 ushort, offset 48
        UInt16 joints[16] = { 0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0 };
        Append(bin, joints, sizeof(joints));

        // weights: 4 * 4 float, offset 80
        float weights[16] = { 1, 0, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0 };
        Append(bin, weights, sizeof(weights));

        // indices: 6 ushort, offset 144
        UInt16 indices[6] = { 0, 1, 2, 1, 3, 2 };
        Append(bin, indices, sizeof(indices));

        // inverse bind matrices: 2 * mat4 float, offset 156
        float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        float childInverseBind[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -1, 0, 1 };
        Append(bin, identity, sizeof(identity));
        Append(bin, childInverseBind, sizeof(childInverseBind));

        // animation input times: 2 floats, offset 284
        float times[2] = { 0.0f, 1.0f };
        Append(bin, times, sizeof(times));

        // animation output translations: 2 * vec3 float, offset 292
        float animValues[6] = { 0, 1, 0,  0, 3, 0 };
        Append(bin, animValues, sizeof(animValues));

        const char* json = R"({
"asset": {"version": "2.0"},
"scenes": [{"nodes": [0]}],
"nodes": [
 {"name": "root", "children": [1]},
 {"name": "child", "translation": [0, 1, 0]}
],
"meshes": [{"primitives": [{
 "attributes": {"POSITION": 0, "JOINTS_0": 1, "WEIGHTS_0": 2},
 "indices": 3
}]}],
"skins": [{"joints": [0, 1], "inverseBindMatrices": 4}],
"animations": [
 {"name": "move", "samplers": [{"input": 5, "output": 6, "interpolation": "LINEAR"}],
  "channels": [{"sampler": 0, "target": {"node": 1, "path": "translation"}}]},
 {"name": "step", "samplers": [{"input": 5, "output": 6, "interpolation": "STEP"}],
  "channels": [{"sampler": 0, "target": {"node": 1, "path": "translation"}}]}
],
"buffers": [{"byteLength": 316}],
"bufferViews": [
 {"buffer": 0, "byteOffset": 0, "byteLength": 48},
 {"buffer": 0, "byteOffset": 48, "byteLength": 32},
 {"buffer": 0, "byteOffset": 80, "byteLength": 64},
 {"buffer": 0, "byteOffset": 144, "byteLength": 12},
 {"buffer": 0, "byteOffset": 156, "byteLength": 128},
 {"buffer": 0, "byteOffset": 284, "byteLength": 8},
 {"buffer": 0, "byteOffset": 292, "byteLength": 24}
],
"accessors": [
 {"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3"},
 {"bufferView": 1, "componentType": 5123, "count": 4, "type": "VEC4"},
 {"bufferView": 2, "componentType": 5126, "count": 4, "type": "VEC4"},
 {"bufferView": 3, "componentType": 5123, "count": 6, "type": "SCALAR"},
 {"bufferView": 4, "componentType": 5126, "count": 2, "type": "MAT4"},
 {"bufferView": 5, "componentType": 5126, "count": 2, "type": "SCALAR", "min": [0.0], "max": [1.0]},
 {"bufferView": 6, "componentType": 5126, "count": 2, "type": "VEC3"}
]})";

        int jsonLength = (int)strlen(json);
        int jsonPadded = (jsonLength + 3)/4*4;
        int binPadded = (bin.Count() + 3)/4*4;

        Vector<UInt8> glb;
        UInt32 header[3] = { 0x46546C67, 2, (UInt32)(12 + 8 + jsonPadded + 8 + binPadded) };
        Append(glb, header, sizeof(header));

        UInt32 jsonChunk[2] = { (UInt32)jsonPadded, 0x4E4F534A };
        Append(glb, jsonChunk, sizeof(jsonChunk));
        Append(glb, json, jsonLength);
        for (int i = jsonLength; i < jsonPadded; i++)
            glb.Add((UInt8)' ');

        UInt32 binChunk[2] = { (UInt32)binPadded, 0x004E4942 };
        Append(glb, binChunk, sizeof(binChunk));
        for (auto byte : bin)
            glb.Add(byte);
        for (int i = bin.Count(); i < binPadded; i++)
            glb.Add((UInt8)0);

        return glb;
    }

    bool NearVec3(const Vec3F& a, const Vec3F& b, float eps = kEps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }
}

TEST(SkinnedModelFormat, ParsesSyntheticGlb)
{
    auto glb = BuildTestGlb();

    SkinnedModelData model;
    String error;
    ASSERT_TRUE(GlbModelFormat::Parse(glb.Data(), (UInt)glb.Count(), model, &error)) << error;

    EXPECT_EQ(model.positions.Count(), 4);
    EXPECT_EQ(model.indices.Count(), 6);
    EXPECT_EQ(model.influences.Count(), 4);

    ASSERT_EQ(model.nodes.Count(), 2);
    EXPECT_EQ(model.nodes[0].name, "root");
    EXPECT_EQ(model.nodes[0].parent, -1);
    EXPECT_EQ(model.nodes[1].name, "child");
    EXPECT_EQ(model.nodes[1].parent, 0);
    EXPECT_TRUE(NearVec3(model.nodes[1].position, Vec3F(0, 1, 0)));

    ASSERT_EQ(model.joints.Count(), 2);
    EXPECT_EQ(model.joints[0], 0);
    EXPECT_EQ(model.joints[1], 1);
    ASSERT_EQ(model.inverseBindMatrices.Count(), 2);

    ASSERT_EQ(model.animations.Count(), 2);
    EXPECT_EQ(model.animations[0].name, "move");
    EXPECT_FALSE(model.animations[0].channels[0].step);
    EXPECT_NEAR(model.animations[0].duration, 1.0f, kEps);
    EXPECT_EQ(model.animations[1].name, "step");
    EXPECT_TRUE(model.animations[1].channels[0].step);

    EXPECT_EQ(model.influences[1].joints[0], 1);
    EXPECT_NEAR(model.influences[1].weights[0], 1.0f, kEps);
}

TEST(SkinnedModelFormat, BindPosePaletteIsIdentity)
{
    auto glb = BuildTestGlb();

    SkinnedModelData model;
    ASSERT_TRUE(GlbModelFormat::Parse(glb.Data(), (UInt)glb.Count(), model));

    Vector<Mat4> palette;
    model.EvaluateJointsPalette(-1, 0.0f, palette);

    ASSERT_EQ(palette.Count(), 2);
    for (auto& matrix : palette)
        EXPECT_EQ(matrix, Mat4::Identity());

    Vector<Vec3F> skinnedPositions, skinnedNormals;
    model.SkinVertices(palette, skinnedPositions, skinnedNormals);

    ASSERT_EQ(skinnedPositions.Count(), 4);
    for (int i = 0; i < 4; i++)
        EXPECT_TRUE(NearVec3(skinnedPositions[i], model.positions[i]));

    // Quad in the XY plane: computed smooth normals are along Z
    ASSERT_EQ(skinnedNormals.Count(), 4);
    for (auto& normal : skinnedNormals)
        EXPECT_NEAR(Math::Abs(normal.z), 1.0f, kEps);
}

TEST(SkinnedModelFormat, LinearAnimationSamplingMovesBoundVertices)
{
    auto glb = BuildTestGlb();

    SkinnedModelData model;
    ASSERT_TRUE(GlbModelFormat::Parse(glb.Data(), (UInt)glb.Count(), model));

    int animation = model.FindAnimation("move");
    ASSERT_GE(animation, 0);

    Vector<Mat4> palette;
    Vector<Vec3F> skinnedPositions, skinnedNormals;

    // Middle of the clip: the child joint is between keys, bound vertices follow it
    model.EvaluateJointsPalette(animation, 0.5f, palette);
    model.SkinVertices(palette, skinnedPositions, skinnedNormals);

    EXPECT_TRUE(NearVec3(skinnedPositions[0], Vec3F(0, 0, 0))) << "root-bound vertex must stay";
    EXPECT_TRUE(NearVec3(skinnedPositions[1], Vec3F(0, 2, 0))) << "child-bound vertex must move by the animation";

    // Clip end
    model.EvaluateJointsPalette(animation, 1.0f, palette);
    model.SkinVertices(palette, skinnedPositions, skinnedNormals);
    EXPECT_TRUE(NearVec3(skinnedPositions[1], Vec3F(0, 3, 0)));
    EXPECT_TRUE(NearVec3(skinnedPositions[3], Vec3F(1, 3, 0)));
}

TEST(SkinnedModelFormat, StepAnimationHoldsLeftKey)
{
    auto glb = BuildTestGlb();

    SkinnedModelData model;
    ASSERT_TRUE(GlbModelFormat::Parse(glb.Data(), (UInt)glb.Count(), model));

    int animation = model.FindAnimation("step");
    ASSERT_GE(animation, 0);

    Vector<Mat4> palette;
    Vector<Vec3F> skinnedPositions, skinnedNormals;

    model.EvaluateJointsPalette(animation, 0.5f, palette);
    model.SkinVertices(palette, skinnedPositions, skinnedNormals);
    EXPECT_TRUE(NearVec3(skinnedPositions[1], Vec3F(0, 1, 0))) << "STEP holds the left key until the next one";

    model.EvaluateJointsPalette(animation, 1.0f, palette);
    model.SkinVertices(palette, skinnedPositions, skinnedNormals);
    EXPECT_TRUE(NearVec3(skinnedPositions[1], Vec3F(0, 3, 0)));
}

TEST(SkinnedModelFormat, ComputeSmoothNormalsForTriangle)
{
    Vector<Vec3F> positions{ Vec3F(0, 0, 0), Vec3F(1, 0, 0), Vec3F(0, 1, 0) };
    Vector<UInt> indices{ 0, 1, 2 };

    Vector<Vec3F> normals;
    SkinnedModelData::ComputeSmoothNormals(positions, indices, normals);

    ASSERT_EQ(normals.Count(), 3);
    for (auto& normal : normals)
    {
        EXPECT_NEAR(normal.x, 0.0f, kEps);
        EXPECT_NEAR(normal.y, 0.0f, kEps);
        EXPECT_NEAR(Math::Abs(normal.z), 1.0f, kEps);
    }
}

TEST(SkinnedModelFormat, RejectsCorruptedData)
{
    SkinnedModelData model;
    String error;

    EXPECT_FALSE(GlbModelFormat::Parse(nullptr, 0, model, &error));

    Vector<UInt8> garbage;
    for (int i = 0; i < 64; i++)
        garbage.Add((UInt8)i);

    EXPECT_FALSE(GlbModelFormat::Parse(garbage.Data(), (UInt)garbage.Count(), model, &error));
    EXPECT_FALSE(error.IsEmpty());
}
