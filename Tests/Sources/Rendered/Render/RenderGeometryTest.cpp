#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/RenderGeometry.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Utils/Bitmap/Bitmap.h"

using namespace o2;

// Geometry retained by its owner: render copies it into a batch only when the batch has changed since the previous frame

namespace
{
    const Color4 background(96, 96, 96, 255);

    struct Indexes: public RefCounterable
    {
        Vector<VertexIndex> indexes = { 0, 1, 2, 0, 2, 3 };
    };

    Ref<RenderGeometry> MakeQuad(const RectF& rect, const Color4& color)
    {
        auto indexes = mmake<Indexes>();

        auto geometry = mmake<RenderGeometry>();
        geometry->vertices =
        {
            Vertex(rect.left, rect.bottom, color.ABGR(), 0.0f, 0.0f), Vertex(rect.left, rect.top, color.ABGR(), 0.0f, 0.0f),
            Vertex(rect.right, rect.top, color.ABGR(), 0.0f, 0.0f), Vertex(rect.right, rect.bottom, color.ABGR(), 0.0f, 0.0f)
        };
        geometry->indexes = indexes->indexes.Data();
        geometry->trianglesCount = 2;
        geometry->indexesOwner = indexes;
        return geometry;
    }

    void DrawQuadBuffer(const RectF& rect, const Color4& color)
    {
        Vertex vertices[] =
        {
            Vertex(rect.left, rect.bottom, color.ABGR(), 0.0f, 0.0f), Vertex(rect.left, rect.top, color.ABGR(), 0.0f, 0.0f),
            Vertex(rect.right, rect.top, color.ABGR(), 0.0f, 0.0f), Vertex(rect.right, rect.bottom, color.ABGR(), 0.0f, 0.0f)
        };
        VertexIndex indexes[] = { 0, 1, 2, 0, 2, 3 };
        o2Render.DrawBuffer(PrimitiveType::Polygon, vertices, 4, indexes, 2, nullptr, TextureRef());
    }

    struct FrameResult
    {
        Ref<Bitmap>             bitmap;
        Render::BatchStatistics statistics;
    };

    FrameResult DrawFrame(const Function<void()>& draw, bool capture = false)
    {
        FrameResult res;
        if (capture)
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { res.bitmap = bitmap; });

        o2Render.Begin();
        o2Render.SetCamera(Camera());
        o2Render.Clear(background);
        draw();
        o2Render.End();

        res.statistics = o2Render.GetBatchStatistics();
        return res;
    }

    Color4 PixelAt(const Ref<Bitmap>& frame, int x, int y)
    {
        Vec2I frameSize = frame->GetSize();
        return VectorRasterizer::GetPixel(*frame, Math::RoundToInt(frameSize.x/2.0f) + x,
                                          Math::RoundToInt(frameSize.y/2.0f) - 1 - y);
    }

    void ExpectColor(const Color4& pixel, const Color4& color, const char* what)
    {
        EXPECT_NEAR(pixel.r, color.r, 2) << what;
        EXPECT_NEAR(pixel.g, color.g, 2) << what;
        EXPECT_NEAR(pixel.b, color.b, 2) << what;
    }

    class RenderGeometryDraw: public ::testing::Test
    {
    public:
        void SetUp() override
        {
            wasMultithreaded = o2Render.IsMultithreadedRenderEnabled();
            o2Render.SetMultithreadedRenderEnabled(true);
        }

        void TearDown() override
        {
            o2Render.SetMultithreadedRenderEnabled(wasMultithreaded);
        }

        // Batches are kept between frames by the recorded commands, which only the multithreaded render has
        bool IsRetaining() const
        {
            return o2Render.IsMultithreadedRenderEnabled();
        }

        bool wasMultithreaded = false;
    };
}

