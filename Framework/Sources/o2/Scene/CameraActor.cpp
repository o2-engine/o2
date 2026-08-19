#include "o2/stdafx.h"
#include "CameraActor.h"

#include "Component.h"
#include "o2/Render/Gizmos.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Render/Render.h"
#include "o2/Scene/ISceneDrawable.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    CameraActor::CameraActor(RefCounter* refCounter) :
        Actor(refCounter)
    {}

    CameraActor::CameraActor(RefCounter* refCounter, const CameraActor& other) :
        Actor(refCounter, other), mType(other.mType), mFixedOrFittedSize(other.mFixedOrFittedSize), mUnits(other.mUnits),
        mFov(other.mFov), mNearClip(other.mNearClip), mFarClip(other.mFarClip)
    {
        if (other.mPipeline)
            mPipeline = other.mPipeline->CloneAsRef<RenderPipeline>();
    }

    CameraActor::~CameraActor()
    {
        o2Scene.OnCameraRemovedScene(this);
    }

    CameraActor& CameraActor::operator=(const CameraActor& other)
    {
        Actor::operator=(other);

        mType = other.mType;
        mFixedOrFittedSize = other.mFixedOrFittedSize;
        mUnits = other.mUnits;
        mFov = other.mFov;
        mNearClip = other.mNearClip;
        mFarClip = other.mFarClip;
        mPipeline = other.mPipeline ? other.mPipeline->CloneAsRef<RenderPipeline>() : nullptr;

        return *this;
    }

    void CameraActor::Setup()
    {
        PROFILE_SAMPLE_FUNC();
        o2Render.SetCamera(GetRenderCamera());
    }

    void CameraActor::SetupAndDraw()
    {
        PROFILE_SAMPLE_FUNC();

        if (fillBackground)
            o2Render.Clear(fillColor);

        listenersLayer->OnBeginDraw();

        Camera prevCamera = o2Render.GetCamera();
        Setup();

        listenersLayer->camera = o2Render.GetCamera();

        if (o2Input.IsKeyDown('G'))
        {
            o2Debug.Log("==========================Draw dump");

            for (auto& layer : drawLayers.GetLayers())
            {
                o2Debug.Log("== Layer " + layer->GetName() + " ==");

                Function<void(const Ref<ISceneDrawable>&, int)> printDrawable = [&printDrawable](const Ref<ISceneDrawable>& drawable, int depth)
                {
                    String str;
                    for (int i = 0; i < depth; i++)
                        str += "  ";

                    str += "(" + drawable->GetType().GetName() + ") ";
                    auto actor = DynamicCast<Actor>(drawable);
                    if (!actor)
                    {
                        if (auto component = DynamicCast<Component>(drawable))
                            actor = component->GetActor();
                    }

                    while (actor)
                    {
                        str += actor->GetName();
                        if (actor->GetParent())
                            str += " #" + (String)(actor->GetParent().Lock()->GetChildren().IndexOf(actor)) + "/";

                        actor = actor->GetParent().Lock();
                    }

                    o2Debug.Log(str);

                    for (auto& inherited : drawable->GetChildrenInheritedDepth())
                        printDrawable(inherited, depth + 1);
                };

                for (auto& drawable : layer->mDrawables)
                {                    
                    printDrawable(drawable, 1);

                    if (auto root = DynamicCast<SceneLayerRootDrawablesContainer>(drawable))
                    {
                        o2Debug.Log("  ROOT:");

                        for (auto& child : root->GetChildrenInheritedDepth())
                        {
                            printDrawable(child, 2);
                        }
                    }
                }
            }
        }

        {
            PROFILE_SAMPLE("CameraActor::SetupAndDraw - Execute render pipeline");

            RenderPassContext context;
            context.cameraActor = this;
            context.camera = o2Render.GetCamera();
            context.layers = drawLayers.GetLayers();
            context.fillBackground = fillBackground;
            context.fillColor = fillColor;

            Ref<RenderPipeline> pipeline = mPipeline ? mPipeline : GetDefaultRenderPipeline();
            pipeline->Execute(context);
        }

        o2Render.SetCamera(prevCamera);

        listenersLayer->OnEndDraw();
    }

    Camera CameraActor::GetRenderCamera() const
    {
        if (mType == Type::Perspective)
        {
            Camera camera = Camera::Perspective(mFov, mNearClip, mFarClip);

            Vec3F position, scale;
            Quat rotation;
            transform->GetWorldTransform3D().Decompose(position, rotation, scale);
            camera.position = position;
            camera.rotation = rotation;

            return camera;
        }

        Camera camera;
        switch (mType)
        {
            case Type::Default: camera = Camera::Default(); break;
            case Type::FreeSize: camera = Camera::FixedSize(transform->GetSize2D()); break;
            case Type::FixedSize: camera = Camera::FixedSize(mFixedOrFittedSize); break;
            case Type::FittedSize: camera = Camera::FittedSize(mFixedOrFittedSize); break;
            case Type::PhysicalCorrect: camera = Camera::PhysicalCorrect(mUnits); break;
            default: camera = Camera::Default(); break;
        }

        transform->size2D = camera.GetSize2D();
        transform->Update();
        camera.basis = transform->worldBasis;

        return camera;
    }

    void CameraActor::SetDefault()
    {
        mType = Type::Default;
    }

    void CameraActor::SetFixedSize(const Vec2F& size)
    {
        mType = Type::FixedSize;
        mFixedOrFittedSize = size;
        mUnits = Units::Pixels;
    }

    void CameraActor::SetFittedSize(const Vec2F& size)
    {
        mType = Type::FittedSize;
        mFixedOrFittedSize = size;
        mUnits = Units::Pixels;
    }

    void CameraActor::SetPhysicalCorrect(Units units)
    {
        mType = Type::PhysicalCorrect;
        mUnits = units;
    }

    void CameraActor::SetPerspective(float fov, float nearClip, float farClip)
    {
        mType = Type::Perspective;
        mFov = fov;
        mNearClip = nearClip;
        mFarClip = farClip;
    }

    float CameraActor::GetFov() const
    {
        return mFov;
    }

    float CameraActor::GetNearClip() const
    {
        return mNearClip;
    }

    float CameraActor::GetFarClip() const
    {
        return mFarClip;
    }

    CameraActor::Type CameraActor::GetCameraType() const
    {
        return mType;
    }

    const Vec2F& CameraActor::GetFittedOrFixedSize() const
    {
        return mFixedOrFittedSize;
    }

    Units CameraActor::GetUnits() const
    {
        return mUnits;
    }

    void CameraActor::SetRenderPipeline(const Ref<RenderPipeline>& pipeline)
    {
        mPipeline = pipeline;
    }

    const Ref<RenderPipeline>& CameraActor::GetRenderPipeline() const
    {
        return mPipeline;
    }

    const Ref<RenderPipeline>& CameraActor::GetDefaultRenderPipeline()
    {
        static Ref<RenderPipeline> defaultPipeline;
        if (!defaultPipeline)
            defaultPipeline = mmake<ForwardPipeline>();

        return defaultPipeline;
    }

    void CameraActor::OnAddToScene()
    {
        o2Scene.OnCameraAddedOnScene(this);

        Actor::OnAddToScene();
    }

    void CameraActor::OnRemoveFromScene()
    {
        o2Scene.OnCameraRemovedScene(this);

        Actor::OnRemoveFromScene();
    }

