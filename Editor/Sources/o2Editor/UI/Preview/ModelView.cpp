#include "o2Editor/stdafx.h"
#include "ModelView.h"

#include "o2/Render/Mesh3DFill.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Quaternion.h"

namespace Editor
{
    namespace
    {
        // glTF and OBJ keep Y up, the scene keeps Z up
        Mat4 ModelToScene()
        {
            return Mat4::Rotation(Quat::FromEuler(Vec3F(Math::Deg2rad(90.0f), 0.0f, 0.0f)));
        }
    }

    ModelView::ModelView(RefCounter* refCounter):
        ScrollView(refCounter)
    {
        horGridEnabled = false;
        verGridEnabled = false;
        mView.yaw = Math::Deg2rad(30.0f);
        mView.pitch = Math::Deg2rad(65.0f);
    }

    ModelView::ModelView(RefCounter* refCounter, const ModelView& other):
        ScrollView(refCounter, other), mDrawMode(other.mDrawMode), mView(other.mView)
    {
        horGridEnabled = false;
        verGridEnabled = false;

        if (other.mModel)
            SetSkinnedModel(other.mModel);
        else
            SetMesh(other.mData);
    }

    ModelView& ModelView::operator=(const ModelView& other)
    {
        ScrollView::operator=(other);

        mDrawMode = other.mDrawMode;
        mView = other.mView;

        if (other.mModel)
            SetSkinnedModel(other.mModel);
        else
            SetMesh(other.mData);

        return *this;
    }

    void ModelView::SetMesh(const Mesh3DData& data)
    {
        mModel = AssetRef<SkinnedModelAsset>();
        mData = data;
        mAnimation = -1;
        mTime = 0.0f;
        mMeshDirty = true;

        mFitPending = true;
        FitToView();
    }

    void ModelView::SetSkinnedModel(const AssetRef<SkinnedModelAsset>& model)
    {
        mModel = model;
        mData = Mesh3DData();
        if (model)
        {
            auto& source = model->GetModelData();
            mData.positions = source.positions;
            mData.normals = source.normals;
            mData.uvs = source.uvs;
            mData.indices = source.indices;
        }

        mAnimation = -1;
        mTime = 0.0f;
        mMeshDirty = true;

        mFitPending = true;
        FitToView();
    }

    const AssetRef<SkinnedModelAsset>& ModelView::GetSkinnedModel() const
    {
        return mModel;
    }

    void ModelView::SetAnimation(int index)
    {
        mAnimation = mModel && index >= 0 && index < mModel->GetModelData().animations.Count() ? index : -1;
        mTime = 0.0f;
        mMeshDirty = true;
    }

    int ModelView::GetAnimation() const
    {
        return mAnimation;
    }

    void ModelView::SetPlaying(bool playing)
    {
        mPlaying = playing;
    }

    bool ModelView::IsPlaying() const
    {
        return mPlaying;
    }

    float ModelView::GetTime() const
    {
        return mTime;
    }

    void ModelView::SetDrawMode(DrawMode mode)
    {
        mDrawMode = mode;
    }

    ModelView::DrawMode ModelView::GetDrawMode() const
    {
        return mDrawMode;
    }

    void ModelView::FitToView()
    {
        AABB bounds;
        Vec2F viewSize = layout->GetWorldRect().Size();
        if (viewSize.x < 1.0f || viewSize.y < 1.0f || !GetBounds(bounds))
            return;

        mFitPending = false;

        float radius = Math::Max(bounds.GetSize().Length()*0.5f, 0.001f);
        float aspect = viewSize.x/viewSize.y;
        float halfFov = mView.fov*0.5f;
        float halfFovX = std::atan(std::tan(halfFov)*aspect);
        float distance = radius/Math::Sin(Math::Min(halfFov, halfFovX))*0.9f;

        mView.target = bounds.GetCenter();
        mView.distance = Math::Clamp(distance, SceneView3DState::minDistance, SceneView3DState::maxDistance);
        mView.nearClip = Math::Max(distance*0.01f, 0.001f);
        mView.farClip = distance*10.0f + radius*4.0f;
    }

    UInt ModelView::GetTrianglesCount() const
    {
        return mData.indices.Count()/3;
    }

