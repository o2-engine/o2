#pragma once
#include "o2/Render/Pipeline/RenderPipeline.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/SceneLayersList.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"

namespace o2
{
    // -------------------------------------------------------------------------------
    // Camera actor. Can works with different types, renders layers or itself children
    // -------------------------------------------------------------------------------
    class CameraActor: public Actor
    {
    public:
        enum class Type { Default, FreeSize, FixedSize, FittedSize, PhysicalCorrect, Perspective };

    public:
        SceneLayersList drawLayers; // List of drawing layers @SERIALIZABLE

        bool   fillBackground = true;       // Is background filling with solid color @SERIALIZABLE
        Color4 fillColor = Color4::White(); // Background fill color @SERIALIZABLE @SCRIPTABLE

        Ref<CursorAreaEventListenersLayer> listenersLayer = mmake<CursorAreaEventListenersLayer>(); // Listeners layer

    public:
        // Default constructor @SCRIPTABLE
        CameraActor(RefCounter* refCounter);

        // Copy constructor
        CameraActor(RefCounter* refCounter, const CameraActor& other);

        // Destructor
        ~CameraActor() override;

        // Copy operator
        CameraActor& operator=(const CameraActor& other);

        // Sets camera to render
        void Setup();

        // Sets camera to render and renders content (layers or children)
        void SetupAndDraw();

        // Returns rendering camera
        Camera GetRenderCamera() const;

        // Sets default camera
        void SetDefault();

        // Sets camera with fixed size
        void SetFixedSize(const Vec2F& size);

        // Sets camera with fixed aspect @SCRIPTABLE
        void SetFittedSize(const Vec2F& size);

        // Sets camera with physical correct units
        void SetPhysicalCorrect(Units units);

        // Sets perspective camera with field of view in radians and clipping planes
        void SetPerspective(float fov, float nearClip, float farClip);

        // Returns perspective field of view in radians
        float GetFov() const;

        // Returns perspective near clipping plane
        float GetNearClip() const;

        // Returns perspective far clipping plane
        float GetFarClip() const;

        // Returns camera type
        Type GetCameraType() const;

        // Returns fitter or fixed camera size
        const Vec2F& GetFittedOrFixedSize() const;

        // Returns current camera units
        Units GetUnits() const;

        // Sets rendering pipeline. Null means the default forward pipeline
        void SetRenderPipeline(const Ref<RenderPipeline>& pipeline);

        // Returns rendering pipeline; null when the default forward pipeline is used
        const Ref<RenderPipeline>& GetRenderPipeline() const;

        // Returns the shared default forward pipeline, used when camera pipeline is null
        static const Ref<RenderPipeline>& GetDefaultRenderPipeline();

        SERIALIZABLE(CameraActor);
        CLONEABLE_REF(CameraActor);

    protected:
        Type  mType = Type::Default;       // Type of camera @SERIALIZABLE
        Vec2F mFixedOrFittedSize;          // Fitted or fixed types size @SERIALIZABLE
        Units mUnits = Units::Centimeters; // Physical camera units @SERIALIZABLE

        Ref<RenderPipeline> mPipeline; // Rendering pipeline; null - default forward pipeline @SERIALIZABLE @EDITOR_PROPERTY

        float mFov = Math::Deg2rad(60.0f); // Perspective field of view in radians @SERIALIZABLE
        float mNearClip = 0.1f;            // Perspective near clipping plane @SERIALIZABLE
        float mFarClip = 1000.0f;          // Perspective far clipping plane @SERIALIZABLE

    protected:
        // Called when actor has added to scene
        void OnAddToScene() override;

        // Called when actor has removed from scene
        void OnRemoveFromScene() override;
    };
}
// --- META ---

PRE_ENUM_META(o2::CameraActor::Type);

CLASS_BASES_META(o2::CameraActor)
{
    BASE_CLASS(o2::Actor);
}
END_META;
CLASS_FIELDS_META(o2::CameraActor)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(drawLayers);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(fillBackground);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(fillColor);
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<CursorAreaEventListenersLayer>()).NAME(listenersLayer);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Type::Default).NAME(mType);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mFixedOrFittedSize);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Units::Centimeters).NAME(mUnits);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mPipeline);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Math::Deg2rad(60.0f)).NAME(mFov);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.1f).NAME(mNearClip);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1000.0f).NAME(mFarClip);
}
END_META;
CLASS_METHODS_META(o2::CameraActor)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CameraActor&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup);
    FUNCTION().PUBLIC().SIGNATURE(void, SetupAndDraw);
    FUNCTION().PUBLIC().SIGNATURE(Camera, GetRenderCamera);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDefault);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFixedSize, const Vec2F&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetFittedSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPhysicalCorrect, Units);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPerspective, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetFov);
    FUNCTION().PUBLIC().SIGNATURE(float, GetNearClip);
    FUNCTION().PUBLIC().SIGNATURE(float, GetFarClip);
    FUNCTION().PUBLIC().SIGNATURE(Type, GetCameraType);
    FUNCTION().PUBLIC().SIGNATURE(const Vec2F&, GetFittedOrFixedSize);
    FUNCTION().PUBLIC().SIGNATURE(Units, GetUnits);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRenderPipeline, const Ref<RenderPipeline>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<RenderPipeline>&, GetRenderPipeline);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Ref<RenderPipeline>&, GetDefaultRenderPipeline);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
}
END_META;
// --- END META ---
