#pragma once

#include "ft2build.h"
#include FT_FREETYPE_H

#if defined PLATFORM_WINDOWS
#include "o2/Render/Windows/RenderBase.h"
#elif defined PLATFORM_ANDROID
#include "o2/Render/Android/RenderBase.h"
#elif defined PLATFORM_MAC
#include "o2/Render/Mac/RenderBase.h"
#elif defined PLATFORM_IOS
#include "o2/Render/iOS/RenderBase.h"
#elif defined(PLATFORM_LINUX)
#if defined(O2_RENDER_GLES2)
#include "o2/Render/Linux GLES2/RenderBase.h"
#else
#include "o2/Render/Linux/RenderBase.h"
#endif
#endif

#include "o2/Render/Camera.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"

// Render access macros
#define o2Render o2::Render::Instance()

namespace o2
{
    FORWARD_CLASS_REF(AtlasAsset);

    class CursorAreaEventListenersLayer;
    class Font;
    class Mesh;
    class Sprite;

    // ------------------
    // 2D Graphics render
    // ------------------
    class Render: public RenderBase, public Singleton<Render>
    {
    public:
        // ---------------------
        // Scissor clipping info
        // ---------------------
        struct ScissorInfo
        {
            float beginDepth = 0.0f; // Drawing depth on enabling clipping
            float endDepth = 0.0f;   // Drawing depth on disabling clipping
            RectI scissorRect;       // Scissor clipping rectangle

        public:
            ScissorInfo();
            ScissorInfo(const RectI& rect, float beginDepth);

            bool operator==(const ScissorInfo& other) const;
        };

        // --------------------------------
        // Scissor clipping stack info item
        // --------------------------------
        struct ScissorStackEntry
        {
            RectI scissorRect;          // Clipping scissor rectangle
            RectI summaryScissorRect;   // Real clipping rectangle: summary of top clipping rectangles
            bool  renderTarget = false; // Is render target turned on this step

        public:
            ScissorStackEntry();
            ScissorStackEntry(const RectI& rect, const RectI& summaryRect, bool renderTarget = false);

            bool operator==(const ScissorStackEntry& other) const;
        };

    public:
        PROPERTIES(Render);
        PROPERTY(Camera, camera, SetCamera, GetCamera);                           // Current camera property
        PROPERTY(RectI, scissorRect, EnableScissorTest, GetScissorRect);          // Scissor rect property
        PROPERTY(TextureRef, renderTexture, BindRenderTexture, GetRenderTexture); // Render target texture property
        GETTER(Vec2I, resolution, GetResolution);                                 // Screen resolution getter
        GETTER(Vec2I, maxTextureSize, GetMaxTextureSize);                         // Maximal texture size getter

    public:
        Function<void()> preRender;  // Pre rendering event. Call after beginning drawing. Clearing every frame
        Function<void()> postRender; // Post rendering event. Call before ending drawing. Clearing every frame

    public:
        // Default constructor
        Render(RefCounter* refCounter);

        // Destructor
        ~Render();

        // Beginning rendering
        void Begin();

        // Finishing rendering
        void End();

        // Flushes current state and gets ready to draw manually
        void BeginCustomRender();

        // Finishes custom rendering, restores state and cameras
        void EndCustomRender();

        // Resets render's state
        void ResetState();

        // Clearing current frame buffer with color
        void Clear(const Color4& color = Color4::Gray());

        // Returns resolution of rendering frame
        Vec2I GetResolution() const;

        // Returns current buffer resolution
        Vec2I GetCurrentResolution() const;

        // Returns device's screen dpi
        Vec2I GetDPI() const;

        // Returns current draw calls count 
        int GetDrawCallsCount() const;

        // Returns current drawn primitives
        int GetDrawnPrimitives() const;

        // Binding camera. NULL - standard camera
        void SetCamera(const Camera& camera);

        // Returns current camera
        Camera GetCamera() const;

        // Draws polygon
        void DrawFilledPolygon(const Vector<Vec2F>& points, const Color4& color = Color4::White());

        // Draws polygon
        void DrawFilledPolygon(const Vertex* points, int vertexCount);

        // Draws single line with color
        void DrawLine(const Vec2F& a, const Vec2F& b, const Color4& color = Color4::White());

