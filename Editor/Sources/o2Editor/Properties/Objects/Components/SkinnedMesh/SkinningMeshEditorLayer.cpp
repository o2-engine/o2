#include "o2Editor/stdafx.h"
#include "SkinningMeshEditorLayer.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    void SkinningMeshEditorLayer::DrawOverScene()
    {
        if (mesh)
        {
            textureSprite.SetImageAsset(mesh->GetImage());
            textureSprite.SetBasis(Basis(mesh->GetMappingFrame())
                * Basis::Translated(mesh->GetActor()->transform->GetWorldPosition())
                * o2EditorSceneScreen.GetLocalToScreenTransform());
            textureSprite.SetTransparency(0.5f);
            textureSprite.Draw();

            if (drawMeshWire)
                DrawMeshWire();
        }
    }

    void SkinningMeshEditorLayer::Update(float dt)
    {}

    int SkinningMeshEditorLayer::GetOrder() const
    {
        return 0;
    }

    bool SkinningMeshEditorLayer::IsEnabled() const
    {
        return true;
    }

    const String& SkinningMeshEditorLayer::GetName() const
    {
        static String str("mesh texture overlay");
        return str;
    }

    const String& SkinningMeshEditorLayer::GetIconName() const
    {
        return String::empty;
    }

    void SkinningMeshEditorLayer::DrawMeshWire()
    {
        if (mesh)
        {
            auto& renderMesh = mesh->GetMesh();
            auto* verts = renderMesh.vertices;

            Color4 wireColor(0, 0, 0, 100);
            Vector<Vertex> verticies;
            for (UInt i = 0; i < renderMesh.polyCount; i++)
            {
                auto v = o2EditorSceneScreen.LocalToScreenPoint(verts[renderMesh.indexes[i * 3]]);
                auto v1 = o2EditorSceneScreen.LocalToScreenPoint(verts[renderMesh.indexes[i * 3 + 1]]);
                auto v2 = o2EditorSceneScreen.LocalToScreenPoint(verts[renderMesh.indexes[i * 3 + 2]]);

                verticies.Clear();
                verticies.Add(Vertex(v.x, v.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
                verticies.Add(Vertex(v1.x, v1.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
                verticies.Add(Vertex(v2.x, v2.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
                verticies.Add(Vertex(v.x, v.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
                o2Render.DrawPolyLine(verticies.Data(), verticies.Count());
            }
        }
    }

}
