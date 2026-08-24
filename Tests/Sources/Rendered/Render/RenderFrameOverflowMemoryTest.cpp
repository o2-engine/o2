#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vertex.h"

#if defined(PLATFORM_MAC)
#include <mach/mach.h>
#endif

using namespace o2;

namespace
{
    size_t ResidentMemoryBytes()
    {
#if defined(PLATFORM_MAC)
        mach_task_basic_info info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
            return 0;

        return info.resident_size;
#else
        return 0;
#endif
    }

    void FillGrid(Mesh& mesh, int cells, float size)
    {
        ULong color = Color4::Green().ABGR();
        Vertex* verts = mesh.GetVertices<Vertex>();
        VertexIndex* indexes = mesh.GetIndexes();

        int pointsPerSide = cells + 1;
        float step = size/(float)cells;
        float half = size*0.5f;

        for (int y = 0; y < pointsPerSide; y++)
        {
            for (int x = 0; x < pointsPerSide; x++)
                verts[y*pointsPerSide + x] = Vertex(-half + x*step, -half + y*step, 0.0f, color, 0.0f, 0.0f);
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

// A frame whose geometry overflows the platform frame buffers retires them and continues in
// fresh ones; sustained overflow frames must not grow memory: the retired buffers are recycled
TEST(RenderFrameOverflowMemory, SustainedOverflowFramesKeepMemoryFlat)
{
#if !defined(PLATFORM_MAC)
    GTEST_SKIP() << "resident memory sampling is implemented for mac only";
#endif

    // Two draws of this grid put the frame well past the 65535-vertex frame buffers
    const int cells = 250; // 63001 vertices, 125000 polygons per draw
    Mesh mesh(TextureRef(), (cells + 1)*(cells + 1), cells*cells*2);
    FillGrid(mesh, cells, 800.0f);

    auto drawFrame = [&]()
    {
        o2Render.Begin();
        o2Render.Clear(Color4::Black());
        o2Render.SetCamera(Camera());
        mesh.Draw();
        mesh.Draw();
        o2Render.End();
    };

    for (int i = 0; i < 30; i++)
        drawFrame();

    size_t warmedUp = ResidentMemoryBytes();

    for (int i = 0; i < 300; i++)
        drawFrame();

    size_t after = ResidentMemoryBytes();
    long long growthMb = ((long long)after - (long long)warmedUp)/(1024*1024);

    EXPECT_LE(growthMb, 100) << "frame buffer overflow leaks memory: +" << growthMb << " MB over 300 frames";
}
