#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Events/EventSystem.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Math/Spline.h"
#include "o2Editor/UI/SplineEditor/SplineEditor.h"

using namespace o2;
using namespace Editor;

// Drives the corner rounding handle of the spline editor through the real cursor
// pipeline: press on the handle, drag along the corner bisector, release. Uses an
// identity wrapper, so spline local space == input screen space.
namespace
{
    struct IdentitySplineWrapper: SplineEditor::ISplineWrapper
    {
        Ref<Spline> spline;

        int GetPointsCount() const override { return spline->GetKeys().Count(); }
        bool IsClosed() const override { return spline->IsClosed(); }

        void AddPoint(int idx, const Vec2F& position, const Vec2F& prevSupport, const Vec2F& nextSupport) override
        {
            spline->InsertKey(idx, position, 0.0f, prevSupport, nextSupport);
        }

        void RemovePoint(int idx) override { spline->RemoveKey(idx); }

        Vec2F GetPointPos(int idx) const override { return spline->GetKey(idx).value; }

        void SetPointPos(int idx, const Vec2F& pos) override
        {
            auto key = spline->GetKey(idx);
            key.value = pos;
            spline->SetKey(key, idx);
        }

        void SetPointRangeValue(int idx, float value) override
        {
            auto key = spline->GetKey(idx);
            key.valueRange = value;
            spline->SetKey(key, idx);
        }

        float GetPointRangeValue(int idx) const override { return spline->GetKey(idx).valueRange; }

        Vec2F GetPointPrevSupportPos(int idx) const override
        {
            auto key = spline->GetKey(idx);
            return key.prevSupport + key.value;
        }

        void SetPointPrevSupportPos(int idx, const Vec2F& pos) override
        {
            auto key = spline->GetKey(idx);
            key.prevSupport = pos - key.value;
            spline->SetKey(key, idx);
        }

        Vec2F GetPointNextSupportPos(int idx) const override
        {
            auto key = spline->GetKey(idx);
            return key.nextSupport + key.value;
        }

        void SetPointNextSupportPos(int idx, const Vec2F& pos) override
        {
            auto key = spline->GetKey(idx);
            key.nextSupport = pos - key.value;
            spline->SetKey(key, idx);
        }

        const ApproximationVec2F* GetPointApproximationLeft(int idx) const override
        {
            return spline->GetKeys()[idx].GetApproximatedPointsLeft();
        }

        const ApproximationVec2F* GetPointApproximationRight(int idx) const override
        {
            return spline->GetKeys()[idx].GetApproximatedPointsRight();
        }

        int GetPointApproximationCount(int idx) const override
        {
            return spline->GetKeys()[idx].GetApproximatedPointsCount();
        }
    };

    struct SplineEditorProbe: SplineEditor
    {
        using SplineEditor::mSplineHandles;
    };

    struct RoundingFixture
    {
        Ref<Spline>              spline;
        Ref<IdentitySplineWrapper> wrapper;
        Ref<SplineEditorProbe>   editor;

        int beginEditCount = 0;
        int endEditCount = 0;

        // Square 240x240 with corners at (0,0), (240,0), (240,240), (0,240)
        RoundingFixture()
        {
            spline = mmake<Spline>();
            spline->BeginKeysBatchChange();
            spline->AppendKey(Vec2F(0.0f, 0.0f), 0.0f, Vec2F(), Vec2F());
            spline->AppendKey(Vec2F(240.0f, 0.0f), 0.0f, Vec2F(), Vec2F());
            spline->AppendKey(Vec2F(240.0f, 240.0f), 0.0f, Vec2F(), Vec2F());
            spline->AppendKey(Vec2F(0.0f, 240.0f), 0.0f, Vec2F(), Vec2F());
            spline->CompleteKeysBatchingChange();
            spline->SetClosed(true);

            wrapper = mmake<IdentitySplineWrapper>();
            wrapper->spline = spline;

            editor = mmake<SplineEditorProbe>();
            editor->SetSpline(wrapper);
            editor->onBeginEdit = [&]() { beginEditCount++; };
            editor->onEndEdit = [&]() { endEditCount++; };
        }

        ~RoundingFixture()
        {
            o2Input.OnCursorReleased(0);
            PumpFrame();
            editor->Reset();
        }

        Ref<Bitmap> PumpFrame(bool capture = false)
        {
            Ref<Bitmap> captured;
            if (capture)
                o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

            o2Input.PreUpdate();

            o2Render.Begin();
            o2Render.Clear(Color4(235, 235, 235));
            editor->Draw();
            o2Render.End();

            o2Events.Update();
            o2Events.PostUpdate();
            o2Input.Update(0.016f);

            // Advance application time: the double-click window check uses it
            o2Time.Update(0.016f);

            return captured;
        }

        void SelectPoint(int idx)
        {
            editor->mSplineHandles[idx]->position->SetSelected(true);
            PumpFrame();
        }

        void Drag(const Vec2F& from, const Vec2F& to, int steps = 4)
        {
            o2Input.OnCursorPressed(from);
            PumpFrame();

            for (int i = 1; i <= steps; i++)
            {
                Vec2F p = from + (to - from)*((float)i/(float)steps);
                o2Input.OnCursorMoved(p);
                PumpFrame();
            }

            o2Input.OnCursorReleased(0);
            PumpFrame();

            // Let the double-click window expire so the next gesture is a fresh click
            for (int i = 0; i < 40; i++)
                PumpFrame();
        }
    };

