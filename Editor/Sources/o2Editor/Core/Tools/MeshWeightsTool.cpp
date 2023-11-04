#include "o2Editor/stdafx.h"
#include "MeshWeightsTool.h"

#include "o2/Render/SkinningMesh.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
	MeshWeightsTool::MeshWeightsTool()
	{
		sceneLayer.tool = this;
	}

	void MeshWeightsTool::OnCursorPressed(const Input::Cursor& cursor)
	{
		mPressed = true;
	}

	void MeshWeightsTool::OnCursorReleased(const Input::Cursor& cursor)
	{
		mPressed = false;
	}

	void MeshWeightsTool::OnCursorPressBreak(const Input::Cursor& cursor)
	{

	}

	void MeshWeightsTool::OnCursorStillDown(const Input::Cursor& cursor)
	{
		mCursosPos = cursor.position;
	}

	void MeshWeightsTool::OnCursorMoved(const Input::Cursor& cursor)
	{
		mCursosPos = cursor.position;
	}

	void MeshWeightsTool::SceneLayer::DrawScene()
	{
		auto boneComponent = tool->boneComponent;
		if (auto meshComponent = boneComponent->FindSkinningMesh())
		{
			auto& mesh = meshComponent->GetMesh();
			auto& debugMesh = tool->mWeightsDebugMesh;

			DrawMeshWire(mesh);

			float weightMeshAlpha = 0.8f;
			float weightMeshSaturation = 1.0f;
			float weightMeshLight = 0.5f;
			float zeroWeightColorHue = 0.3f;
			float oneWeightColorHue = 0.6f;

			if (mesh.vertexCount > debugMesh.GetMaxVertexCount() ||
				mesh.polyCount > debugMesh.GetMaxPolyCount())
			{
				debugMesh.Resize(mesh.vertexCount, mesh.polyCount);
			}

			auto zeroWeightColor = Color4::HSL(zeroWeightColorHue, weightMeshSaturation, weightMeshLight, weightMeshAlpha).ABGR();
			for (UInt i = 0; i < mesh.vertexCount; i++)
				debugMesh.vertices[i].Set(mesh.vertices[i], zeroWeightColor, 0, 0);

			for (auto& p : boneComponent->vertexWeights)
			{
				float hue = Math::Lerp(zeroWeightColorHue, oneWeightColorHue, p.second);
				debugMesh.vertices[p.first].color = Color4::HSL(hue, weightMeshSaturation, weightMeshLight, weightMeshAlpha).ABGR();
			}

			memcpy(debugMesh.indexes, mesh.indexes, sizeof(VertexIndex)*mesh.polyCount*3);

			debugMesh.vertexCount = mesh.vertexCount;
			debugMesh.polyCount = mesh.polyCount;

			debugMesh.Draw();
		}

		Vec2F localCursorPos = o2EditorSceneScreen.ScreenToLocalPoint(tool->mCursosPos);
		o2Render.DrawAACircle(localCursorPos, tool->mBrushReadius, Color4::White(), 150, 2.0f);
	}

	void MeshWeightsTool::SceneLayer::DrawMeshWire(auto& mesh)
	{
		Color4 wireColor = Color4::White();
		for (UInt i = 0; i < mesh.polyCount; i++)
		{
			Vec2F p0 = mesh.vertices[mesh.indexes[i*3]];
			Vec2F p1 = mesh.vertices[mesh.indexes[i*3 + 1]];
			Vec2F p2 = mesh.vertices[mesh.indexes[i*3 + 2]];

			Vertex vertices[] =
			{
				Vertex(p0, wireColor.ABGR(), 0, 0),
				Vertex(p1, wireColor.ABGR(), 0, 0),
				Vertex(p2, wireColor.ABGR(), 0, 0),
				Vertex(p0, wireColor.ABGR(), 0, 0)
			};

			o2Render.DrawPolyLine(vertices, 4);
		}
	}

	void MeshWeightsTool::SceneLayer::Update(float dt)
	{
		if (tool->mPressed)
			UpdateBrush(dt);
	}

	void MeshWeightsTool::SceneLayer::UpdateBrush(float dt)
	{
		auto meshComponent = tool->boneComponent->FindSkinningMesh();
		if (!meshComponent)
			return;

		auto& vertices = meshComponent->GetMesh().vertices;
		auto& vertexWeights = tool->boneComponent->vertexWeights;
		int vertexCount = meshComponent->GetMesh().vertexCount;

		Vec2F localCursorPos = o2EditorSceneScreen.ScreenToLocalPoint(tool->mCursosPos);

		for (int i = 0; i < vertexCount; i++)
		{
			auto& v = vertices[i];
			bool isInsideBrush = (Vec2F(v) - localCursorPos).Length() < tool->mBrushReadius;
			if (isInsideBrush)
			{
				auto w = vertexWeights.Find([=](const Pair<int, float>& x) { return x.first == i; });
				if (!w)
				{
					vertexWeights.Add({ i, 0.0f });
					w = &vertexWeights.Last();
				}

				float brushSign = o2Input.IsKeyDown(VK_CONTROL) ? -1.0f : 1.0f;

				w->second = Math::Clamp01(w->second + tool->mBrushStrength*brushSign*dt);
			}
		}

		vertexWeights.RemoveAll([](const Pair<int, float>& x) { return x.second <= 0.0f; });
	}

	int MeshWeightsTool::SceneLayer::GetOrder() const
	{
		return 0;
	}

	bool MeshWeightsTool::SceneLayer::IsEnabled() const
	{
		return tool->isEnabled;
	}

	const String& MeshWeightsTool::SceneLayer::GetName() const
	{
		static String res("Weights tool");
		return res;
	}

	const String& MeshWeightsTool::SceneLayer::GetIconName() const
	{
		return String::empty;
	}

	String MeshWeightsTool::MeshWeightsTool::GetPanelIcon() const
	{
		return "ui/UI4_weight_tool.png";
	}

	void MeshWeightsTool::MeshWeightsTool::OnEnabled()
	{
		o2EditorSceneScreen.AddEditorLayer(&sceneLayer);
		isEnabled = true;
	}

	void MeshWeightsTool::MeshWeightsTool::OnDisabled()
	{
		o2EditorSceneScreen.RemoveEditorLayer(&sceneLayer);
		isEnabled = false;
	}
}
// --- META ---

DECLARE_CLASS(Editor::MeshWeightsTool);
// --- END META ---
