#pragma once
#include "IEditorTool.h"

#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

namespace Editor
{
    // -------------------------------------------------
    // Skeleton tool. Used for editing bones of skeleton
    // -------------------------------------------------
    struct SkeletonTool: public IEditTool
    {
        // ---------------------------------------------------------------------
        // Scene layer. Used to draw and update stuff of bones handles and other
        // ---------------------------------------------------------------------
        struct SceneLayer: public SceneEditorLayer
        {
            WeakRef<SkeletonTool> tool; // Skeleton tool

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
        };

        // ---------------------------------------------------------
        // Bone handle container. Keeps handle and reference to bone
        // ---------------------------------------------------------
        struct BoneHandle: public RefCounterable
        {
            Ref<SceneDragHandle>           handle;        // Bone handle in scene
            Ref<SkinningMeshBoneComponent> boneComponent; // Bone component

            Basis pressedTransform; // Stored bone transform when pressed

        public:
            // Default constructor, creates handle
            BoneHandle();

            // Destructor, destroys handle
            ~BoneHandle();

            // Updates handle position by bone transform
            void UpdatePosition();

        private:
            // Called when handle is moved, changes bone transformation
            void OnHandleChangedPos(const Vec2F& pos);
        };

        // ---------------------------------------------------------------------------------
        // Skeleton edit instance. Contains handles and reference to skinning mesh component
        // ---------------------------------------------------------------------------------
        struct SkeletonInstance: public RefCounterable
        {
            Ref<SkinningMeshComponent> skinningMeshComponent; // Reference to component
            Vector<Ref<BoneHandle>>    boneHandles;           // List of bones handles

        public:
            // Updates handles positions by bones transforms
            void UpdateHandlesPositions();
        };

    public:
        Ref<SceneLayer> sceneLayer = mmake<SceneLayer>(); // Scene layer for drawing spline

        bool isEnabled = false; // Is tool enabled now     

    public:
        // Default constructor
        explicit SkeletonTool(RefCounter* refCounter);

        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        // Adds and enables handles for editing skeleton bones
        void AddSkeletonInstance(const Ref<SkinningMeshComponent>& skinningMeshComponent);

        // Removing skeleton from editing
        void RemoveSkeletonInstance(const Ref<SkinningMeshComponent>& skinningMeshComponent);

        // Returns whether the given skeleton is already being edited
        bool IsEditingSkeleton(const Ref<SkinningMeshComponent>& skinningMeshComponent) const;

        // Returns list of editing skeletons
        const Vector<Ref<SkeletonInstance>>& GetEditingSkeletons() const;

        IOBJECT(SkeletonTool);

    private:
        Vector<Ref<SkeletonInstance>> mSkeletons; // List of editing skeletons
    };
}
// --- META ---

CLASS_BASES_META(Editor::SkeletonTool)
{
    BASE_CLASS(Editor::IEditTool);
}
END_META;
CLASS_FIELDS_META(Editor::SkeletonTool)
{
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<SceneLayer>()).NAME(sceneLayer);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(isEnabled);
    FIELD().PRIVATE().NAME(mSkeletons);
}
END_META;
CLASS_METHODS_META(Editor::SkeletonTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDisabled);
    FUNCTION().PUBLIC().SIGNATURE(void, AddSkeletonInstance, const Ref<SkinningMeshComponent>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveSkeletonInstance, const Ref<SkinningMeshComponent>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditingSkeleton, const Ref<SkinningMeshComponent>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<SkeletonInstance>>&, GetEditingSkeletons);
}
END_META;
// --- END META ---