    UInt ModelView::GetVerticesCount() const
    {
        return mData.positions.Count();
    }

    const SceneView3DState& ModelView::GetViewState() const
    {
        return mView;
    }

    void ModelView::Update(float dt)
    {
        ScrollView::Update(dt);

        if (!mPlaying || !mModel || mAnimation < 0)
            return;

        float duration = mModel->GetModelData().animations[mAnimation].duration;
        mTime += dt;
        if (duration > 0.0f && mTime > duration)
            mTime = Math::Mod(mTime, duration);

        mMeshDirty = true;
    }

    void ModelView::RedrawContent()
    {
        if (mMeshDirty)
            RebuildMesh();

        if (mMesh.polyCount == 0)
            return;

        Camera screenCamera = o2Render.GetCamera();
        o2Render.SetCamera(mView.BuildCamera());

        if (mDrawMode != DrawMode::Wireframe)
        {
            o2Render.SetDepthTestEnabled(true);
            mMesh.Draw();
            o2Render.SetDepthTestEnabled(false);
        }

        if (mDrawMode != DrawMode::Shaded)
        {
            o2Render.SetDebugDrawMode(Render::DebugDrawMode::Wireframe,
                                      mDrawMode == DrawMode::Wireframe ? Color4(200, 210, 220, 255) : Color4(0, 0, 0, 120));
            mMesh.Draw();
            o2Render.SetDebugDrawMode(Render::DebugDrawMode::Normal);
        }

        o2Render.SetCamera(screenCamera);
    }

    void ModelView::RebuildMesh()
    {
        mMeshDirty = false;

        Mesh3DData data = mData;
        if (mModel && mAnimation >= 0)
        {
            Vector<Mat4> palette;
            auto& model = mModel->GetModelData();
            model.EvaluateJointsPalette(mAnimation, mTime, palette);
            model.SkinVertices(palette, data.positions, data.normals);
        }

        if (data.normals.Count() != data.positions.Count())
            SkinnedModelData::ComputeSmoothNormals(data.positions, data.indices, data.normals);

        Mesh3DPrimitives::FillMesh(mMesh, data, ModelToScene(), Color4(225, 225, 230, 255), TextureSource(), true);
    }

    bool ModelView::GetBounds(AABB& bounds) const
    {
        AABB local;
        if (!mData.GetBounds(local))
            return false;

        Mat4 transform = ModelToScene();
        bounds = AABB(transform.TransformPoint(local.min), transform.TransformPoint(local.min));
        for (int i = 0; i < 8; i++)
        {
            Vec3F corner((i & 1) ? local.max.x : local.min.x, (i & 2) ? local.max.y : local.min.y,
                         (i & 4) ? local.max.z : local.min.z);
            bounds.Include(transform.TransformPoint(corner));
        }

        return true;
    }

    void ModelView::OnTransformUpdated()
    {
        ScrollView::OnTransformUpdated();

        if (mFitPending)
            FitToView();
    }

    void ModelView::OnCursorPressed(const Input::Cursor& cursor)
    {
        mOrbiting = true;
    }

    void ModelView::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mOrbiting || cursor.delta.Length() < Math::Epsilon)
            return;

        mView.Orbit(Vec2F(-cursor.delta.x, cursor.delta.y)*Math::Deg2rad(0.4f));
    }

    void ModelView::OnCursorReleased(const Input::Cursor& cursor)
    {
        mOrbiting = false;
    }

    void ModelView::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {
        if (cursor.delta.Length() > Math::Epsilon)
            mView.Pan(cursor.delta, layout->GetWorldRect().Size());
    }

    void ModelView::OnScrolled(float scroll)
    {
        mView.Zoom(1.0f - WheelZoomStep(scroll));
    }
}
// --- META ---

ENUM_META(Editor::ModelView::DrawMode, Editor__ModelView__DrawMode)
{
    ENUM_ENTRY(Shaded);
    ENUM_ENTRY(ShadedWireframe);
    ENUM_ENTRY(Wireframe);
}
END_ENUM_META;

DECLARE_CLASS(Editor::ModelView, Editor__ModelView);
// --- END META ---
