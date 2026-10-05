#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Events/EventSystem.h"
#include "EditorVectorUIFixture.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

// Cost of the editor frame drawn with vector graphics against the same frame drawn with raster sprites

namespace
{
    const int warmUpFrames = 20;
    const int measuredFrames = 240;
    const int changedFrames = 40;
    const int rounds = 2;

    // Vector frame must not cost more than the raster one. The factor and the slack cover the noise of a developer
    // machine: frames of the smallest scene take microseconds
    const double maxCostFactor = 1.15;
    const double costSlack = 0.01;

    using Clock = std::chrono::steady_clock;

    double Milliseconds(const Clock::time_point& from)
    {
        return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
    }

    double Median(Vector<double> values)
    {
        std::sort(values.begin(), values.end());
        return values.IsEmpty() ? 0.0 : values[values.Count()/2];
    }

    int GetEnvInt(const char* name, int defaultValue)
    {
        const char* value = std::getenv(name);
        return value ? atoi(value) : defaultValue;
    }

    // Medians of the frames of a scene, milliseconds, and the counters of its last frame
    struct SceneCost
    {
        double draw = 0;        // Draw traversal with batching on the main thread, nothing changes between frames
        double present = 0;     // Render::End of the same frames: submit to GPU by the render thread and present
        double update = 0;      // Editor update
        double updatedDraw = 0; // Draw traversal that follows an update
        double relaidDraw = 0;  // Draw traversal after all transforms were updated in place
        double movedDraw = 0;   // Draw traversal after the whole editor was moved: no batch is left from the last frame

        int                     drawCalls = 0;
        int                     triangles = 0;
        Render::BatchStatistics batches;

        // Vector sprites per frame
        UInt64 vectorDraws = 0;
        UInt64 vectorCulledDraws = 0;
        UInt64 vectorTriangles = 0;
        UInt64 vectorVertices = 0;
        UInt64 rebuiltVertices = 0;        // In the frames without changes
        UInt64 updatedRebuiltVertices = 0; // In the frames with the editor updated
        UInt64 relaidRebuiltVertices = 0;  // In the frames with all transforms updated in place
        UInt64 movedChangedVertices = 0;   // Rebuilt or shifted in the frames with the whole editor moved

        UInt tessellations = 0; // Meshes built by all measured frames
    };

    struct PassCost
    {
        double styleBuild = 0;       // Rebuilding the editor style
        double shellBuild = 0;       // Building the editor windows
        double firstFrame = 0;       // First frame: meshes of everything visible are built
        UInt   tessellations = 0;    // Meshes tessellated by the pass
        double tessellationTime = 0; // Time of their tessellation

        Vector<SceneCost> scenes;
    };

    const Vector<String> sceneNames = { "editor_frame", "editor_frame_tabs", "editor_frame_selection" };

    // As EditorApplication updates the UI: without the forced transforms update of EditorShell::Update
    void UpdateEditor(EditorShell& shell, float dt)
    {
        PushEditorScopeOnStack scope;

        shell.windows->Update(dt);

        auto root = EditorUIRoot.GetRootWidget();
        root->Update(dt);
        root->UpdateChildren(dt);
    }

    void DrawFrame(EditorShell& shell, double* drawTime = nullptr, double* presentTime = nullptr)
    {
        PushEditorScopeOnStack scope;

        // As the application frame does, otherwise the listeners drawn by every frame pile up
        o2Events.PostUpdate();

        o2Render.Begin();
        o2Render.SetCamera(Camera());

        auto drawStart = Clock::now();
        shell.Draw();
        o2UI.DrawCurrentLayerTopWidgets();

        if (drawTime)
            *drawTime = Milliseconds(drawStart);

        auto presentStart = Clock::now();
        o2Render.End();

        if (presentTime)
            *presentTime = Milliseconds(presentStart);
    }

    // Draws frames with the change made before each one, returns the median of the draw traversal time
    double MeasureDraw(EditorShell& shell, int frames, const Function<void(int)>& change, double* update = nullptr,
                       double* present = nullptr)
    {
        Vector<double> draws, updates, presents;
        for (int i = 0; i < frames; i++)
        {
            auto start = Clock::now();
            if (change)
                change(i);

            updates.Add(Milliseconds(start));

            double draw = 0, presentTime = 0;
            DrawFrame(shell, &draw, &presentTime);
            draws.Add(draw);
            presents.Add(presentTime);
        }

        if (update)
            *update = Median(updates);

        if (present)
            *present = Median(presents);

        return Median(draws);
    }

