#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/Camera.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"

using namespace o2;

namespace o2
{
    class Sprite;
}

namespace Editor
{
    // ----------------------------
    // Scrollable and zoomable view
    // ----------------------------
    class ScrollView: public Widget, virtual public CursorAreaEventsListener
    {
    public:
        bool horGridEnabled = true; // Is horizontal lines of grid are enabled
        bool verGridEnabled = true; // Is vertical lines of grid are enabled

    public:
        // Default constructor
        ScrollView(RefCounter* refCounter);

        // Copy-constructor
        ScrollView(RefCounter* refCounter, const ScrollView& other);

        // Destructor
        ~ScrollView();

        // Copy-operator
        ScrollView& operator=(const ScrollView& other);

        // Draws widget, updates render target 
        void Draw() override;

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Transforms point from screen space to local space
        Vec2F ScreenToLocalPoint(const Vec2F& point);

        // Transforms point from local space to screen space
        Vec2F LocalToScreenPoint(const Vec2F& point);

        // Returns local to screen transform
        const Basis& GetLocalToScreenTransform() const;

        // Returns screen to local transform
        const Basis& GetScreenToLocalTransform() const;

        // Returns camera scale
        Vec2F GetCameraScale() const;

        // Sets view camera
        void SetCamera(const Camera& camera);

        // Returns view camera
        const Camera& GetCamera() const;

        // Sets back color
        void SetBackColor(const Color4& color);

        // Sets grid color
        void SetGridColor(const Color4& color);

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns is this widget can be focused
        bool IsFocusable() const override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ScrollView);
        CLONEABLE_REF(ScrollView);

    protected:
        bool mReady = false; // Whether the widget is initialized and ready to use

		Ref<CursorAreaEventListenersLayer> mListenersLayer = mmake<CursorAreaEventListenersLayer>(); // Listeners layer, used to handle cursor events
                                                              
        Ref<Sprite> mRenderTargetSprite; // Render target sprite used for caching graphics
        TextureRef  mRenderTarget;       // Render target texture used for caching graphics
        bool        mNeedRedraw = false; // Whether the render target needs to be redrawn
                                                              
        Color4 mBackColor; // Background color @SERIALIZABLE
        Color4 mGridColor; // Grid line color @SERIALIZABLE

        RectF  mViewArea;                              // View area bounds
        Camera mViewCamera;                            // Scene view camera
        Vec2F  mViewCameraTargetScale = Vec2F(1, 1);   // Target scale for camera
        float  mViewCameraScaleSence = 0.1f / 120.0f;  // Camera scale sensitivity
        float  mViewCameraScaleElasticyCoef = 30.0f;   // Scale smoothing coefficient
        Vec2F  mViewCameraTargetPos;                   // Target camera position
        Vec2F  mViewCameraVelocity;                    // Camera movement velocity
        float  mViewCameraPosElasticyCoef = 30.0f;     // Camera position smoothing coefficient
        float  mViewCameraVelocityDampingCoef = 10.0f; // Camera velocity damping coefficient
        float  mViewCameraMinScale = 0.001f;           // Minimum camera scale
        float  mViewCameraMaxScale = 10000.0f;         // Maximum camera scale
		bool   mViewCameraMoved = false;               // Whether the camera was moved       
                   
        Basis mLocalToScreenTransform; // Transform from local to screen coordinates
        Basis mScreenToLocalTransform; // Transform from screen to local coordinates

    protected:
        // Called when transformation was changed and updated, updates render texture and sprite
        void OnTransformUpdated() override;

        // Updates transparency for this and children widgets
		void UpdateTransparency() override;

        // Updates camera
        void UpdateCamera(float dt);

        // Returns transformation basis from screen to local for camera
        Basis GetCameraScreenToLocalTransform(const Camera& camera) const;

        // Updates scene to screen and screen and scene transformations
        void UpdateLocalScreenTransforms();

        // Redraws content into render target
        void RedrawRenderTarget();

        // Redraws content into render target
        virtual void RedrawContent();

        // Draws grid
        virtual void DrawGrid();

        // Called when camera position was changed
        virtual void OnCameraTransformChanged();

        // Called when scrolling
        void OnScrolled(float scroll) override;

        // Updates camera's scale and position
        void ChangeCameraScaleRelativeToCursor(const Vec2F& newScale);

        // Called when right mouse button was pressed on this
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Returns that this scroll area has transparent input
        bool IsInputTransparent() const override;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ScrollView)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::ScrollView)
{
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(horGridEnabled);
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(verGridEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mReady);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<CursorAreaEventListenersLayer>()).NAME(mListenersLayer);
    FIELD().PROTECTED().NAME(mRenderTargetSprite);
    FIELD().PROTECTED().NAME(mRenderTarget);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedRedraw);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBackColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mGridColor);
    FIELD().PROTECTED().NAME(mViewArea);
    FIELD().PROTECTED().NAME(mViewCamera);
    FIELD().PROTECTED().DEFAULT_VALUE(Vec2F(1, 1)).NAME(mViewCameraTargetScale);
    FIELD().PROTECTED().DEFAULT_VALUE(0.1f / 120.0f).NAME(mViewCameraScaleSence);
    FIELD().PROTECTED().DEFAULT_VALUE(30.0f).NAME(mViewCameraScaleElasticyCoef);
    FIELD().PROTECTED().NAME(mViewCameraTargetPos);
    FIELD().PROTECTED().NAME(mViewCameraVelocity);
    FIELD().PROTECTED().DEFAULT_VALUE(30.0f).NAME(mViewCameraPosElasticyCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(10.0f).NAME(mViewCameraVelocityDampingCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(0.001f).NAME(mViewCameraMinScale);
    FIELD().PROTECTED().DEFAULT_VALUE(10000.0f).NAME(mViewCameraMaxScale);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mViewCameraMoved);
    FIELD().PROTECTED().NAME(mLocalToScreenTransform);
    FIELD().PROTECTED().NAME(mScreenToLocalTransform);
}
END_META;
CLASS_METHODS_META(Editor::ScrollView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ScrollView&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToLocalPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, LocalToScreenPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const Basis&, GetLocalToScreenTransform);
    FUNCTION().PUBLIC().SIGNATURE(const Basis&, GetScreenToLocalTransform);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetCameraScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCamera, const Camera&);
    FUNCTION().PUBLIC().SIGNATURE(const Camera&, GetCamera);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBackColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGridColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransparency);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateCamera, float);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetCameraScreenToLocalTransform, const Camera&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLocalScreenTransforms);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawRenderTarget);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawGrid);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCameraTransformChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, ChangeCameraScaleRelativeToCursor, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMousePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsInputTransparent);
}
END_META;
// --- END META ---
