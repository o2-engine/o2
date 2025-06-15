#pragma once

#include "o2/Scene/Components/MeshComponent.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Tools/CustomFrameTool.h"
#include "o2Editor/Tools/MeshTopologyTool.h"
#include "o2Editor/Tools/SplineTool.h"
#include "o2Editor/UI/SplineEditor/SplineEditor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

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
    class MeshComponentViewer: public TObjectPropertiesViewer<MeshComponent>
    {
    public:
        // Default constructor
        MeshComponentViewer();

        // Destructor
        ~MeshComponentViewer();

        // Copy operator
        MeshComponentViewer& operator=(const MeshComponentViewer& other);

        IOBJECT(MeshComponentViewer);

    protected:
        // ------------------------------------
        // Scene layer for draw texture overlay
        // ------------------------------------
        struct SceneLayer : public SceneEditorLayer
        {
            WeakRef<MeshComponentViewer> viewer;        // Reference to viewer
            Sprite                       textureSprite; // Texture sprite

        public:
            // Draws editor over scene
            void DrawOverScene() override;

            // Updates editor
            void Update(float dt) override;

            // Returns order of layer
            int GetOrder() const override;

            // Returns true if layer is enabled
            bool IsEnabled() const override;

            // Returns name of layer
            const String& GetName() const override;

            // Returns icon name of layer
            const String& GetIconName() const override;

            // Draws mesh wire
            void DrawMeshWire();
        };

    protected:
        Ref<SplineTool>       mSplineTool;        // Spline tool
        Ref<CustomFrameTool>  mFrameTool;         // Mapping frame tool
        Ref<MeshTopologyTool> mTopologyTool;      // Mesh topology tool
        Ref<SceneLayer>       mFrameTetxureLayer; // Frame texture drawing layer

        WeakRef<IEditTool> mPrevSelectedTool; // Previous selected tool, for restore

        Ref<Button> mFitAndCenterButton; // Fit and centerize button

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Called when button pressed
        void FitAndCenterize();
    };
}
// --- META ---

CLASS_BASES_META(Editor::MeshComponentViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<MeshComponent>);
}
END_META;
CLASS_FIELDS_META(Editor::MeshComponentViewer)
{
    FIELD().PROTECTED().NAME(mSplineTool);
    FIELD().PROTECTED().NAME(mFrameTool);
    FIELD().PROTECTED().NAME(mTopologyTool);
    FIELD().PROTECTED().NAME(mFrameTetxureLayer);
    FIELD().PROTECTED().NAME(mPrevSelectedTool);
    FIELD().PROTECTED().NAME(mFitAndCenterButton);
}
END_META;
CLASS_METHODS_META(Editor::MeshComponentViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, FitAndCenterize);
}
END_META;
// --- END META ---