#if IS_EDITOR
    void CameraActor::OnDrawGizmos()
    {
        o2Gizmos.SetColor(Gizmos::cameraColor);

        if (mType != Type::Perspective)
        {
            Basis basis = transform->GetWorldNonSizedBasis();
            Vec2F half = transform->GetSize2D()*0.5f;
            float z = transform->GetWorldPosition().z;

            o2Gizmos.DrawPolyLine({ Vec3F(Vec2F(-half.x, -half.y)*basis, z), Vec3F(Vec2F(half.x, -half.y)*basis, z),
                                    Vec3F(Vec2F(half.x, half.y)*basis, z), Vec3F(Vec2F(-half.x, half.y)*basis, z) },
                                  true);
            return;
        }

        Vec3F position = transform->GetWorldPosition();
        Quat rotation = transform->GetWorldRotation();
        Vec3F right = rotation*Vec3F(1, 0, 0);
        Vec3F up = rotation*Vec3F(0, 1, 0);
        Vec3F forward = rotation*Vec3F(0, 0, -1);

        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        float aspect = resolution.y > FLT_EPSILON ? resolution.x/resolution.y : 1.0f;
        float tangent = Math::Sin(mFov*0.5f)/Math::Cos(mFov*0.5f);

        Vec3F nearCorners[4], farCorners[4];
        auto planeCorners = [&](float distance, Vec3F* corners)
        {
            Vec3F center = position + forward*distance;
            Vec3F halfUp = up*(tangent*distance);
            Vec3F halfRight = right*(tangent*distance*aspect);

            corners[0] = center - halfRight - halfUp;
            corners[1] = center + halfRight - halfUp;
            corners[2] = center + halfRight + halfUp;
            corners[3] = center - halfRight + halfUp;
        };

        planeCorners(mNearClip, nearCorners);
        planeCorners(mFarClip, farCorners);

        o2Gizmos.DrawPolyLine({ nearCorners[0], nearCorners[1], nearCorners[2], nearCorners[3] }, true);
        o2Gizmos.DrawPolyLine({ farCorners[0], farCorners[1], farCorners[2], farCorners[3] }, true);

        for (int i = 0; i < 4; i++)
            o2Gizmos.DrawLine(nearCorners[i], farCorners[i]);
    }
#endif

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::CameraActor>);
// --- META ---

ENUM_META(o2::CameraActor::Type, o2__CameraActor__Type)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(FittedSize);
    ENUM_ENTRY(FixedSize);
    ENUM_ENTRY(FreeSize);
    ENUM_ENTRY(Perspective);
    ENUM_ENTRY(PhysicalCorrect);
}
END_ENUM_META;

DECLARE_CLASS(o2::CameraActor, o2__CameraActor);
// --- END META ---