TEST_F(RenderGeometryDraw, SteadyGeometryIsCopiedOnce)
{
    if (!IsRetaining())
        GTEST_SKIP() << "the render does not record frames on this platform";

    auto first = MakeQuad(RectF(-40, 20, -20, 0), Color4::Red());
    auto second = MakeQuad(RectF(0, 20, 20, 0), Color4::Green());
    auto draw = [&]() { o2Render.DrawGeometry(first, nullptr); o2Render.DrawGeometry(second, nullptr); };

    FrameResult frame = DrawFrame(draw);
    EXPECT_EQ(frame.statistics.vertices, 8u);
    EXPECT_EQ(frame.statistics.retainedVertices, 0u) << "new geometry is copied";

    for (int i = 0; i < 3; i++)
    {
        frame = DrawFrame(draw);
        EXPECT_EQ(frame.statistics.vertices, 8u);
        EXPECT_EQ(frame.statistics.retainedVertices, 8u) << "frame " << i << ": the batch of the previous frame is kept";
    }

    second->vertices[0].x -= 5.0f;
    second->OnChanged();

    frame = DrawFrame(draw);
    EXPECT_EQ(frame.statistics.retainedVertices, 0u) << "changed geometry rebuilds its batch";

    frame = DrawFrame(draw);
    EXPECT_EQ(frame.statistics.retainedVertices, 8u);
}

TEST_F(RenderGeometryDraw, RetainedBatchDrawsTheSamePixelsAsCopiedBuffer)
{
    for (bool multithreaded : { true, false })
    {
        o2Render.SetMultithreadedRenderEnabled(multithreaded);

        auto quad = MakeQuad(RectF(-40, 20, -20, 0), Color4::Red());
        auto draw = [&]()
        {
            o2Render.DrawGeometry(quad, nullptr);
            DrawQuadBuffer(RectF(0, 20, 20, 0), Color4::Green());
        };

        // The frames after the first one take the batch of the quad from the previous frame
        FrameResult frame;
        for (int i = 0; i < 4; i++)
            frame = DrawFrame(draw, true);

        ASSERT_TRUE(frame.bitmap);
        ExpectColor(PixelAt(frame.bitmap, -30, 10), Color4::Red(), "retained quad");
        ExpectColor(PixelAt(frame.bitmap, 10, 10), Color4::Green(), "copied quad");
        ExpectColor(PixelAt(frame.bitmap, -10, 10), background, "between the quads");

        quad->vertices[0].x += 60.0f;
        quad->vertices[1].x += 60.0f;
        quad->vertices[2].x += 60.0f;
        quad->vertices[3].x += 60.0f;
        for (Vertex& vertex : quad->vertices)
            vertex.color = Color4::Blue().ABGR();

        quad->OnChanged();

        for (int i = 0; i < 3; i++)
            frame = DrawFrame(draw, true);

        ASSERT_TRUE(frame.bitmap);
        ExpectColor(PixelAt(frame.bitmap, -30, 10), background, "the place the quad has left");
        ExpectColor(PixelAt(frame.bitmap, 30, 10), Color4::Blue(), "moved quad");
        ExpectColor(PixelAt(frame.bitmap, 10, 10), Color4::Green(), "copied quad");
    }
}

TEST_F(RenderGeometryDraw, GeometryIsQueuedTillItsBatchIsSent)
{
    auto quad = MakeQuad(RectF(-40, 20, -20, 0), Color4::Red());
    EXPECT_FALSE(quad->IsQueued());

    bool queuedInFrame = false, queuedAfterOtherBatch = true;
    DrawFrame([&]()
    {
        o2Render.DrawGeometry(quad, nullptr);
        queuedInFrame = quad->IsQueued();

        // A copied buffer closes the batch of retained geometries
        DrawQuadBuffer(RectF(0, 20, 20, 0), Color4::Green());
        queuedAfterOtherBatch = quad->IsQueued();

        o2Render.DrawGeometry(quad, nullptr);
    });

    EXPECT_TRUE(queuedInFrame);
    EXPECT_FALSE(queuedAfterOtherBatch);
    EXPECT_FALSE(quad->IsQueued()) << "the end of the frame sends all batches";
    EXPECT_EQ(o2Render.GetBatchStatistics().geometryBreaks, 2u);
}