        // Draws single line with color
        void DrawArrow(const Vec2F& a, const Vec2F& b, const Color4& color = Color4::White(),
                       const Vec2F& arrowSize = Vec2F(10, 10));

        // Draws single line with color
        void DrawLine(const Vector<Vec2F>& points, const Color4& color = Color4::White());

        // Draws rect frame with color
        void DrawRectFrame(const Vec2F& minp, const Vec2F& maxp, const Color4& color = Color4::White());

        // Draws rect frame with color
        void DrawRectFrame(const RectF& rect, const Color4& color = Color4::White());

        // Draws basis frame
        void DrawBasis(const Basis& basis, const Color4& xcolor = Color4::Red(), const Color4& ycolor = Color4::Blue(),
                       const Color4& color = Color4::White());

        // Draws cross with color
        void DrawCross(const Vec2F& pos, float size = 5, const Color4& color = Color4::White());

        // Draws circle with color
        void DrawCircle(const Vec2F& pos, float radius = 5, const Color4& color = Color4::White(), int segCount = 20);

        // Draws filled circle with color
        void DrawFilledCircle(const Vec2F& pos, float radius = 5, const Color4& color = Color4::White(), int segCount = 20);

        // Draws bezier curve with color
        void DrawBezierCurve(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
                             const Color4& color = Color4::White());

        // Draws bezier curve with color
        void DrawBezierCurveArrow(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
                                  const Color4& color = Color4::White(), const Vec2F& arrowSize = Vec2F(10, 10));

