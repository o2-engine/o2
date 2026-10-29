#pragma once

#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2Editor/UI/ScrollView.h"
#include "o2Editor/Windows/SceneWindow/SceneView3DState.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------
    // 3D view of a model: the left button orbits, the right one pans, the wheel zooms. A static mesh is drawn
    // as it is, a skinned model in its bind pose or posed by an animation clip played in the view. The models
    // keep Y up, as glTF and OBJ do, and are turned to the Z up of the scene
    // ---------------------------------------------------------------------------------------------------
    class ModelView: public ScrollView
    {
    public:
        enum class DrawMode { Shaded, Wireframe, ShadedWireframe };

    public:
        // Default constructor
        ModelView(RefCounter* refCounter);

        // Copy-constructor
        ModelView(RefCounter* refCounter, const ModelView& other);

        // Copy-operator
        ModelView& operator=(const ModelView& other);

        // Shows a static mesh and fits it into the view
        void SetMesh(const Mesh3DData& data);

        // Shows a skinned model in its bind pose and fits it into the view
        void SetSkinnedModel(const AssetRef<SkinnedModelAsset>& model);

        // Returns the skinned model, empty for a static mesh
        const AssetRef<SkinnedModelAsset>& GetSkinnedModel() const;

        // Sets the animation clip of the skinned model, -1 for the bind pose; the time starts over
        void SetAnimation(int index);

        // Returns the animation clip index
        int GetAnimation() const;

        // Plays or pauses the animation clip
        void SetPlaying(bool playing);

        // Returns is the animation clip playing
        bool IsPlaying() const;

        // Returns time of the animation clip
        float GetTime() const;

        // Sets how the model is drawn
        void SetDrawMode(DrawMode mode);

        // Returns how the model is drawn
        DrawMode GetDrawMode() const;

        // Fits the model into the view
        void FitToView();

        // Returns count of the triangles of the model
        UInt GetTrianglesCount() const;

        // Returns count of the vertices of the model
        UInt GetVerticesCount() const;

        // Returns the orbit camera state
        const SceneView3DState& GetViewState() const;

        // Updates the animation clip time
        void Update(float dt) override;

        SERIALIZABLE(ModelView);
        CLONEABLE_REF(ModelView);

    protected:
        Mesh3DData                  mData;                     // Static mesh or the bind pose of the model
        AssetRef<SkinnedModelAsset> mModel;                    // Skinned model
        int                         mAnimation = -1;           // Animation clip, -1 for the bind pose
        bool                        mPlaying = false;          // Is the clip playing
        float                       mTime = 0.0f;              // Time of the clip
        DrawMode                    mDrawMode = DrawMode::Shaded; // How the model is drawn

        SceneView3DState mView;                // Orbit camera
        Mesh             mMesh;                // Filled triangles of the model
        bool             mMeshDirty = true;    // Are the triangles filled again before drawing
        bool             mFitPending = false;  // Is the model fitted when the view gets its size
        bool             mOrbiting = false;    // Is the camera orbited by the left button

    protected:
        // Draws the model with the orbit camera
        void RedrawContent() override;

        // Fills the triangles of the model for the clip time
        void RebuildMesh();

        // Returns the bounds of the model, turned to Z up
        bool GetBounds(AABB& bounds) const;

        // Called when transformation was changed and updated, fits the model when it waits for the size
        void OnTransformUpdated() override;

        // Called when cursor pressed on this: starts orbiting
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame: orbits the camera
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released: stops orbiting
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this: pans the camera
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when scrolling: zooms the camera
        void OnScrolled(float scroll) override;
    };
}
// --- META ---

PRE_ENUM_META(Editor::ModelView::DrawMode);

CLASS_BASES_META(Editor::ModelView)
{
    BASE_CLASS(Editor::ScrollView);
}
END_META;
CLASS_FIELDS_META(Editor::ModelView)
{
    FIELD().PROTECTED().NAME(mData);
    FIELD().PROTECTED().NAME(mModel);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mAnimation);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mTime);
    FIELD().PROTECTED().DEFAULT_VALUE(DrawMode::Shaded).NAME(mDrawMode);
    FIELD().PROTECTED().NAME(mView);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mMeshDirty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFitPending);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOrbiting);
}
END_META;
CLASS_METHODS_META(Editor::ModelView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ModelView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMesh, const Mesh3DData&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSkinnedModel, const AssetRef<SkinnedModelAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<SkinnedModelAsset>&, GetSkinnedModel);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetAnimation);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlaying, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTime);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawMode, DrawMode);
    FUNCTION().PUBLIC().SIGNATURE(DrawMode, GetDrawMode);
    FUNCTION().PUBLIC().SIGNATURE(void, FitToView);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetTrianglesCount);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetVerticesCount);
    FUNCTION().PUBLIC().SIGNATURE(const SceneView3DState&, GetViewState);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildMesh);
    FUNCTION().PROTECTED().SIGNATURE(bool, GetBounds, AABB&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
}
END_META;
// --- END META ---
