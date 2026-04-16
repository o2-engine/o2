#include "o2/stdafx.h"
#include "CameraActor.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/ISceneDrawable.h"
#include "o2/Render/Render.h"
#include "Component.h"

namespace o2
{
    CameraActor::CameraActor(RefCounter* refCounter) :
        Actor(refCounter)
    {}

    CameraActor::CameraActor(RefCounter* refCounter, const CameraActor& other) :
        Actor(refCounter, other), mType(other.mType), mFixedOrFittedSize(other.mFixedOrFittedSize), mUnits(other.mUnits)
    {}

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
            PROFILE_SAMPLE("CameraActor::SetupAndDraw - Draw layers");

            for (auto& layer : drawLayers.GetLayers())
            {
                for (auto& comp : layer->mDrawables)
                    comp->Draw();
            }
        }

        o2Render.SetCamera(prevCamera);

        listenersLayer->OnEndDraw();
    }

    Camera CameraActor::GetRenderCamera() const
    {
        Camera camera;
        switch (mType)
        {
            case Type::Default: camera = Camera::Default(); break;
            case Type::FreeSize: camera = Camera::FixedSize(transform->GetSize()); break;
            case Type::FixedSize: camera = Camera::FixedSize(mFixedOrFittedSize); break;
            case Type::FittedSize: camera = Camera::FittedSize(mFixedOrFittedSize); break;
            case Type::PhysicalCorrect: camera = Camera::PhysicalCorrect(mUnits); break;
            default: camera = Camera::Default(); break;
        }

        transform->size = camera.GetSize();
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

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::CameraActor>);
// --- META ---

ENUM_META(o2::CameraActor::Type, o2__CameraActor__Type)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(FittedSize);
    ENUM_ENTRY(FixedSize);
    ENUM_ENTRY(FreeSize);
    ENUM_ENTRY(PhysicalCorrect);
}
END_ENUM_META;

DECLARE_CLASS(o2::CameraActor, o2__CameraActor);
// --- END META ---
