#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "o2/Render/Render.h"
#include "o2/Render/Video.h"
#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "Video/VideoTestData.h"

using namespace o2;

// End-to-end video path: decode the embedded MPEG-1 clip into a texture, draw it like a
// sprite and check pixels. The clip is a solid green frame with a centered red square.
namespace
{
    // Draws the video as a 256x256 quad over a blue background and returns the captured frame.
    // Sampling: blue clear shows where the video is transparent (keyed out).
    Ref<Bitmap> DrawVideoCaptured(Video& video)
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.Clear(Color4(0, 0, 255, 255));
        o2Render.SetCamera(Camera());

        video.SetSize(Vec2F(256.0f, 256.0f));
        video.SetPosition(Vec2F(0.0f, 0.0f));
        video.Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();

        return captured;
    }
}

// One suite per case: ctest runs each in its own process for a fresh frame capture
TEST(VideoDraw, DecodesAndDrawsFrame)
{
    Video video(Tests::MakeTestVideoAsset());
    ASSERT_EQ(video.GetVideoSize(), Vec2I(64, 64));

    auto captured = DrawVideoCaptured(video);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();
    auto pixel = [&](int x, int y) { return data + (y*size.x + x)*4; };
    int cx = size.x/2, cy = size.y/2;

    // Center: red square, drawn opaque
    const UInt8* center = pixel(cx, cy);
    EXPECT_GT(center[0], 150); EXPECT_LT(center[1], 120); EXPECT_LT(center[2], 120);

    // Off-center but inside the quad: green background, opaque (not the blue clear)
    const UInt8* green = pixel(cx - 80, cy - 80);
    EXPECT_LT(green[0], 120); EXPECT_GT(green[1], 120); EXPECT_LT(green[2], 120);
}

TEST(VideoChromaKey, KeysOutBackgroundKeepsForeground)
{
    Video video(Tests::MakeTestVideoAsset());

    video.SetKeyColor(Color4(0, 177, 64, 255)); // fixture background green 0x00B140
    video.SetSimilarity(0.4f);
    video.SetSmoothness(0.1f);
    video.SetSpill(0.1f);
    video.SetChromaKeyEnabled(true);

    auto captured = DrawVideoCaptured(video);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();
    auto pixel = [&](int x, int y) { return data + (y*size.x + x)*4; };
    int cx = size.x/2, cy = size.y/2;

    // Center: red foreground kept opaque
    const UInt8* center = pixel(cx, cy);
    EXPECT_GT(center[0], 150); EXPECT_LT(center[2], 120);

    // Off-center green background keyed out -> blue clear shows through
    const UInt8* keyed = pixel(cx - 80, cy - 80);
    EXPECT_LT(keyed[1], 130); EXPECT_GT(keyed[2], 150);
}

// The frame follows the animation time (IAnimation), so a Video can drive an animation
// sub-track. The moving clip has a red square sliding left->right over time.
TEST(VideoAnimation, FrameFollowsAnimationTime)
{
    Video video(Tests::MakeMovingVideoAsset());
    video.SetSubControlled(true); // as when driven by a parent animation

    auto redAt = [](const Ref<Bitmap>& b, int x, int y)
    {
        const UInt8* p = b->GetData() + (y*b->GetSize().x + x)*4;
        return p[0] > 150 && p[1] < 120 && p[2] < 120;
    };

    // At time 0 the square is on the left (video x ~12 -> screen cx-80), not at cx-8
    video.SetTime(0.0f);
    auto f0 = DrawVideoCaptured(video);
    ASSERT_TRUE(f0);
    int cx = f0->GetSize().x/2, cy = f0->GetSize().y/2;
    EXPECT_TRUE(redAt(f0, cx - 80, cy));
    EXPECT_FALSE(redAt(f0, cx - 8, cy));

    // At time 0.48 the square has moved to the center (video x ~30 -> screen cx-8)
    video.SetTime(0.48f);
    auto f1 = DrawVideoCaptured(video);
    ASSERT_TRUE(f1);
    EXPECT_TRUE(redAt(f1, cx - 8, cy));
    EXPECT_FALSE(redAt(f1, cx - 80, cy));
}

