#pragma once

#include "o2/Scene/Components/MeshComponent.h"
#include "o2Editor/Core/UI/SplineEditor/SplineEditor.h"
#include "o2Editor/PropertiesWindow/ActorsViewer/DefaultActorComponentViewer.h"
#include "o2Editor/SceneWindow/SceneEditorLayer.h"

using namespace o2;

namespace o2
{
	class Spoiler;
}

namespace Editor
{
	// ---------------------
	// Mesh component viewer
	// ---------------------
	class MeshComponentViewer: public TActorComponentViewer<MeshComponent>
	{
	public:
		// Default constructor
		MeshComponentViewer();

		// Sets target actors
		void SetTargetComponents(const Vector<Component*>& components) override;

		// Copy operator
		MeshComponentViewer& operator=(const MeshComponentViewer& other);

		IOBJECT(MeshComponentViewer);

	protected:
		struct SplineWrapper: public SplineEditor::ISplineWrapper
		{
			MeshComponentViewer* viewer;

		public:
			Vec2F GetOrigin() const;

			Vec2F WorldToLocal(const Vec2F& point) const override;
			Vec2F LocalToWorld(const Vec2F& point) const override;

			int GetPointsCount() const override;

			void AddPoint(int idx, const Vec2F& position, const Vec2F& prevSupport, const Vec2F& nextSupport) override;
			void RemovePoint(int idx) override;

			Vec2F GetPointPos(int idx) const override;
			void SetPointPos(int idx, const Vec2F& pos) override;

			Vec2F GetPointPrevSupportPos(int idx) const override;
			void SetPointPrevSupportPos(int idx, const Vec2F& pos) override;

			Vec2F GetPointNextSupportPos(int idx) const override;
			void SetPointNextSupportPos(int idx, const Vec2F& pos) override;

			Vector<Vec2F> GetDrawPoints() const override;

			const ApproximationVec2F* GetPointApproximation(int idx) const override;
			int GetPointApproximationCount(int idx) const override;

			void OnChanged() override;
		};

		struct SplineSceneLayer: public SceneEditorLayer
		{
			MeshComponentViewer* viewer;

		public:
			void DrawOverScene() override;
			void Update(float dt) override;

			int GetOrder() const override;

			bool IsEnabled() const override;

			const String& GetName() const override;
			const String& GetIconName() const override;
		};

		struct SplineTool: public IEditTool
		{
			MeshComponentViewer* viewer;

		public:
			// Returns toggle in menu panel icon name
			String GetPanelIcon() const override;

			// It is called when tool was enabled
			void OnEnabled() override;

			// It is called when tool was disabled
			void OnDisabled() override;
		};

	protected:
		SplineEditor     mSplineEditor; // Animation spline editor
		SplineSceneLayer mSceneLayer;   // Scene layer for drawing spline

		SplineTool mTool;                       // Other handles locking tool
		IEditTool* mPrevSelectedTool = nullptr; // Previous selected tool, for restore

		bool mIsEnabled = false; // Is spline editing enabled

	protected:
		// Enable viewer event function
		void OnEnabled() override;

		// Disable viewer event function
		void OnDisabled() override;
	};
}

CLASS_BASES_META(Editor::MeshComponentViewer)
{
	BASE_CLASS(Editor::TActorComponentViewer<MeshComponent>);
}
END_META;
CLASS_FIELDS_META(Editor::MeshComponentViewer)
{
	FIELD().NAME(mSplineEditor).PROTECTED();
	FIELD().NAME(mSceneLayer).PROTECTED();
	FIELD().NAME(mTool).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mPrevSelectedTool).PROTECTED();
	FIELD().DEFAULT_VALUE(false).NAME(mIsEnabled).PROTECTED();
}
END_META;
CLASS_METHODS_META(Editor::MeshComponentViewer)
{

	PUBLIC_FUNCTION(void, SetTargetComponents, const Vector<Component*>&);
	PROTECTED_FUNCTION(void, OnEnabled);
	PROTECTED_FUNCTION(void, OnDisabled);
}
END_META;