        // Draws anti-aliased single line with color
        void DrawAALine(const Vec2F& a, const Vec2F& b, const Color4& color = Color4::White(),
                        float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased single line with color
        void DrawAAArrow(const Vec2F& a, const Vec2F& b, const Color4& color = Color4::White(),
                         const Vec2F& arrowSize = Vec2F(10, 10),
                         float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased single line with color
        void DrawAALine(const Vector<Vec2F>& points, const Color4& color = Color4::White(),
                        float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased rect frame with color
        void DrawAARectFrame(const Vec2F& minp, const Vec2F& maxp, const Color4& color = Color4::White(),
                             float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased rect frame with color
        void DrawAARectFrame(const RectF& rect, const Color4& color = Color4::White(),
                             float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased basis frame
        void DrawAABasis(const Basis& basis, const Color4& xcolor = Color4::Red(), const Color4& ycolor = Color4::Blue(),
                         const Color4& color = Color4::White(),
                         float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased cross with color
        void DrawAACross(const Vec2F& pos, float size = 5, const Color4& color = Color4::White(),
                         float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased circle with color
        void DrawAACircle(const Vec2F& pos, float radius = 5, const Color4& color = Color4::White(), int segCount = 20,
                          float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased bezier curve with color
        void DrawAABezierCurve(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
                               const Color4& color = Color4::White(),
                               float width = 1.0f, LineType lineType = LineType::Solid);

        // Draws anti-aliased bezier curve with color
        void DrawAABezierCurveArrow(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
                                    const Color4& color = Color4::White(), const Vec2F& arrowSize = Vec2F(10, 10),
                                    float width = 1.0f, LineType lineType = LineType::Solid);

        // Beginning render to stencil buffer
        void BeginRenderToStencilBuffer();

        // Finishing rendering in stencil buffer
        void EndRenderToStencilBuffer();

        // Enabling stencil test
        void EnableStencilTest();

        // Disabling stencil test
        void DisableStencilTest();

        // Returns true, if stencil test enabled
        bool IsStencilTestEnabled() const;

        // Clearing stencil buffer
        void ClearStencil();

        // Returns scissor rect
        RectI GetScissorRect() const;

        // Returns result scissor rect of all scissor stack
        RectI GetResScissorRect() const;

        // Returns scissors stack
        const Vector<ScissorStackEntry>& GetScissorsStack() const;

        // Enabling scissor test
        void EnableScissorTest(const RectI& rect);

        // Disabling scissor test
        void DisableScissorTest(bool forcible = false);

        // Returns true, if scissor test enabled
        bool IsScissorTestEnabled() const;

        // Returns true when specified rectangle is fully clipped by current scissor test
        bool IsClippedByScissor(const RectF& rect) const;

        // Returns true when specified point is clipped by current scissor test
        bool IsClippedByScissor(const Vec2F& point) const;

        // Draws mesh
        void DrawMesh(Mesh* mesh);

        // Draws data from buffer with specified texture and primitive type
        void DrawBuffer(PrimitiveType primitiveType, Vertex* vertices, UInt verticesCount,
                        VertexIndex* indexes, UInt elementsCount, const TextureRef& texture,
                        BlendMode blendMode);

        // Platform specific upload vertex and index buffers
        void PlatformUploadBuffers(Vertex* vertices, UInt verticesCount,
                                   VertexIndex* indexes, UInt indexesCount);

        // Draws mesh wire
        void DrawMeshWire(Mesh* mesh, const Color4& color = Color4::White());

        // Draws mesh buffer wire
        void DrawMeshBufferWire(Vertex* vertices, UInt verticesCount, VertexIndex* indexes, UInt elementsCount,
                                const Color4& color = Color4::White());

        // Draws hard poly line. Vertices - buffer of vertex pairs for each line
        void DrawPolyLine(Vertex* vertices, int count, float width = 1.0f);

        // Draws anti-aliased lines
        void DrawAAPolyLine(Vertex* vertices, int count, float width = 1.0f, LineType lineType = LineType::Solid,
                            bool scaleToScreenSpace = true);

        // Binding render target
        void BindRenderTexture(TextureRef renderTarget);

        // Unbinding render target
        void UnbindRenderTexture();

        // Returns current render target. Returns NULL if no render target
        TextureRef GetRenderTexture() const;

        // Returns maximum texture size
        Vec2I GetMaxTextureSize() const;

        // Returns last draw depth of mesh
        float GetDrawingDepth();

        // Returns scissor infos at current frame
        const Vector<ScissorInfo>& GetScissorInfos() const;

    protected:
        PrimitiveType mCurrentPrimitiveType = PrimitiveType::Polygon; // Type of drawing primitives for next DIP
        BlendMode     mCurrentBlendMode = BlendMode::Normal;          // Current blend mode for next DIP

        TextureRef mCurrentDrawTexture = nullptr; // Stored texture ptr from last DIP
        UInt       mLastDrawVertex;               // Last vertex idx for next DIP
        UInt       mLastDrawIdx;                  // Last vertex index for next DIP
        UInt       mTrianglesCount;               // Triangles count for next DIP
        UInt       mFrameTrianglesCount;          // Total triangles at current frame
        UInt       mDrawCallsCount;               // DrawIndexedPrimitives calls count

        Ref<LogStream> mLog; // Render log stream

        TextureRef mWhiteTexture; // Default white texture

        Vector<TextureRef> mTextures; // Loaded textures
        Vector<Ref<Font>>  mFonts;    // Loaded fonts

        Camera mCamera;     // Camera transformation
        Camera mPrevCamera; // Previous camera transformation

        Vec2I  mResolution;        // Primary back buffer size
        Vec2I  mCurrentResolution; // Current back buffer size
        Vec2I  mPrevResolution;    // Previous back buffer size

        Vec2F  mViewScale;         // Current view scale, depends on camera
        Vec2F  mInvViewScale;      // Inverted mViewScale
        Vec2I  mDPI;               // Current device screen DPI

        bool mStencilDrawing = false; // True, if drawing in stencil buffer
        bool mStencilTest = false;    // True, if drawing with stencil test

        Vector<ScissorInfo>       mScissorInfos;               // Scissor clipping depth infos vector
        Vector<ScissorStackEntry> mStackScissors;              // Stack of scissors clippings
        bool                      mClippingEverything = false; // Is everything clipped

        TextureRef mCurrentRenderTarget; // Current render target. NULL if rendering in back buffer

        float mDrawingDepth = 0.0f; // Current drawing depth, increments after each drawing drawables

        FT_Library mFreeTypeLib; // FreeType library, for rendering fonts

        Vector<Sprite*>         mSprites; // All sprites
        Vector<Ref<AtlasAsset>> mAtlases; // All atlases

        VertexIndex* mHardLinesIndexData = nullptr; // Index data buffer
        TextureRef   mSolidLineTexture;             // Solid line texture
        TextureRef   mDashLineTexture;              // Dash line texture

        Vec2I mMaxTextureSize; // Max texture size

        bool mReady = false; // True, if render is ready to draw

    protected:
        // Don't copy
        Render(const Render& other) = delete;

        // Don't copy
        Render& operator=(const Render& other);

        // Initializes platform specific part of render
        void InitializePlatform();

        // Initialized white texture
        void InitializeWhiteTexture();

        // Initializes index buffer for drawing lines - pairs of lines beginnings and ends
        void InitializeLinesIndexBuffer();

        // Initializes lines textures
        void InitializeLinesTextures();

        // Initializes free type library
        void InitializeFreeType();

        // Initializes standard shader
        void InitializeSandardShader();

        // Deinitializes platform specific part of render
        void DeinitializePlatform();

        // Deinitializes free type library
        void DeinitializeFreeType();

        // Returns platform specific max texture size
        Vec2I GetPlatformMaxTextureSize();

        // Returns platform specific DPI
        Vec2I GetPlatformDPI();

        // Called when target frame or window was resized
        void OnFrameResized();

        // Platform specific begin of rendering
        void PlatformBegin();

        // Platform specific end of rendering
        void PlatformEnd();

        // Send buffers to draw
        void DrawPrimitives();

        // Platform specific draw primitives (draw call)
        void PlatformDrawPrimitives();

        // Checks vertex buffer for texture coordinate flip by texture format
        void CheckVertexBufferTexCoordFlipByTextureFormat();

        // Platform specific flips current vertex buffer UX by Y
        void PlatformFlipVerticesUV();

        // Platform specific reset renderer state
        void PlatformResetState();

        // Sets orthographic view matrix by view size
        void SetupViewMatrix(const Vec2I& viewSize);

        // Updates render transformations for camera
        void UpdateCameraTransforms();

        // Platform specific setup camera transforms
        void PlatformSetupCameraTransforms(float* matrix);

        // Platform specific begin of stencil drawing
        void PlatformBeginStencilDrawing();

        // Platform specific end of stencil drawing
        void PlatformEndStencilDrawing();

        // Platform specific setup stencil test
        void PlatformEnableStencilTest();

        // Platform specific disable stencil test
        void PlatformDisableStencilTest();

        // Platform specific enable scissor test
        void PlatformEnableScissorTest();

        // Platform specific disable scissor test
        void PlatformDisableScissorTest();

        // Platform specific set scissor rect
        void PlatformSetScissorRect(const RectI& rect);

        // Platform specific bind render target
        void PlatformBindRenderTarget(const TextureRef& renderTarget);

        // Calculates screen space scissor clipping rectangle from camera space rectangle
        RectI CalculateScreenSpaceScissorRect(const RectF& cameraSpaceScissorRect) const;

        // Check textures for unloading
        void CheckTexturesUnloading();

        // Checks font for unloading
        void CheckFontsUnloading();

        // Called when assets was rebuilded
        void OnAssetsRebuilt(const Vector<UID>& changedAssets);

        // Called when sprite created, registers it in render
        static void OnSpriteCreated(Sprite* sprite);

        // Called when sprite destroyed, unregisters it from render
        static void OnSpriteDestroyed(Sprite* sprite);

        // Called when texture created, registers it in render
        void OnTextureCreated(Texture* texture);

        // Called when texture destroyed, unregisters it from render
        void OnTextureDestroyed(Texture* texture);

        // Called when atlas created, registers it in render
        void OnAtlasCreated(AtlasAsset* atlas);

        // Called when atlas destroyed, unregisters it from render
        void OnAtlasDestroyed(AtlasAsset* atlas);

        // Called when font created, registers it in render
        void OnFontCreated(Font* font);

        // Called when font destroyed, unregisters it from render
        void OnFontDestroyed(Font* font);

        friend class Application;
        friend class AtlasAsset;
        friend class BitmapFont;
        friend class BitmapFontAsset;
        friend class Font;
        friend class RenderBase;
        friend class Sprite;
        friend class Texture;
        friend class TextureRef;
        friend class VectorFont;
        friend class VectorFontAsset;
        friend class WndProcFunc;
        friend struct ApplicationPlatformWrapper;
    };
}