    SceneCost MeasureScene(EditorShell& shell)
    {
        SceneCost res;
        int frames = GetEnvInt("O2_VECTOR_PERF_FRAMES", measuredFrames);

        ParkCursor();
        for (int i = 0; i < settleSteps; i++)
            shell.Update(settleStep);

        if (rasterPass)
        {
            PushEditorScopeOnStack scope;
            RasterizeWidget(EditorUIRoot.GetRootWidget());
        }

        for (int i = 0; i < warmUpFrames; i++)
            DrawFrame(shell);

        UInt tessellations = VectorImageAsset::GetTessellationsCount();

        VectorSprite::Statistics before = VectorSprite::GetStatistics();
        res.draw = MeasureDraw(shell, frames, {}, nullptr, &res.present);
        VectorSprite::Statistics after = VectorSprite::GetStatistics();

        res.drawCalls = o2Render.GetDrawCallsCount();
        res.triangles = o2Render.GetDrawnPrimitives();
        res.batches = o2Render.GetBatchStatistics();
        res.vectorDraws = (after.draws - before.draws)/frames;
        res.vectorCulledDraws = (after.culledDraws - before.culledDraws)/frames;
        res.vectorTriangles = (after.drawnTriangles - before.drawnTriangles)/frames;
        res.vectorVertices = (after.drawnVertices - before.drawnVertices)/frames;
        res.rebuiltVertices = (after.rebuiltVertices - before.rebuiltVertices)/frames;

        before = after;
        res.updatedDraw = MeasureDraw(shell, frames, [&](int) { UpdateEditor(shell, 1.0f/60.0f); }, &res.update);
        after = VectorSprite::GetStatistics();
        res.updatedRebuiltVertices = (after.rebuiltVertices - before.rebuiltVertices)/frames;

        // A layout update sets every transform again, as the forced update of EditorShell does
        before = after;
        res.relaidDraw = MeasureDraw(shell, changedFrames, [&](int) { shell.Update(1.0f/60.0f); });
        after = VectorSprite::GetStatistics();
        res.relaidRebuiltVertices = (after.rebuiltVertices - before.rebuiltVertices)/changedFrames;

        // The worst case: every drawable moves every frame
        before = after;
        res.movedDraw = MeasureDraw(shell, changedFrames, [&](int frame)
        {
            PushEditorScopeOnStack scope;

            auto root = EditorUIRoot.GetRootWidget();
            *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize(),
                                                Vec2F((float)(frame%2), 0.0f));
            shell.Update(1.0f/60.0f);
        });

        after = VectorSprite::GetStatistics();
        res.movedChangedVertices = (after.rebuiltVertices + after.shiftedVertices - before.rebuiltVertices -
                                    before.shiftedVertices)/changedFrames;

        FitRootToFrame();
        shell.Update(1.0f/60.0f);

        return res;
    }

    PassCost MeasurePass(bool raster, Ref<Actor> selected)
    {
        PassCost res;

        UInt tessellations = VectorImageAsset::GetTessellationsCount();
        double tessellationTime = VectorImageAsset::GetTessellationTime();

        auto start = Clock::now();
        BuildStyle(raster);
        res.styleBuild = Milliseconds(start);

        Vector<WindowsLayout> layouts = { MakeLayout("scene window", "assets window", "properties window"),
                                          MakeLayout("pipeline window", "animation window", "game window"),
                                          MakeLayout("animation state graph window", "log window", "properties window") };

        for (int i = 0; i < layouts.Count(); i++)
        {
            if (i > 0)
                BuildStyle(raster);

            start = Clock::now();
            EditorShell shell(layouts[i]);
            double shellBuild = Milliseconds(start);

            if (i == 2)
            {
                o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ selected->GetID() });
                o2EditorTree.GetSceneTree()->ExpandAll();
            }

            ParkCursor();
            shell.Update(settleStep);

            start = Clock::now();
            DrawFrame(shell);

            if (i == 0)
            {
                res.shellBuild = shellBuild;
                res.firstFrame = Milliseconds(start);
            }

            res.scenes.Add(MeasureScene(shell));
        }

        res.tessellations = VectorImageAsset::GetTessellationsCount() - tessellations;
        res.tessellationTime = (VectorImageAsset::GetTessellationTime() - tessellationTime)*1000.0;

        return res;
    }

    void PrintPass(const char* name, const PassCost& pass)
    {
        printf("  %s: style build %.1f ms, editor build %.1f ms, first frame %.1f ms, tessellations %u in %.1f ms\n", name,
               pass.styleBuild, pass.shellBuild, pass.firstFrame, pass.tessellations, pass.tessellationTime);

        for (int i = 0; i < pass.scenes.Count(); i++)
        {
            const SceneCost& scene = pass.scenes[i];
            printf("    %-22s draw %.3f ms, present %.3f ms, update %.3f ms, draw after: update %.3f, relayout %.3f, "
                   "move %.3f ms\n", sceneNames[i].Data(), scene.draw, scene.present, scene.update, scene.updatedDraw,
                   scene.relaidDraw, scene.movedDraw);

            printf("    %-22s triangles %i, vertices %u (retained %u), draw buffers %u, draw calls %i; batches closed by: "
                   "texture %u, material %u, primitive %u, layout %u, capacity %u, geometry %u\n", "", scene.triangles,
                   scene.batches.vertices, scene.batches.retainedVertices, scene.batches.drawBuffers, scene.drawCalls,
                   scene.batches.textureBreaks, scene.batches.materialBreaks, scene.batches.primitiveBreaks,
                   scene.batches.vertexTypeBreaks, scene.batches.capacityBreaks, scene.batches.geometryBreaks);

            printf("    %-22s vector sprites: draws %llu, culled %llu, triangles %llu, vertices %llu; rebuilt vertices: "
                   "still %llu, updated %llu, relaid %llu; rebuilt or shifted when moved %llu\n", "",
                   (unsigned long long)scene.vectorDraws, (unsigned long long)scene.vectorCulledDraws,
                   (unsigned long long)scene.vectorTriangles, (unsigned long long)scene.vectorVertices,
                   (unsigned long long)scene.rebuiltVertices, (unsigned long long)scene.updatedRebuiltVertices,
                   (unsigned long long)scene.relaidRebuiltVertices, (unsigned long long)scene.movedChangedVertices);
        }

        fflush(stdout);
    }

    using EditorVectorUIPerf = EditorVectorUIFixture;
}

