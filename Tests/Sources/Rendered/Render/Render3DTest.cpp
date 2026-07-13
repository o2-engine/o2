#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vertex.h"

using namespace o2;

namespace
{
    void FillQuad(Mesh& mesh, float halfSize, float z, const Color4& color)
    {
        ULong dcolor = color.ABGR();
        Vertex* verts = mesh.GetVertices<Vertex>();
        verts[0] = Vertex(-halfSize, -halfSize, z, dcolor, 0.0f, 0.0f);
        verts[1] = Vertex(halfSize, -halfSize, z, dcolor, 1.0f, 0.0f);
        verts[2] = Vertex(halfSize, halfSize, z, dcolor, 1.0f, 1.0f);
        verts[3] = Vertex(-halfSize, halfSize, z, dcolor, 0.0f, 1.0f);

        VertexIndex* indexes = mesh.GetIndexes();
        indexes[0] = 0; indexes[1] = 1; indexes[2] = 2;
        indexes[3] = 0; indexes[4] = 2; indexes[5] = 3;

        mesh.vertexCount = 4;
        mesh.polyCount = 2;
    }
}

TEST(Render3D, PerspectiveCameraDrawSmoke)
{
    o2Render.Begin();
    o2Render.Clear(Color4::Black());

    Camera perspective = Camera::Perspective(Math::Deg2rad(60.0f), 0.1f, 1000.0f);
    perspective.position = Vec3F(0.0f, 0.0f, 0.0f);
    o2Render.SetCamera(perspective);
    EXPECT_EQ(o2Render.GetCamera().projection, Camera::Projection::Perspective);

    Mesh mesh(TextureRef(), 4, 2);
    FillQuad(mesh, 2.0f, -5.0f, Color4::White());
    mesh.Draw();

    o2Render.SetCamera(Camera());
    EXPECT_EQ(o2Render.GetCamera().projection, Camera::Projection::Orthographic);

    o2Render.DrawFilledPolygon({ Vec2F(-10, -10), Vec2F(10, -10), Vec2F(10, 10), Vec2F(-10, 10) }, Color4::Red());

    o2Render.End();
}

TEST(Render3D, DepthTestToggleSmoke)
{
    o2Render.Begin();
    o2Render.Clear(Color4::Black());

    EXPECT_FALSE(o2Render.IsDepthTestEnabled());

    o2Render.SetDepthTestEnabled(true);
    EXPECT_TRUE(o2Render.IsDepthTestEnabled());

    Mesh mesh(TextureRef(), 4, 2);
    FillQuad(mesh, 10.0f, 1.0f, Color4::Green());
    mesh.Draw();

    o2Render.SetDepthTestEnabled(false);
    EXPECT_FALSE(o2Render.IsDepthTestEnabled());

    FillQuad(mesh, 10.0f, 1.0f, Color4::Blue());
    mesh.Draw();

    o2Render.End();

    o2Render.Begin();
    EXPECT_FALSE(o2Render.IsDepthTestEnabled());
    o2Render.End();
}

TEST(Render3D, DepthTestNearGeometryWinsOverFarDrawnLater)
{
    Ref<Bitmap> captured;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

    o2Render.Begin();
    o2Render.Clear(Color4::Black());

    Camera perspective = Camera::Perspective(Math::Deg2rad(60.0f), 0.1f, 1000.0f);
    o2Render.SetCamera(perspective);

    o2Render.SetDepthTestEnabled(true);

    // Near green quad covers screen center, far red quad covers everything and is drawn after
    Mesh nearQuad(TextureRef(), 4, 2);
    FillQuad(nearQuad, 2.0f, -5.0f, Color4::Green());
    nearQuad.Draw();

    Mesh farQuad(TextureRef(), 4, 2);
    FillQuad(farQuad, 40.0f, -10.0f, Color4::Red());
    farQuad.Draw();

    o2Render.SetDepthTestEnabled(false);
    o2Render.SetCamera(Camera());

    o2Render.End();

    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    const UInt8* data = captured->GetData();
    int centerOffset = ((size.y/2)*size.x + size.x/2)*4;

    // Near quad must win at center: green channel is byte 1 in both RGBA and BGRA layouts
    EXPECT_GT((int)data[centerOffset + 1], 200);

    // Far quad must win at corner area not covered by the near quad (red, so green stays low)
    int cornerOffset = ((size.y/8)*size.x + size.x/8)*4;
    EXPECT_LT((int)data[cornerOffset + 1], 60);
}
