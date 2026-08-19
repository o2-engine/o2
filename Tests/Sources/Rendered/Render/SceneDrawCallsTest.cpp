#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Editor/EditorScope.h"

using namespace o2;

namespace
{
    Ref<Mesh> MakeQuad()
    {
        auto mesh = mmake<Mesh>();
        mesh->Resize(4, 2);

        auto* vertices = mesh->GetVertices<Vertex>();
        for (int i = 0; i < 4; i++)
        {
            vertices[i].x = (float)(i%2)*10.0f;
            vertices[i].y = (float)(i/2)*10.0f;
            vertices[i].z = 0.0f;
            vertices[i].color = 0xffffffff;
            vertices[i].tu = 0.0f;
            vertices[i].tv = 0.0f;
        }

        auto* indexes = mesh->GetIndexes();
        indexes[0] = 0; indexes[1] = 1; indexes[2] = 2;
        indexes[3] = 0; indexes[4] = 2; indexes[5] = 3;

        mesh->vertexCount = 4;
        mesh->polyCount = 2;

        return mesh;
    }
}

// The editor draws its own UI over the same render, and mixing it into the numbers the profiler panel
// shows makes them useless. Everything the editor draws happens inside an editor scope, and the scene
// counters ignore it
TEST(SceneDrawCalls, CountOnlyWhatIsDrawnOutsideTheEditorScope)
{
    ASSERT_EQ(EditorScope::GetDepth(), 0);

    auto mesh = MakeQuad();

    o2Render.Begin();

    // SetCamera flushes the batch, so every draw below ends up as its own draw call
    {
        PushEditorScopeOnStack editorScope;

        o2Render.DrawMesh(mesh.Get());
        o2Render.SetCamera(Camera::Default());
    }

    const int totalAfterEditor = o2Render.GetDrawCallsCount();
    const int sceneAfterEditor = o2Render.GetSceneDrawCallsCount();
    const int scenePrimitivesAfterEditor = o2Render.GetSceneDrawnPrimitives();

    EXPECT_GT(totalAfterEditor, 0) << "the editor's own drawing still counts into the total";
    EXPECT_EQ(sceneAfterEditor, 0);
    EXPECT_EQ(scenePrimitivesAfterEditor, 0);

    o2Render.DrawMesh(mesh.Get());
    o2Render.SetCamera(Camera::Default());

    EXPECT_GT(o2Render.GetDrawCallsCount(), totalAfterEditor);
    EXPECT_GT(o2Render.GetSceneDrawCallsCount(), sceneAfterEditor);
    EXPECT_GE(o2Render.GetSceneDrawnPrimitives(), 2);

    o2Render.End();
}

// Outside the editor the two are the same thing
TEST(SceneDrawCalls, MatchTheTotalWhenNothingIsInAnEditorScope)
{
    ASSERT_EQ(EditorScope::GetDepth(), 0);

    auto mesh = MakeQuad();

    o2Render.Begin();

    for (int i = 0; i < 3; i++)
    {
        o2Render.DrawMesh(mesh.Get());
        o2Render.SetCamera(Camera::Default());
    }

    EXPECT_EQ(o2Render.GetSceneDrawCallsCount(), o2Render.GetDrawCallsCount());
    EXPECT_EQ(o2Render.GetSceneDrawnPrimitives(), o2Render.GetDrawnPrimitives());

    o2Render.End();
}