// O2_VECTOR_PERF_FRAMES and O2_VECTOR_PERF_ROUNDS change the count of measured frames and of passes, for profiling
TEST_F(EditorVectorUIPerf, VectorFrameCostsNotMoreThanRaster)
{
    TinyScene scene;

    // Passes alternate and the best round of each is taken: a busy machine slows a round down, never speeds it up
    Vector<PassCost> raster, vector;
    for (int i = 0; i < GetEnvInt("O2_VECTOR_PERF_ROUNDS", rounds); i++)
    {
        raster.Add(MeasurePass(true, scene.root));
        PrintPass("raster", raster.Last());

        vector.Add(MeasurePass(false, scene.root));
        PrintPass("vector", vector.Last());
    }

    auto best = [&](const Vector<PassCost>& passes, int sceneIdx, const Function<double(const SceneCost&)>& cost)
    {
        double res = cost(passes[0].scenes[sceneIdx]);
        for (auto& pass : passes)
            res = Math::Min(res, cost(pass.scenes[sceneIdx]));

        return res;
    };

    auto draw = [](const SceneCost& scene) { return scene.draw; };
    auto updateAndDraw = [](const SceneCost& scene) { return scene.update + scene.updatedDraw; };
    auto relaidDraw = [](const SceneCost& scene) { return scene.relaidDraw; };

    for (int i = 0; i < sceneNames.Count(); i++)
    {
        const char* name = sceneNames[i].Data();

        double rasterDraw = best(raster, i, draw), vectorDraw = best(vector, i, draw);
        double rasterUpdated = best(raster, i, updateAndDraw), vectorUpdated = best(vector, i, updateAndDraw);
        double rasterRelaid = best(raster, i, relaidDraw), vectorRelaid = best(vector, i, relaidDraw);

        printf("  %-22s vector to raster: draw %.3f / %.3f ms = %.2f, update and draw %.3f / %.3f ms = %.2f, "
               "draw after relayout %.3f / %.3f ms = %.2f\n", name, vectorDraw, rasterDraw, vectorDraw/rasterDraw,
               vectorUpdated, rasterUpdated, vectorUpdated/rasterUpdated, vectorRelaid, rasterRelaid,
               vectorRelaid/rasterRelaid);

        const SceneCost& vectorScene = vector.Last().scenes[i];
        const SceneCost& rasterScene = raster.Last().scenes[i];

        EXPECT_LE(rasterScene.vectorDraws*20, vectorScene.vectorDraws) << name << ": the raster pass has drawn vector sprites";
        EXPECT_GT(vectorScene.vectorDraws, 50u) << name << ": the vector pass has drawn too few";
        EXPECT_GT(vectorScene.vectorTriangles, 5000u) << name << ": the vector pass has drawn too few";

        EXPECT_EQ(vectorScene.tessellations, 0u) << name << ": meshes are built after the warm up";
        EXPECT_EQ(vectorScene.rebuiltVertices, 0u) << name << ": a still frame rebuilds vertices";
        EXPECT_EQ(vectorScene.relaidRebuiltVertices, 0u) << name << ": a layout update in place rebuilds vertices";

        if (o2Render.IsMultithreadedRenderEnabled())
        {
            EXPECT_EQ(vectorScene.batches.retainedVertices, (UInt)vectorScene.vectorVertices)
                << name << ": a still frame copies vertices of vector sprites";
        }

        EXPECT_LE(vectorDraw, rasterDraw*maxCostFactor + costSlack) << name << ": draw";
        EXPECT_LE(vectorUpdated, rasterUpdated*maxCostFactor + costSlack) << name << ": update and draw";
        EXPECT_LE(vectorRelaid, rasterRelaid*maxCostFactor + costSlack) << name << ": draw after relayout";
    }

    fflush(stdout);
}