#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
// Per-stage cost of the hardware video pipeline on a real 720p clip; prints a breakdown
// and smoke-checks the per-frame budget. Skipped when the game clip isn't built.
TEST(VideoPerf, HardwareDecodePipelineCost)
{
    using Clock = std::chrono::high_resolution_clock;
    auto ms = [](Clock::time_point a, Clock::time_point b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    };

    // Tests run with cwd at the exe dir; the real game clip lives in BuiltAssets
    std::ifstream src("../../BuiltAssets/Mac/Data/output.mp4", std::ios::binary);
    if (!src)
        GTEST_SKIP() << "no built output.mp4 to measure on";

    std::vector<char> bytes((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());

    auto asset = mmake<VideoAsset>();
    asset->SetPath("o2_video_perf.mp4");
    String builtPath = asset->GetBuiltFullPath();
    {
        std::ofstream file(builtPath.Data(), std::ios::binary);
        file.write(bytes.data(), (std::streamsize)bytes.size());
    }

    // Stage timings straight through the decoder
    {
        auto decoder = CreateVideoDecoder(asset, false);
        ASSERT_TRUE(decoder);
        Vec2I size = decoder->GetSize();
        Bitmap frame(PixelFormat::R8G8B8A8, size);
        TextureRef texture(size, TextureFormat::R8G8B8A8, Texture::Usage::Default);

        double decodeMs = 0, readMs = 0, uploadMs = 0;
        int frames = 0;
        float time = 0;
        for (int i = 0; i < 100; i++)
        {
            auto t0 = Clock::now();
            if (!decoder->DecodeNextFrame(time))
                break;
            auto t1 = Clock::now();
            decoder->ReadLastFrame(frame);
            auto t2 = Clock::now();
            texture->SetData(frame);
            auto t3 = Clock::now();

            decodeMs += ms(t0, t1); readMs += ms(t1, t2); uploadMs += ms(t2, t3);
            frames++;
        }

        ASSERT_GT(frames, 0);
        printf("[VideoPerf] %dx%d, %d frames: decode %.2f ms, convert %.2f ms, upload %.2f ms (per frame)\n",
               size.x, size.y, frames, decodeMs/frames, readMs/frames, uploadMs/frames);

        // Whole per-frame video cost must fit far inside a 60 FPS frame
        EXPECT_LT((decodeMs + readMs + uploadMs)/frames, 8.0);
    }

    // Frame loop cost with and without the playing video (End includes batch flush/present)
    {
        Video video(asset);
        video.SetLoop(Loop::Repeat);
        video.Play();
        video.SetSize(Vec2F(1280, 720));
        video.SetPosition(Vec2F());

        double updateMs = 0, drawMs = 0, endMs = 0, endBaseMs = 0;
        const int N = 60;

        for (int i = 0; i < N; i++)
        {
            auto t0 = Clock::now();
            video.Update(1.0f/60.0f);
            auto t1 = Clock::now();

            o2Render.Begin();
            o2Render.Clear(Color4::Black());
            o2Render.SetCamera(Camera());
            auto t2 = Clock::now();
            video.Draw();
            auto t3 = Clock::now();
            o2Render.SetCamera(Camera());
            auto t4 = Clock::now();
            o2Render.End();
            auto t5 = Clock::now();

            updateMs += ms(t0, t1); drawMs += ms(t2, t3); endMs += ms(t4, t5);
        }

        for (int i = 0; i < N; i++)
        {
            auto t0 = Clock::now();
            o2Render.Begin();
            o2Render.Clear(Color4::Black());
            o2Render.End();
            auto t1 = Clock::now();
            endBaseMs += ms(t0, t1);
        }

        printf("[VideoPerf] frame loop: update %.2f ms, draw %.2f ms, frameEnd %.2f ms (baseline frameEnd %.2f ms)\n",
               updateMs/N, drawMs/N, endMs/N, endBaseMs/N);
    }

    std::filesystem::remove(builtPath.Data());
}

// Hardware decode path: H.264 mp4 through AVAssetReader/VideoToolbox, frame follows time
TEST(VideoHardwareDecode, DecodesMp4AndFollowsTime)
{
    auto asset = mmake<VideoAsset>();
    asset->SetPath("o2_video_hw_test.mp4");

    String builtPath = asset->GetBuiltFullPath();
    {
        std::ofstream file(builtPath.Data(), std::ios::binary);
        file.write((const char*)Tests::kMovingVideoMp4, sizeof(Tests::kMovingVideoMp4));
    }

    Video video;
    video.SetVideoAsset(asset);
    video.SetSubControlled(true);

    EXPECT_EQ(video.GetVideoSize(), Vec2I(64, 64));
    EXPECT_NEAR(video.GetDuration(), 1.0f, 0.1f);

    auto redAt = [](const Ref<Bitmap>& b, int x, int y)
    {
        const UInt8* p = b->GetData() + (y*b->GetSize().x + x)*4;
        return p[0] > 150 && p[1] < 120 && p[2] < 120;
    };

    // Frame 0: square on the left
    video.SetTime(0.0f);
    auto f0 = DrawVideoCaptured(video);
    ASSERT_TRUE(f0);
    int cx = f0->GetSize().x/2, cy = f0->GetSize().y/2;
    EXPECT_TRUE(redAt(f0, cx - 80, cy));
    EXPECT_FALSE(redAt(f0, cx - 8, cy));

    // t=0.48: square moved to the center (hardware seek through reader recreation)
    video.SetTime(0.48f);
    auto f1 = DrawVideoCaptured(video);
    ASSERT_TRUE(f1);
    EXPECT_TRUE(redAt(f1, cx - 8, cy));
    EXPECT_FALSE(redAt(f1, cx - 80, cy));

    std::filesystem::remove(builtPath.Data());
}
#endif

// Streaming decodes straight from the file on disk (no in-memory encoded bytes)
TEST(VideoStreaming, DecodesFromDiskFile)
{
    // Asset with only a path, no SetData -> nothing loaded into memory. Streaming reads the
    // file at GetBuiltFullPath(), so drop the clip exactly there.
    auto asset = mmake<VideoAsset>();
    asset->SetPath("o2_video_stream_test.mpg");
    ASSERT_EQ(asset->GetDataSize(), 0u);

    String builtPath = asset->GetBuiltFullPath();
    {
        std::ofstream file(builtPath.Data(), std::ios::binary);
        file.write((const char*)Tests::kMovingVideoMpg, sizeof(Tests::kMovingVideoMpg));
    }

    Video video;
    video.SetStreaming(true);
    video.SetVideoAsset(asset);

    ASSERT_TRUE(video.IsStreaming());
    EXPECT_EQ(video.GetVideoSize(), Vec2I(64, 64)); // headers decoded from the file

    auto captured = DrawVideoCaptured(video);
    ASSERT_TRUE(captured);
    int cx = captured->GetSize().x/2, cy = captured->GetSize().y/2;
    const UInt8* left = captured->GetData() + (cy*captured->GetSize().x + (cx - 80))*4;
    EXPECT_GT(left[0], 150); EXPECT_LT(left[1], 120); // frame 0 red square on the left

    std::filesystem::remove(builtPath.Data());
}
