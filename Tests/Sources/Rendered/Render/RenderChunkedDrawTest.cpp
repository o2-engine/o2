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
    // Grid of quads centered at origin, more indices than the batch buffers capacity
    void FillGrid(Mesh& mesh, int cells, float size, float z, const Color4& color)
    {
        ULong dcolor = color.ABGR();
        Vertex* verts = mesh.GetVertices<Vertex>();
        VertexIndex* indexes = mesh.GetIndexes();

        int pointsPerSide = cells + 1;
        float step = size/(float)cells;
        float half = size*0.5f;

        for (int y = 0; y < pointsPerSide; y++)
        {
            for (int x = 0; x < pointsPerSide; x++)
                verts[y*pointsPerSide + x] = Vertex(-half + x*step, -half + y*step, z, dcolor, 0.0f, 0.0f);
        }

        int index = 0;
        for (int y = 0; y < cells; y++)
        {
            for (int x = 0; x < cells; x++)
            {
                VertexIndex leftBottom = y*pointsPerSide + x;
                indexes[index++] = leftBottom; indexes[index++] = leftBottom + 1; indexes[index++] = leftBottom + pointsPerSide + 1;
                indexes[index++] = leftBottom; indexes[index++] = leftBottom + pointsPerSide + 1; indexes[index++] = leftBottom + pointsPerSide;
            }
        }

        mesh.vertexCount = pointsPerSide*pointsPerSide;
        mesh.polyCount = cells*cells*2;
    }
}

// Single mesh with more indices and vertices than batch buffers capacity must draw in chunks without crash
TEST(RenderChunkedDraw, OversizedMeshDrawsCorrectly)
{
    Ref<Bitmap> captured;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

    o2Render.Begin();
    o2Render.Clear(Color4::Red());

    Camera perspective = Camera::Perspective(Math::Deg2rad(60.0f), 0.1f, 1000.0f);
    o2Render.SetCamera(perspective);

    // Grid covers the screen center but not the corners at this distance
    const int cells = 260; // 68121 vertices, 405600 indices - both above the 65535 batch capacity
    Mesh mesh(TextureRef(), (cells + 1)*(cells + 1), cells*cells*2);
    FillGrid(mesh, cells, 3.0f, -5.0f, Color4::Green());
    mesh.Draw();

    o2Render.SetCamera(Camera());
    o2Render.End();

    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();

    // Grid covers screen center: every sampled pixel inside it must be green with no holes
    for (int dy = -30; dy <= 30; dy += 10)
    {
        for (int dx = -30; dx <= 30; dx += 10)
        {
            int offset = ((size.y/2 + dy)*size.x + size.x/2 + dx)*4;
            EXPECT_GT((int)data[offset + 1], 200) << "at " << dx << ", " << dy;
        }
    }

    // Outside the grid the clear color must remain
    int cornerOffset = (10*size.x + 10)*4;
    EXPECT_LT((int)data[cornerOffset + 1], 60);
}
