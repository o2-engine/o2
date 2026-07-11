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
#elif defined PLATFORM_WASM
#include "o2/Render/WebAssembly/RenderBase.h"
#elif defined(PLATFORM_LINUX)
#if defined(O2_RENDER_GLES2)
#include "o2/Render/Linux GLES2/RenderBase.h"
#else
#include "o2/Render/Linux/RenderBase.h"
#endif
#endif

#include "o2/Render/Camera.h"
#include "o2/Render/Material.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"

// Render access macros
#define o2Render o2::Render::Instance()

namespace o2
{
	FORWARD_CLASS_REF(AtlasAsset);

	class Bitmap;
	class CursorAreaEventListenersLayer;
	class Font;
	class Material;
	class Mesh;
	class Shader;
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

		// Requests capture of the next rendered frame: it is drawn into an offscreen target and
		// delivered as an upright bitmap inside that frame's End(). One-shot, replaces prev request
		void CaptureNextFrame(const Function<void(const Ref<Bitmap>&)>& onCaptured);

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
						float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased single line with color
		void DrawAAArrow(const Vec2F& a, const Vec2F& b, const Color4& color = Color4::White(),
						 const Vec2F& arrowSize = Vec2F(10, 10),
						 float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased single line with color
		void DrawAALine(const Vector<Vec2F>& points, const Color4& color = Color4::White(),
						float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased rect frame with color
		void DrawAARectFrame(const Vec2F& minp, const Vec2F& maxp, const Color4& color = Color4::White(),
							 float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased rect frame with color
		void DrawAARectFrame(const RectF& rect, const Color4& color = Color4::White(),
							 float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased basis frame
		void DrawAABasis(const Basis& basis, const Color4& xcolor = Color4::Red(), const Color4& ycolor = Color4::Blue(),
						 const Color4& color = Color4::White(),
						 float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased cross with color
		void DrawAACross(const Vec2F& pos, float size = 5, const Color4& color = Color4::White(),
						 float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased circle with color
		void DrawAACircle(const Vec2F& pos, float radius = 5, const Color4& color = Color4::White(), int segCount = 20,
						  float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased bezier curve with color
		void DrawAABezierCurve(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
							   const Color4& color = Color4::White(),
							   float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

		// Draws anti-aliased bezier curve with color
		void DrawAABezierCurveArrow(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
									const Color4& color = Color4::White(), const Vec2F& arrowSize = Vec2F(10, 10),
									float width = 1.0f, LineType lineType = LineType::Solid, bool scaleToScreenSpace = true);

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

		// Enables or disables depth buffer test, flushes current batch
		void SetDepthTestEnabled(bool enabled);

		// Returns true when depth buffer test is enabled
		bool IsDepthTestEnabled() const;

		// Draws mesh
		void DrawMesh(Mesh* mesh);

		// Draws data from buffer. Vertex type describes the source vertex format.
		// If allowVertexConversion is true, vertices will be converted to match the material's texcoord requirements
		// by remapping 0..1 UVs through texSrcRect. If false and types don't match, asserts.
		void DrawBuffer(PrimitiveType primitiveType, const UInt8* vertices, UInt verticesCount, const VertexType& vertexType,
						VertexIndex* indexes, UInt elementsCount,
						const Ref<Material>& material, const TextureRef& overrideTexture, const RectI& texSrcRect = RectI(), 
						bool allowVertexConversion = false);

		// Convenience overload for typed Vertex data
		void DrawBuffer(PrimitiveType primitiveType, const Vertex* vertices, UInt verticesCount,
						VertexIndex* indexes, UInt elementsCount, 
						const Ref<Material>& material, const TextureRef& overrideTexture, const RectI& texSrcRect = RectI(),
						bool allowVertexConversion = false);

		// Fills the CPU-side batch buffer with vertex/index data, applying UV remapping if needed
		void UploadBuffers(const UInt8* vertices, UInt verticesCount, const VertexType& srcVertexType,
						   VertexIndex* indexes, UInt indexesCount, const RectI& texSrcRect,
						   const TextureRef& texture, bool allowVertexConversion);

		// Draws geometry that exceeds batch buffers capacity by splitting it into re-indexed chunks
		void DrawBufferChunked(PrimitiveType primitiveType, const UInt8* vertices, UInt verticesCount,
							   const VertexType& vertexType, VertexIndex* indexes, UInt elementsCount,
							   const Ref<Material>& material, const TextureRef& overrideTexture,
							   const RectI& texSrcRect, bool allowVertexConversion, UInt vertexCapacity);

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

		// Returns true when platform supports multiple render targets
		bool IsMRTSupported() const;

		// Binds multiple render targets (MRT). First target is primary, provides depth attachment.
		// When MRT is not supported, binds only the first target and warns once
		void BindRenderTargets(const Vector<TextureRef>& renderTargets);

		// Binds render targets remembering the current one; restore with PopRenderTargets
		void PushRenderTargets(const Vector<TextureRef>& renderTargets);

		// Restores the render target saved by the paired PushRenderTargets
		void PopRenderTargets();

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

		// Binds material for rendering. nullptr is ignored (no-op).
		void BindMaterial(const Ref<Material>& material);

		// Returns current bound material
		const Ref<Material>& GetCurrentMaterial() const;

		// Returns default material
		const Ref<Material>& GetDefaultMaterial() const;

		// Sets material that overrides all materials in draw calls (render pass override). nullptr disables
		void SetOverrideMaterial(const Ref<Material>& material);

		// Returns current override material
		const Ref<Material>& GetOverrideMaterial() const;

		// Returns true when material's color attachment formats match the currently bound render targets
		bool IsMaterialCompatibleWithCurrentTargets(const Ref<Material>& material) const;

#if IS_EDITOR
		// Reloads all shader and texture assets from disk on editor activation
		void ReloadAssetsOnActivation();
#endif

	protected:
		PrimitiveType mCurrentPrimitiveType = PrimitiveType::Polygon; // Type of drawing primitives for next DIP

		TextureRef mCurrentDrawTexture = nullptr; // Stored texture ptr from last DIP
		VertexType mCurrentBatchVertexType;       // Vertex type of the current batch
		UInt       mLastDrawVertex = 0;           // Last vertex idx for next DIP
		UInt       mLastDrawIdx = 0;              // Last vertex index for next DIP
		UInt       mTrianglesCount = 0;           // Triangles count for next DIP
		UInt       mFrameTrianglesCount = 0;      // Total triangles at current frame
		UInt       mDrawCallsCount = 0;           // DrawIndexedPrimitives calls count

		UInt8*       mVertexData = nullptr;       // CPU-side vertex batch buffer
		UInt         mVertexBufferSize = 0;       // Max vertex count in batch buffer
		UInt         mVertexBufferByteSize = 0;   // Max byte size of vertex batch buffer
		VertexIndex* mVertexIndexData = nullptr;  // CPU-side index batch buffer
		int          mVertexBufferIdx = 0;        // Current vertex offset in batch (for GPU upload)
		UInt         mIndexBufferSize = 0;        // Max index count in batch buffer
		int          mIndexBufferIdx = 0;         // Current index offset in batch (for GPU upload)

		Ref<Material> mDefaultMaterial; // Default material, loaded from Shaders/Default 
		Ref<Material> mCurrentMaterial; // Currently bound material

		Ref<LogStream> mLog; // Render log stream

		TextureRef mWhiteTexture; // Default white texture

		Vector<TextureRef> mTextures; // Loaded textures
		Vector<Ref<Font>>  mFonts;    // Loaded fonts

		Camera mCamera;     // Camera transformation
		Camera mPrevCamera; // Previous camera transformation

		bool mPrevTransformsToTarget = false; // Was a render target bound when camera transforms were computed

		Vec2I  mResolution;        // Primary back buffer size
		Vec2I  mCurrentResolution; // Current back buffer size
		Vec2I  mPrevResolution;    // Previous back buffer size

		Vec2F  mViewScale;         // Current view scale, depends on camera
		Vec2F  mInvViewScale;      // Inverted mViewScale
		Vec2I  mDPI;               // Current device screen DPI

		Vector<ScissorInfo>       mScissorInfos;               // Scissor clipping depth infos vector
		Vector<ScissorStackEntry> mStackScissors;              // Stack of scissors clippings
		bool                      mClippingEverything = false; // Is everything clipped

		TextureRef mCurrentRenderTarget; // Current render target. NULL if rendering in back buffer

		Vector<TextureRef> mExtraRenderTargets; // Additional MRT color targets, bound after the primary one

		Vector<TextureRef> mRenderTargetsStack; // Saved render targets for Push/PopRenderTargets pairs

		Ref<Material> mOverrideMaterial; // Material that overrides all draw call materials when set

		bool mMRTUnsupportedWarned = false; // One-time warning flag for MRT fallback

		bool mSkinnedMaterialMismatchWarned = false; // One-time warning flag for skinned layout draws with non-skinned materials

		Function<void(const Ref<Bitmap>&)> mCaptureCallback; // Pending frame capture callback (CaptureNextFrame)
		TextureRef                         mCaptureTarget;   // Offscreen target of the frame being captured

		bool mDepthTestEnabled = false; // Is depth buffer test enabled, resets on Begin

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

		// Platform specific: binds next GPU buffer from pool and configures vertex attributes
		void PlatformBindNextPoolBuffers();

		// Platform specific begin of rendering
		void PlatformBegin();

		// Platform specific end of rendering
		void PlatformEnd();

		// Checks whether a batch break is needed for the given draw state
		bool CheckBatchBreak(const TextureRef& texture, PrimitiveType primitiveType,
							 const Ref<Material>& material, const VertexType& batchVertexType,
							 UInt verticesCount, UInt indexesCount) const;

		// Resolves a batch vertex layout using the default material-driven texcoord expansion rules
		VertexType ResolveBatchVertexTypeByMaterial(const VertexType& sourceVertexType,
											        const Ref<Material>& material) const;

		// Initializes default material (calls platform to load shaders and set mDefaultMaterial, mStdShader*)
		void InitializeDefaultMaterial();

		// Platform: load default shader files, compile, create default material, set mDefaultMaterial and mStdShader*
		void PlatformInitializeDefaultMaterial();

		// Platform specific material binding
		void PlatformBindMaterial(const Ref<Material>& material);

		// Platform specific batch vertex layout selection
		VertexType PlatformResolveBatchVertexType(const VertexType& sourceVertexType,
										          const Ref<Material>& material) const;

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

		// Reads the finished capture target into a bitmap and fires the capture callback
		void DeliverFrameCapture();

		// Platform specific setup camera transforms
		void PlatformSetupCameraTransforms(float* modelMatrix, float* viewMatrix, float* projMatrix);

		// Platform specific enable or disable depth buffer test
		void PlatformSetDepthTest(bool enabled);

		// Platform specific enable scissor test
		void PlatformEnableScissorTest();

		// Platform specific disable scissor test
		void PlatformDisableScissorTest();

		// Platform specific set scissor rect
		void PlatformSetScissorRect(const RectI& rect);

		// Platform specific bind render target
		void PlatformBindRenderTarget(const TextureRef& renderTarget);

		// Platform specific multiple render targets support check
		bool PlatformSupportsMRT() const;

		// Platform specific sync of extra MRT attachments to the bound render target (used by GL backends)
		void PlatformSyncRenderTargetAttachments();

		// Restores platform scissor state and clipping flag from the scissors stack top
		void RestoreScissorStateFromStack();

		// Remaps UV coordinates from 0..1 range to texture source rect
		static void RemapUV(float srcU, float srcV, const RectI& srcRect,
							const Vec2F& invTexSize, float& outU, float& outV);

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
		friend class Integration;
		friend class Material;
		friend class RenderBase;
		friend class Shader;
		friend class Sprite;
		friend class Texture;
		friend class TextureRef;
		friend class VectorFont;
		friend class VectorFontAsset;
		friend class WndProcFunc;
		friend struct ApplicationPlatformWrapper;
	};
}