TEST_F(RenderGeometryDraw, VersionsAreUniqueBetweenGeometries)
{
    auto first = MakeQuad(RectF(-40, 20, -20, 0), Color4::Red());
    auto second = MakeQuad(RectF(-40, 20, -20, 0), Color4::Red());
    EXPECT_NE(first->GetVersion(), second->GetVersion());

    UInt64 version = first->GetVersion();
    first->OnChanged();
    EXPECT_NE(first->GetVersion(), version);
    EXPECT_NE(first->GetVersion(), second->GetVersion());
}

TEST_F(RenderGeometryDraw, AnotherGeometryOfTheSameSizeReplacesTheBatch)
{
    auto red = MakeQuad(RectF(-40, 20, -20, 0), Color4::Red());
    auto blue = MakeQuad(RectF(-40, 20, -20, 0), Color4::Blue());

    FrameResult frame;
    for (int i = 0; i < 3; i++)
        frame = DrawFrame([&]() { o2Render.DrawGeometry(red, nullptr); }, true);

    ExpectColor(PixelAt(frame.bitmap, -30, 10), Color4::Red(), "first geometry");

    for (int i = 0; i < 3; i++)
        frame = DrawFrame([&]() { o2Render.DrawGeometry(blue, nullptr); }, true);

    ExpectColor(PixelAt(frame.bitmap, -30, 10), Color4::Blue(), "second geometry in the same batch");
}

TEST_F(RenderGeometryDraw, EmptyGeometryDrawsNothing)
{
    auto empty = mmake<RenderGeometry>();
    FrameResult frame = DrawFrame([&]() { o2Render.DrawGeometry(empty, nullptr); o2Render.DrawGeometry(nullptr, nullptr); });
    EXPECT_EQ(frame.statistics.vertices, 0u);
    EXPECT_FALSE(empty->IsQueued());
}

TEST_F(RenderGeometryDraw, RectOutOfTargetOrScissorIsClipped)
{
    bool inside = true, outside = false, crossing = true, outOfScissor = false, inScissor = true, zoomedOut = true;
    bool perspective = true;
    DrawFrame([&]()
    {
        Vec2F half = (Vec2F)o2Render.GetCurrentResolution()*0.5f;

        inside = o2Render.IsClipped(RectF(-10, 10, 10, -10));
        outside = o2Render.IsClipped(RectF(half.x + 5, 10, half.x + 25, -10));
        crossing = o2Render.IsClipped(RectF(half.x - 5, 10, half.x + 25, -10));

        o2Render.EnableScissorTest(RectI(-50, 50, 50, -50));
        outOfScissor = o2Render.IsClipped(RectF(60, 10, 80, -10));
        inScissor = o2Render.IsClipped(RectF(40, 10, 80, -10));
        o2Render.DisableScissorTest();

        // The rectangle is in the space of the camera: twice smaller on the screen it is inside
        Camera camera;
        camera.SetScale(Vec2F(2.0f, 2.0f));
        o2Render.SetCamera(camera);
        zoomedOut = o2Render.IsClipped(RectF(half.x + 5, 10, half.x + 25, -10));

        camera = Camera();
        camera.projection = Camera::Projection::Perspective;
        o2Render.SetCamera(camera);
        perspective = o2Render.IsClipped(RectF(half.x + 5, 10, half.x + 25, -10));
        o2Render.SetCamera(Camera());
    });

    EXPECT_FALSE(inside);
    EXPECT_TRUE(outside);
    EXPECT_FALSE(crossing);
    EXPECT_TRUE(outOfScissor);
    EXPECT_FALSE(inScissor);
    EXPECT_FALSE(zoomedOut);
    EXPECT_FALSE(perspective) << "a rectangle of the 3D space is never clipped";
}