    const Vec2F kCorner(240.0f, 0.0f);
    const Vec2F kBisector(-0.70711f, 0.70711f); // into the square from corner 1
    const float kBaseOffset = 20.0f;            // rounding handle offset at zero rounding, screen px
}

TEST(SplineEditorRoundingUI, DragRoundsCornerAndDragBackRestoresSharp)
{
    RoundingFixture fx;

    fx.SelectPoint(1);

    // Rounding handle sits on the bisector at the base offset from the sharp corner
    Vec2F handlePos = fx.editor->mSplineHandles[1]->rounding->GetPosition();
    EXPECT_NEAR(handlePos.x, kCorner.x + kBisector.x*kBaseOffset, 1.0f);
    EXPECT_NEAR(handlePos.y, kCorner.y + kBisector.y*kBaseOffset, 1.0f);

    // Drag the handle 30 px deeper along the bisector -> rounding value 30
    fx.Drag(handlePos, kCorner + kBisector*(kBaseOffset + 30.0f));

    auto key = fx.spline->GetKey(1);
    Vec2F expectedPos = kCorner + kBisector*30.0f;
    EXPECT_NEAR(key.value.x, expectedPos.x, 1.0f);
    EXPECT_NEAR(key.value.y, expectedPos.y, 1.0f);

    // Supports land on the square edges and are symmetric
    Vec2F prevAbs = key.value + key.prevSupport;
    Vec2F nextAbs = key.value + key.nextSupport;
    EXPECT_NEAR(prevAbs.y, 0.0f, 1.0f) << "prev support on the bottom edge";
    EXPECT_NEAR(nextAbs.x, 240.0f, 1.0f) << "next support on the right edge";
    EXPECT_NEAR(key.prevSupport.Length(), 30.0f, 1.5f);
    EXPECT_NEAR(key.nextSupport.Length(), 30.0f, 1.5f);

    EXPECT_GE(fx.beginEditCount, 1) << "rounding drag must open an undo edit";
    EXPECT_GE(fx.endEditCount, 1);

    // Now drag the handle back to the base offset -> sharp corner restored
    fx.SelectPoint(1);
    Vec2F roundedHandlePos = fx.editor->mSplineHandles[1]->rounding->GetPosition();
    EXPECT_NEAR(roundedHandlePos.x, kCorner.x + kBisector.x*(kBaseOffset + 30.0f), 1.5f);
    EXPECT_NEAR(roundedHandlePos.y, kCorner.y + kBisector.y*(kBaseOffset + 30.0f), 1.5f);

    fx.Drag(roundedHandlePos, kCorner + kBisector*kBaseOffset);

    key = fx.spline->GetKey(1);
    EXPECT_NEAR(key.value.x, 240.0f, 1.0f);
    EXPECT_NEAR(key.value.y, 0.0f, 1.0f);
    EXPECT_LT(key.prevSupport.Length(), 1.0f);
    EXPECT_LT(key.nextSupport.Length(), 1.0f);

    EXPECT_EQ(fx.spline->GetKeys().Count(), 4) << "gestures must not create points";
}

// Renders the spline editor with rounding handles into screenshots (report images)
TEST(SplineEditorRoundingUI, HandleScreenshotsSaved)
{
    RoundingFixture fx;

    auto renderShot = [&](const String& path) {
        // First captured frame after switching to a capture target needs a repeat
        fx.PumpFrame(true);
        Ref<Bitmap> captured = fx.PumpFrame(true);

        ASSERT_TRUE(captured);
        o2FileSystem.FolderCreate("TestScreenshots", true);
        EXPECT_TRUE(captured->Save("TestScreenshots/" + path, Bitmap::ImageType::Png));
    };

    fx.SelectPoint(1);
    renderShot("spline_rounding_sharp.png");

    Vec2F handlePos = fx.editor->mSplineHandles[1]->rounding->GetPosition();
    fx.Drag(handlePos, kCorner + kBisector*(kBaseOffset + 45.0f));

    fx.SelectPoint(1);
    renderShot("spline_rounding_rounded.png");
}

TEST(SplineEditorRoundingUI, RoundingIsClampedByAdjacentEdges)
{
    RoundingFixture fx;

    fx.SelectPoint(2);

    // Corner 2 at (240, 240), bisector towards the square center
    Vec2F corner(240.0f, 240.0f);
    Vec2F bisector(-0.70711f, -0.70711f);
    Vec2F handlePos = fx.editor->mSplineHandles[2]->rounding->GetPosition();

    // Drag far beyond the limit: value must clamp so arc tangents stay inside half-edges
    fx.Drag(handlePos, corner + bisector*(kBaseOffset + 200.0f), 6);

    auto key = fx.spline->GetKey(2);
    float value = (key.value - corner).Length();
    float maxValue = 0.5f*240.0f*(1.0f - Math::Sin(Math::PI()*0.25f))/Math::Cos(Math::PI()*0.25f);

    EXPECT_NEAR(value, maxValue, 1.5f);
    EXPECT_LT(key.prevSupport.Length(), 120.0f);
}
