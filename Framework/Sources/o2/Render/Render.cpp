#include "o2/stdafx.h"
#include "o2/Render/Render.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Integration.h"
#include "o2/Render/Font.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Shader.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Interpolation.h"

namespace o2
{
	DECLARE_SINGLETON(Render);

	FORWARD_REF_IMPL(AtlasAsset);

	Render::Render(RefCounter* refCounter):
		Singleton<Render>(refCounter)
	{
		mLog = mmake<LogStream>("Render");
		o2Debug.GetLog()->BindStream(mLog);

		InitializePlatform();

		mResolution = o2Integration.GetContentSize();
		mMaxTextureSize = GetPlatformMaxTextureSize();
		mDPI = GetPlatformDPI();

		InitializeDefaultMaterial();
		InitializeWhiteTexture();
		InitializeFreeType();
		InitializeLinesIndexBuffer();
		InitializeLinesTextures();

		if (IsDevMode())
			o2Assets.onAssetsRebuilt += MakeFunction(this, &Render::OnAssetsRebuilt);

		mReady = true;
	}

	Render::~Render()
	{
		if (!mReady)
			return;

		mSolidLineTexture = nullptr;
		mDashLineTexture = nullptr;

		mCurrentMaterial = nullptr;
		mDefaultMaterial = nullptr;

		DeinitializePlatform();

		mFonts.Clear();
		mTextures.Clear();

		DeinitializeFreeType();

		mReady = false;
	}

	void Render::InitializeWhiteTexture()
	{
		Bitmap whiteBitmap(PixelFormat::R8G8B8A8, Vec2I(16, 16));
		whiteBitmap.Fill(Color4::White());
		mWhiteTexture = TextureRef(whiteBitmap);
	}

	void Render::InitializeLinesTextures()
	{
		mSolidLineTexture = TextureRef::Null();

		Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(32, 32));
		bitmap.Fill(Color4(255, 255, 255, 255));
		bitmap.FillRect(0, 32, 16, 0, Color4(255, 255, 255, 0));
		mDashLineTexture = mmake<Texture>(bitmap);
	}

	void Render::InitializeLinesIndexBuffer()
	{
		mHardLinesIndexData = mnew VertexIndex[USHRT_MAX];

		for (UInt i = 0; i < USHRT_MAX / 2; i++)
		{
			mHardLinesIndexData[i * 2] = i;
			mHardLinesIndexData[i * 2 + 1] = i + 1;
		}
	}

	void Render::InitializeFreeType()
	{
		FT_Error error = FT_Init_FreeType(&mFreeTypeLib);
		if (error)
			mLog->Out("Failed to initialize FreeType: %i", error);
	}

	void Render::DeinitializeFreeType()
	{
		FT_Done_FreeType(mFreeTypeLib);
	}

	void Render::InitializeDefaultMaterial()
	{
		PlatformInitializeDefaultMaterial();
	}

	void Render::Begin()
	{
		PROFILE_SAMPLE_FUNC();

		if (!mReady)
			return;

		mCurrentDrawTexture = nullptr;
		mLastDrawVertex = 0;
		mLastDrawIdx = 0;
		mTrianglesCount = 0;
		mFrameTrianglesCount = 0;
		mDrawCallsCount = 0;
		mCurrentPrimitiveType = PrimitiveType::Polygon;
		mCurrentBatchVertexType = Vertex::Type();
		mDrawingDepth = 0.0f;
		mClippingEverything = false;

		mScissorInfos.Clear();
		mStackScissors.Clear();

		BindMaterial(mDefaultMaterial);

		PlatformBegin();
		SetupViewMatrix(mResolution);
		UpdateCameraTransforms();

		preRender();

		if (IsRenderDrawCallsDebugEnabled())
			mLog->OutStr("==== New Frame ====");
	}

	void Render::DrawBuffer(PrimitiveType primitiveType, const Vertex* vertices, UInt verticesCount,
							VertexIndex* indexes, UInt elementsCount,
							const Ref<Material>& material, const TextureRef& overrideTexture, const RectI& texSrcRect /*= RectI()*/,
							bool allowVertexConversion /*= false*/)
	{
		DrawBuffer(primitiveType, reinterpret_cast<const UInt8*>(vertices), verticesCount,
				   Vertex::Type(), indexes, elementsCount, material, overrideTexture,
				   texSrcRect, allowVertexConversion);
	}

	void Render::DrawBuffer(PrimitiveType primitiveType, const UInt8* vertices, UInt verticesCount, const VertexType& vertexType,
							VertexIndex* indexes, UInt elementsCount,
							const Ref<Material>& material, const TextureRef& overrideTexture, const RectI& texSrcRect /*= RectI()*/,
							bool allowVertexConversion /*= false*/)
	{
		if (!mReady)
			return;

		mDrawingDepth += 1.0f;

		if (mClippingEverything)
			return;

		UInt indexesCount;
		if (primitiveType == PrimitiveType::Line)
			indexesCount = elementsCount * 2;
		else
			indexesCount = elementsCount * 3;

		Ref<Material> drawMaterial = material ? material : mDefaultMaterial;
		TextureRef texture = overrideTexture ? overrideTexture : drawMaterial->GetTexture();

		// Determine batch vertex type: expand with extra texcoords if material needs them
		VertexType batchVertexType = vertexType;
		int materialTexChannels = drawMaterial ? drawMaterial->GetTotalTextureChannelsCount() : 1;
		if (materialTexChannels > 1 && !batchVertexType.HasParam(VertexParam::TexCoord1))
			batchVertexType = Vertex2Tex::Type();
		if (materialTexChannels > 2 && !batchVertexType.HasParam(VertexParam::TexCoord2))
			batchVertexType = Vertex3Tex::Type();

		if (CheckBatchBreak(texture, primitiveType, drawMaterial, batchVertexType, verticesCount, indexesCount))
		{
			DrawPrimitives();

			mCurrentDrawTexture = texture;
			mCurrentPrimitiveType = primitiveType;
			mCurrentBatchVertexType = batchVertexType;

			BindMaterial(drawMaterial);
		}

		UploadBuffers(vertices, verticesCount, vertexType, indexes, indexesCount, texSrcRect, texture, allowVertexConversion);

		mLastDrawVertex += verticesCount;
		mLastDrawIdx += indexesCount;

		if (primitiveType != PrimitiveType::Line)
			mTrianglesCount += elementsCount;
	}

	bool Render::CheckBatchBreak(const TextureRef& texture, PrimitiveType primitiveType,
								 const Ref<Material>& material, const VertexType& batchVertexType,
								 UInt verticesCount, UInt indexesCount) const
	{
		size_t materialHash = material ? material->GetHash() : 0;
		size_t currentBatchMaterialHash = mCurrentMaterial ? mCurrentMaterial->GetHash() : 0;

		size_t batchStride = batchVertexType.GetStride();
		UInt effectiveVertexCapacity = (UInt)(mVertexBufferByteSize / batchStride);

		return mCurrentDrawTexture != texture ||
			mCurrentPrimitiveType != primitiveType ||
			mCurrentBatchVertexType != batchVertexType ||
			currentBatchMaterialHash != materialHash ||
			mLastDrawVertex + verticesCount >= effectiveVertexCapacity ||
			mLastDrawIdx + indexesCount >= mIndexBufferSize;
	}

	void Render::UploadBuffers(const UInt8* vertices, UInt verticesCount, const VertexType& srcVertexType,
							   VertexIndex* indexes, UInt indexesCount, const RectI& texSrcRect,
							   const TextureRef& texture, bool allowVertexConversion)
	{
		size_t srcStride = srcVertexType.GetStride();
		size_t dstStride = mCurrentBatchVertexType.GetStride();
		bool needsUVRemap = (texSrcRect != RectI()) && texture;

		int materialTexChannels = mCurrentMaterial ? mCurrentMaterial->GetTotalTextureChannelsCount() : 1;
		int srcTexCoords = srcVertexType.GetTexCoordsCount();
		bool needsConversion = srcTexCoords < materialTexChannels;

		if (needsConversion)
		{
			Assert(allowVertexConversion, "Vertex type has fewer texcoord sets than material requires, and conversion is not allowed");
		}

		if (srcVertexType == mCurrentBatchVertexType && !needsUVRemap)
		{
			memcpy(&mVertexData[mLastDrawVertex * dstStride], vertices, srcStride * verticesCount);
		}
		else
		{
			bool srcHasUV = srcVertexType.HasParam(VertexParam::TexCoord0);
			size_t srcUVOffset = srcHasUV ? srcVertexType.GetParamOffset(VertexParam::TexCoord0) : 0;

			Vec2F primaryInvTexSize(1.0f, 1.0f);
			if (texture)
			{
				Vec2F texSize = texture->GetSize();
				primaryInvTexSize.Set(1.0f / texSize.x, 1.0f / texSize.y);
			}

			static const Vector<TextureSampler> emptySamplers;
			const auto& samplers = mCurrentMaterial ? mCurrentMaterial->GetTextureSamplers() : emptySamplers;

			UInt8* dst = &mVertexData[mLastDrawVertex * dstStride];

			for (UInt i = 0; i < verticesCount; i++)
			{
				const UInt8* srcVtx = vertices + i * srcStride;
				UInt8* dstVtx = dst + i * dstStride;

				if (mCurrentBatchVertexType.HasParam(VertexParam::Position) && srcVertexType.HasParam(VertexParam::Position))
				{
					memcpy(dstVtx + mCurrentBatchVertexType.GetParamOffset(VertexParam::Position),
						   srcVtx + srcVertexType.GetParamOffset(VertexParam::Position),
						   VertexParam::ParamSize(VertexParam::Position));
				}

				if (mCurrentBatchVertexType.HasParam(VertexParam::Color) && srcVertexType.HasParam(VertexParam::Color))
				{
					memcpy(dstVtx + mCurrentBatchVertexType.GetParamOffset(VertexParam::Color),
						   srcVtx + srcVertexType.GetParamOffset(VertexParam::Color),
						   VertexParam::ParamSize(VertexParam::Color));
				}

				float srcU = 0.0f, srcV = 0.0f;
				if (srcHasUV)
				{
					srcU = *(const float*)(srcVtx + srcUVOffset);
					srcV = *(const float*)(srcVtx + srcUVOffset + sizeof(float));
				}

				// TexCoord0: remap through primary texture srcRect
				if (mCurrentBatchVertexType.HasParam(VertexParam::TexCoord0))
				{
					float* dstUV = (float*)(dstVtx + mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0));
					if (needsUVRemap)
						RemapUV(srcU, srcV, texSrcRect, primaryInvTexSize, dstUV[0], dstUV[1]);
					else
					{
						dstUV[0] = srcU;
						dstUV[1] = srcV;
					}
				}

				// TexCoord1, TexCoord2: remap through additional sampler srcRects
				const UInt texCoordParams[] = { VertexParam::TexCoord1, VertexParam::TexCoord2 };
				for (int s = 0; s < samplers.Count() && s < 2; s++)
				{
					if (!mCurrentBatchVertexType.HasParam(texCoordParams[s]))
						continue;

					float* dstUV = (float*)(dstVtx + mCurrentBatchVertexType.GetParamOffset(texCoordParams[s]));

					RectI samplerSrcRect = samplers[s].GetSrcRect();
					if (samplerSrcRect != RectI())
					{
						TextureRef samplerTex = samplers[s].GetTexture();
						Vec2F samplerInvTexSize(1.0f, 1.0f);
						if (samplerTex)
						{
							Vec2F sz = samplerTex->GetSize();
							samplerInvTexSize.Set(1.0f / sz.x, 1.0f / sz.y);
						}
						RemapUV(srcU, srcV, samplerSrcRect, samplerInvTexSize, dstUV[0], dstUV[1]);
					}
					else
					{
						dstUV[0] = srcU;
						dstUV[1] = srcV;
					}
				}

				if (mCurrentBatchVertexType.HasParam(VertexParam::Normal) && srcVertexType.HasParam(VertexParam::Normal))
				{
					memcpy(dstVtx + mCurrentBatchVertexType.GetParamOffset(VertexParam::Normal),
						   srcVtx + srcVertexType.GetParamOffset(VertexParam::Normal),
						   VertexParam::ParamSize(VertexParam::Normal));
				}
			}
		}

		for (UInt i = mLastDrawIdx, j = 0; j < indexesCount; i++, j++)
			mVertexIndexData[i] = mVertexBufferIdx + mLastDrawVertex + indexes[j];
	}

	void Render::RemapUV(float srcU, float srcV, const RectI& srcRect,
						 const Vec2F& invTexSize, float& outU, float& outV)
	{
		float u0 = srcRect.left * invTexSize.x;
		float v0 = srcRect.top * invTexSize.y;
		float u1 = srcRect.right * invTexSize.x;
		float v1 = srcRect.bottom * invTexSize.y;

		outU = u0 + srcU * (u1 - u0);
		outV = 1.0f - (v1 - srcV * (v1 - v0));
	}

	void Render::DrawPrimitives()
	{
		PROFILE_SAMPLE_FUNC();

		if (mLastDrawVertex < 1)
			return;

		CheckVertexBufferTexCoordFlipByTextureFormat();

		PlatformDrawPrimitives();

		mFrameTrianglesCount += mTrianglesCount;
		mLastDrawVertex = mTrianglesCount = mLastDrawIdx = 0;

		mDrawCallsCount++;

		if (IsRenderDrawCallsDebugEnabled())
			mLog->OutStr("#DC " + (String)mDrawCallsCount + "; with texture\"" + mCurrentDrawTexture->GetFileName() + "\"");
	}

	void Render::CheckVertexBufferTexCoordFlipByTextureFormat()
	{
		PROFILE_SAMPLE_FUNC();

		static const Vector<TextureFormat> flipFormats = { TextureFormat::DXT5 };

		if (mCurrentDrawTexture && flipFormats.Contains(mCurrentDrawTexture->GetFormat()))
			PlatformFlipVerticesUV();
	}

	void Render::SetupViewMatrix(const Vec2I& viewSize)
	{
		mCurrentResolution = viewSize;
		UpdateCameraTransforms();
	}

	void Render::End()
	{
		PROFILE_SAMPLE_FUNC();

		if (!mReady)
			return;

		postRender();
		postRender.Clear();

		DrawPrimitives();

		PlatformEnd();

		CheckTexturesUnloading();
		CheckFontsUnloading();
	}

	void Render::BindMaterial(const Ref<Material>& material)
	{
		if (!material)
			return;

		if (!material->IsReady())
			material->Build();

		if (!material->IsReady())
			return;

		if (material == mCurrentMaterial)
			return;

		mCurrentMaterial = material;

		PlatformBindMaterial(material);
	}

	const Ref<Material>& Render::GetCurrentMaterial() const
	{
		return mCurrentMaterial;
	}

	const Ref<Material>& Render::GetDefaultMaterial() const
	{
		return mDefaultMaterial;
	}

	void Render::BeginCustomRender()
	{
		DrawPrimitives();
	}

	void Render::ResetState()
	{
		PROFILE_SAMPLE_FUNC();

		mCurrentDrawTexture = nullptr;
		mCurrentBatchVertexType = Vertex::Type();

		PlatformResetState();
		SetupViewMatrix(mResolution);
		UpdateCameraTransforms();
	}

	void Render::EndCustomRender()
	{
		ResetState();
	}

	void Render::UpdateCameraTransforms()
	{
		PROFILE_SAMPLE_FUNC();

		if (mCurrentResolution == mPrevResolution && mCamera == mPrevCamera)
			return;

		DrawPrimitives();

		Vec2F resf = (Vec2F)mCurrentResolution;
		Vec2F halfRes(Math::Round(resf.x / 2.0f), Math::Round(resf.y / 2.0f));

		float projMatrix[16];
		Math::OrthoProjMatrix(projMatrix, 0.0f, (float)mCurrentResolution.x, (float)mCurrentResolution.y, 0.0f, 0.0f, 10.0f);

		float modelMatrix[16] =
		{
			1,         0,          0, 0,
			0,        -1,          0, 0,
			0,         0,          1, 0,
			halfRes.x, halfRes.y, -1, 1
		};

		Basis defaultCameraBasis((Vec2F)mCurrentResolution * -0.5f, Vec2F::Right() * resf.x, Vec2F().Up() * resf.y);
		Basis camTransf = mCamera.GetBasis().Inverted() * defaultCameraBasis;
		mViewScale = Vec2F(camTransf.xv.Length(), camTransf.yv.Length());
		mInvViewScale = Vec2F(1.0f / mViewScale.x, 1.0f / mViewScale.y);

		float viewMatrix[16] =
		{
			camTransf.xv.x,     camTransf.xv.y,     0, 0,
			camTransf.yv.x,     camTransf.yv.y,     0, 0,
			0,                  0,                  0, 0,
			camTransf.origin.x, camTransf.origin.y, 0, 1
		};

		PlatformSetupCameraTransforms(modelMatrix, viewMatrix, projMatrix);

		mPrevCamera = mCamera;
		mPrevResolution = mCurrentResolution;
	}

	void Render::EnableScissorTest(const RectI& rect)
	{
		float scale = mCurrentRenderTarget ? 1.0f : o2Integration.GetGraphicsScale();
		Vec2I resolution = Vec2I(Vec2F(mCurrentResolution) * scale);
		RectI invRect(rect.left * 2, -rect.top * 2, rect.right * 2, -rect.bottom * 2);

		DrawPrimitives();

		RectI summaryScissorRect = rect;
		if (!mStackScissors.IsEmpty())
		{
			mScissorInfos.Last().endDepth = mDrawingDepth;

			if (!mStackScissors.Last().renderTarget)
			{
				RectI lastSummaryClipRect = mStackScissors.Last().summaryScissorRect;
				mClippingEverything = !summaryScissorRect.IsIntersects(lastSummaryClipRect);
				summaryScissorRect = summaryScissorRect.GetIntersection(lastSummaryClipRect);
			}
			else
			{
				PlatformEnableScissorTest();
				mClippingEverything = false;
			}
		}
		else
		{
			PlatformEnableScissorTest();
			mClippingEverything = false;
		}

		mScissorInfos.Add(ScissorInfo(summaryScissorRect, mDrawingDepth));
		mStackScissors.Add(ScissorStackEntry(rect, summaryScissorRect));

		RectI screenScissorRect = CalculateScreenSpaceScissorRect(summaryScissorRect);
		PlatformSetScissorRect(screenScissorRect);
	}

	void Render::DisableScissorTest(bool forcible /*= false*/)
	{
		if (mStackScissors.IsEmpty())
		{
			mLog->WarningStr("Can't disable scissor test - no scissor were enabled!");
			return;
		}

		DrawPrimitives();

		if (forcible)
		{
			PlatformDisableScissorTest();

			while (!mStackScissors.IsEmpty() && !mStackScissors.Last().renderTarget)
				mStackScissors.PopBack();

			mScissorInfos.Last().endDepth = mDrawingDepth;
		}
		else
		{
			if (mStackScissors.Count() == 1)
			{
				PlatformDisableScissorTest();
				mStackScissors.PopBack();

				mScissorInfos.Last().endDepth = mDrawingDepth;
				mClippingEverything = false;
			}
			else
			{
				mStackScissors.PopBack();
				RectI lastClipRect = mStackScissors.Last().summaryScissorRect;

				mScissorInfos.Last().endDepth = mDrawingDepth;
				mScissorInfos.Add(ScissorInfo(lastClipRect, mDrawingDepth));

				if (mStackScissors.Last().renderTarget)
				{
					PlatformDisableScissorTest();
					mClippingEverything = false;
				}
				else
				{
					RectI screenScissorRect = CalculateScreenSpaceScissorRect(lastClipRect);
					PlatformSetScissorRect(screenScissorRect);

					mClippingEverything = lastClipRect == RectI();
				}
			}
		}
	}

	void Render::BindRenderTexture(TextureRef renderTarget)
	{
		if (!renderTarget)
		{
			UnbindRenderTexture();
			return;
		}

		if (renderTarget->mUsage != Texture::Usage::RenderTarget)
		{
			mLog->Error("Can't set texture as render target: not render target texture");
			UnbindRenderTexture();
			return;
		}

		if (!renderTarget->IsReady())
		{
			mLog->Error("Can't set texture as render target: texture isn't ready");
			UnbindRenderTexture();
			return;
		}

		DrawPrimitives();

		if (!mStackScissors.IsEmpty())
		{
			mScissorInfos.Last().endDepth = mDrawingDepth;
			PlatformDisableScissorTest();
		}

		mStackScissors.Add(ScissorStackEntry(RectI(), RectI(), true));

		mCurrentRenderTarget = renderTarget;

		PlatformBindRenderTarget(renderTarget);
		SetupViewMatrix(renderTarget->GetSize());
	}

	void Render::UnbindRenderTexture()
	{
		if (!mCurrentRenderTarget)
			return;

		DrawPrimitives();
		PlatformBindRenderTarget(nullptr);

		mCurrentRenderTarget = TextureRef();

		SetupViewMatrix(mResolution);
		SetCamera(Camera());

		DisableScissorTest(true);
		mStackScissors.PopBack();
		if (!mStackScissors.IsEmpty())
		{
			auto clipRect = mStackScissors.Last().summaryScissorRect;

			PlatformEnableScissorTest();
			PlatformSetScissorRect(clipRect);

			mClippingEverything = clipRect == RectI();
		}
	}

	void Render::OnFrameResized()
	{
		mResolution = o2Integration.GetContentSize();
	}

	Vec2I Render::GetResolution() const
	{
		return mResolution;
	}

	Vec2I Render::GetCurrentResolution() const
	{
		return mCurrentResolution;
	}

	Vec2I Render::GetDPI() const
	{
		return mDPI;
	}

	int Render::GetDrawCallsCount() const
	{
		return mDrawCallsCount;
	}

	int Render::GetDrawnPrimitives() const
	{
		return mFrameTrianglesCount;
	}

	void Render::SetCamera(const Camera& camera)
	{
		DrawPrimitives();
		mCamera = camera;
		UpdateCameraTransforms();
	}

	Camera Render::GetCamera() const
	{
		return mCamera;
	}

	void Render::DrawFilledPolygon(const Vertex* verticies, int vertexCount)
	{
		static Mesh mesh(TextureRef(), 1024, 1024);

		int polyCount = vertexCount - 2;
		if (mesh.GetMaxVertexCount() < (UInt)vertexCount || mesh.GetMaxPolyCount() < (UInt)polyCount)
			mesh.Resize(vertexCount, polyCount);

		Vertex* meshVerts = mesh.GetVertices<Vertex>();
		memcpy(meshVerts, verticies, sizeof(Vertex) * vertexCount);

		for (int i = 2; i < vertexCount; i++)
		{
			int ii = (i - 2) * 3;
			mesh.mIndexData[ii] = i - 1;
			mesh.mIndexData[ii + 1] = i;
			mesh.mIndexData[ii + 2] = 0;
		}

		mesh.vertexCount = vertexCount;
		mesh.polyCount = polyCount;
		mesh.Draw();
	}

	void Render::DrawFilledPolygon(const Vector<Vec2F>& points, const Color4& color /*= Color4::White()*/)
	{
		static Mesh mesh(TextureRef(), 1024, 1024);

		int vertexCount = points.Count();
		int polyCount = points.Count() - 2;
		if (mesh.GetMaxVertexCount() < points.Count() || mesh.GetMaxPolyCount() < polyCount)
			mesh.Resize(points.Count(), polyCount);

		ULong dcolor = color.ABGR();
		Vertex* meshVerts = mesh.GetVertices<Vertex>();
		for (int i = 0; i < points.Count(); i++)
			meshVerts[i] = Vertex(points[i], dcolor, 0.0f, 0.0f);

		for (int i = 2; i < points.Count(); i++)
		{
			int ii = (i - 2) * 3;
			mesh.mIndexData[ii] = i - 1;
			mesh.mIndexData[ii + 1] = i;
			mesh.mIndexData[ii + 2] = 0;
		}

		mesh.vertexCount = vertexCount;
		mesh.polyCount = polyCount;
		mesh.Draw();
	}

	RectI Render::CalculateScreenSpaceScissorRect(const RectF& cameraSpaceScissorRect) const
	{
		float scale = mCurrentRenderTarget ? o2Integration.GetGraphicsScale() : 1.0f;
		Vec2I resolution = Vec2I(Vec2F(mCurrentResolution) * scale);

		Basis defaultCameraBasis((Vec2F)resolution * -0.5f, Vec2F((float)resolution.x, 0.0f), Vec2F(0.0f, (float)resolution.y));
		Basis camTransf = mCamera.GetBasis().Inverted() * defaultCameraBasis;
		Basis scissorBasis(cameraSpaceScissorRect.LeftBottom(), Vec2F(cameraSpaceScissorRect.Width(), 0.0f), Vec2F(0.0f, cameraSpaceScissorRect.Height()));
		Basis screenScissorBasis = scissorBasis * camTransf;
		RectI screenScissorRect = screenScissorBasis.AABB();

		return screenScissorRect;
	}

	void Render::CheckTexturesUnloading()
	{}

	void Render::CheckFontsUnloading()
	{}

	void Render::OnAssetsRebuilt(const Vector<UID>& changedAssets)
	{
		for (auto& tex : mTextures)
			tex->Reload();

		for (auto& atlas : mAtlases)
			atlas->ReloadPages();

		for (auto& spr : mSprites)
			spr->ReloadImage();
	}

	void Render::OnSpriteCreated(Sprite* sprite)
	{
		if (!IsSingletonInitialzed())
			return;

		Instance().mSprites.Add(sprite);
	}

	void Render::OnSpriteDestroyed(Sprite* sprite)
	{
		if (!IsSingletonInitialzed())
			return;

		Instance().mSprites.Remove(sprite);
	}

	void Render::OnTextureCreated(Texture* texture)
	{
		mTextures.Add(Ref(texture));
	}

	void Render::OnTextureDestroyed(Texture* texture)
	{
		mTextures.RemoveFirst([=](const TextureRef& x) { return x == texture; });
	}

	void Render::OnAtlasCreated(AtlasAsset* atlas)
	{
		mAtlases.Add(Ref(atlas));
	}

	void Render::OnAtlasDestroyed(AtlasAsset* atlas)
	{
		mAtlases.RemoveFirst([=](const AssetRef<AtlasAsset>& x) { return x == atlas; });
	}

	void Render::OnFontCreated(Font* font)
	{
		mFonts.Add(Ref(font));
	}

	void Render::OnFontDestroyed(Font* font)
	{
		mFonts.RemoveFirst([=](const Ref<Font>& x) { return x == font; });
	}

	void Render::DrawAALine(const Vec2F& a, const Vec2F& b, const Color4& color /*= Color4::White()*/,
							float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
							bool scaleToScreenSpace /*= true*/)
	{
		ULong dcolor = color.ABGR();
		Vertex v[] = { Vertex(a.x, a.y, dcolor, 0, 0), Vertex(b.x, b.y, dcolor, 0, 0) };
		DrawAAPolyLine(v, 2, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAALine(const Vector<Vec2F>& points, const Color4& color /*= Color4::White()*/,
							float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
							bool scaleToScreenSpace /*= true*/)
	{
		ULong dcolor = color.ABGR();
		Vertex* v = mnew Vertex[points.Count()];
		for (int i = 0; i < points.Count(); i++)
			v[i] = Vertex(points[i], dcolor, 0, 0);

		DrawAAPolyLine(v, points.Count(), width, lineType, scaleToScreenSpace);
		delete[] v;
	}

	void Render::DrawAAArrow(const Vec2F& a, const Vec2F& b, const Color4& color /*= Color4::White()*/,
							 const Vec2F& arrowSize /*= Vec2F(10, 10)*/,
							 float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
							 bool scaleToScreenSpace /*= true*/)
	{
		ULong dcolor = color.ABGR();
		Vec2F dir = (b - a).Normalized();
		Vec2F ndir = dir.Perpendicular();

		Vertex v[] = {
			Vertex(a, dcolor, 0, 0), Vertex(b, dcolor, 0, 0),
			Vertex(b - dir * arrowSize.x + ndir * arrowSize.y, dcolor, 0, 0), Vertex(b, dcolor, 0, 0),
			Vertex(b - dir * arrowSize.x - ndir * arrowSize.y, dcolor, 0, 0), Vertex(b, dcolor, 0, 0) };

		DrawAAPolyLine(v, 6, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAARectFrame(const Vec2F& minp, const Vec2F& maxp, const Color4& color /*= Color4::White()*/,
								 float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
								 bool scaleToScreenSpace /*= true*/)
	{
		ULong dcolor = color.ABGR();
		Vertex v[] = {
			Vertex(minp.x, minp.y, dcolor, 0, 0),
			Vertex(maxp.x, minp.y, dcolor, 0, 0),
			Vertex(maxp.x, maxp.y, dcolor, 0, 0),
			Vertex(minp.x, maxp.y, dcolor, 0, 0),
			Vertex(minp.x, minp.y, dcolor, 0, 0)
		};

		DrawAAPolyLine(v, 5, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAARectFrame(const RectF& rect, const Color4& color /*= Color4::White()*/,
								 float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
								 bool scaleToScreenSpace /*= true*/)
	{
		DrawAARectFrame(rect.LeftBottom(), rect.RightTop(), color, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAABasis(const Basis& basis, const Color4& xcolor /*= Color4::Red()*/,
							 const Color4& ycolor /*= Color4::Blue()*/, const Color4& color /*= Color4::White()*/,
							 float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
							 bool scaleToScreenSpace /*= true*/)
	{
		DrawAALine(basis.origin, basis.origin + basis.xv, xcolor, width, lineType, scaleToScreenSpace);
		DrawAALine(basis.origin, basis.origin + basis.yv, ycolor, width, lineType, scaleToScreenSpace);

		Vertex v[] =
		{
			Vertex(basis.origin + basis.xv, color.ABGR(), 0, 0),
			Vertex(basis.origin + basis.yv + basis.xv, color.ABGR(), 0, 0),
			Vertex(basis.origin + basis.yv, color.ABGR(), 0, 0)
		};

		DrawAAPolyLine(v, 3, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAACross(const Vec2F& pos, float size /*= 5*/, const Color4& color /*= Color4::White()*/,
							 float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
							 bool scaleToScreenSpace /*= true*/)
	{
		DrawAALine(Vec2F(pos.x - size, pos.y), Vec2F(pos.x + size, pos.y), color, width, lineType, scaleToScreenSpace);
		DrawAALine(Vec2F(pos.x, pos.y - size), Vec2F(pos.x, pos.y + size), color, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAACircle(const Vec2F& pos, float radius /*= 5*/, const Color4& color /*= Color4::White()*/,
							  int segCount /*= 20*/,
							  float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
							  bool scaleToScreenSpace /*= true*/)
	{
		Vertex* v = mnew Vertex[segCount + 1];
		ULong dcolor = color.ABGR();

		float angleSeg = 2.0f * Math::PI() / (float)(segCount - 1);
		for (int i = 0; i < segCount + 1; i++)
		{
			float a = (float)i * angleSeg;
			v[i] = Vertex(Vec2F::Rotated(a) * radius + pos, dcolor, 0, 0);
		}

		DrawAAPolyLine(v, segCount + 1, width, lineType, scaleToScreenSpace);
		delete[] v;
	}

	void Render::DrawAABezierCurve(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
								   const Color4& color /*= Color4::White()*/,
								   float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
								   bool scaleToScreenSpace /*= true*/)
	{
		const int segCount = 20;
		Vertex v[segCount + 1];
		ULong dcolor = color.ABGR();

		for (int i = 0; i < segCount + 1; i++)
		{
			float coef = (float)i / (float)segCount;
			Vec2F p = Bezier(p1, p2, p3, p4, coef);
			v[i] = Vertex(p, dcolor, 0, 0);
		}

		DrawAAPolyLine(v, segCount + 1, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawAABezierCurveArrow(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
										const Color4& color /*= Color4::White()*/, const Vec2F& arrowSize /*= Vec2F(10, 10)*/,
										float width /*= 1.0f*/, LineType lineType /*= LineType::Solid*/,
										bool scaleToScreenSpace /*= true*/)
	{
		const int segCount = 20;
		Vertex v[segCount + 1];
		ULong dcolor = color.ABGR();

		Vec2F lastp = p1;
		Vec2F dir;
		for (int i = 0; i < segCount + 1; i++)
		{
			float coef = (float)i / (float)segCount;
			Vec2F p = Bezier(p1, p2, p3, p4, coef);
			v[i] = Vertex(p, dcolor, 0, 0);
			dir = p - lastp;
			lastp = p;
		}

		DrawAAPolyLine(v, segCount + 1, width, lineType, scaleToScreenSpace);

		dir.Normalize();
		Vec2F ndir = dir.Perpendicular();

		Vertex va[] =
		{
			Vertex(p4 - dir * arrowSize.x + ndir * arrowSize.y, dcolor, 0, 0),
			Vertex(p4, dcolor, 0, 0),
			Vertex(p4 - dir * arrowSize.x - ndir * arrowSize.y, dcolor, 0, 0)
		};
		DrawAAPolyLine(va, 3, width, lineType, scaleToScreenSpace);
	}

	void Render::DrawLine(const Vec2F& a, const Vec2F& b, const Color4& color /*= Color4::White()*/)
	{
		ULong dcolor = color.ABGR();
		Vertex v[] = { Vertex(a.x, a.y, dcolor, 0, 0), Vertex(b.x, b.y, dcolor, 0, 0) };
		DrawPolyLine(v, 2);
	}

	void Render::DrawLine(const Vector<Vec2F>& points, const Color4& color /*= Color4::White()*/)
	{
		ULong dcolor = color.ABGR();
		Vertex* v = mnew Vertex[points.Count()];
		for (int i = 0; i < points.Count(); i++)
			v[i] = Vertex(points[i], dcolor, 0, 0);

		DrawPolyLine(v, points.Count());
		delete[] v;
	}

	void Render::DrawArrow(const Vec2F& a, const Vec2F& b, const Color4& color /*= Color4::White()*/,
						   const Vec2F& arrowSize /*= Vec2F(10, 10)*/)
	{
		ULong dcolor = color.ABGR();
		Vec2F dir = (b - a).Normalized();
		Vec2F ndir = dir.Perpendicular();

		Vertex v[] = {
			Vertex(a, dcolor, 0, 0), Vertex(b, dcolor, 0, 0),
			Vertex(b - dir * arrowSize.x + ndir * arrowSize.y, dcolor, 0, 0), Vertex(b, dcolor, 0, 0),
			Vertex(b - dir * arrowSize.x - ndir * arrowSize.y, dcolor, 0, 0), Vertex(b, dcolor, 0, 0) };

		DrawPolyLine(v, 6);
	}

	void Render::DrawRectFrame(const Vec2F& minp, const Vec2F& maxp, const Color4& color /*= Color4::White()*/)
	{
		ULong dcolor = color.ABGR();
		Vertex v[] = {
			Vertex(minp.x, minp.y, dcolor, 0, 0),
			Vertex(maxp.x, minp.y, dcolor, 0, 0),
			Vertex(maxp.x, maxp.y, dcolor, 0, 0),
			Vertex(minp.x, maxp.y, dcolor, 0, 0),
			Vertex(minp.x, minp.y, dcolor, 0, 0)
		};
		DrawPolyLine(v, 5);
	}

	void Render::DrawRectFrame(const RectF& rect, const Color4& color /*= Color4::White()*/)
	{
		DrawRectFrame(rect.LeftBottom(), rect.RightTop(), color);
	}

	void Render::DrawBasis(const Basis& basis, const Color4& xcolor /*= Color4::Red()*/,
						   const Color4& ycolor /*= Color4::Blue()*/, const Color4& color /*= Color4::White()*/)
	{
		DrawLine(basis.origin, basis.origin + basis.xv, xcolor);
		DrawLine(basis.origin, basis.origin + basis.yv, ycolor);

		Vertex v[] =
		{
			Vertex(basis.origin + basis.xv, color.ABGR(), 0, 0),
			Vertex(basis.origin + basis.yv + basis.xv, color.ABGR(), 0, 0),
			Vertex(basis.origin + basis.yv, color.ABGR(), 0, 0)
		};

		DrawPolyLine(v, 3);
	}

	void Render::DrawCross(const Vec2F& pos, float size /*= 5*/, const Color4& color /*= Color4::White()*/)
	{
		DrawLine(Vec2F(pos.x - size, pos.y), Vec2F(pos.x + size, pos.y), color);
		DrawLine(Vec2F(pos.x, pos.y - size), Vec2F(pos.x, pos.y + size), color);
	}

	void Render::DrawCircle(const Vec2F& pos, float radius /*= 5*/, const Color4& color /*= Color4::White()*/,
							int segCount /*= 20*/)
	{
		static int vertexBufferSize = segCount + 1;
		static Vertex* vertexBuffer = mnew Vertex[vertexBufferSize];

		int vertexCount = segCount + 1;
		if (vertexCount > vertexBufferSize)
		{
			delete[] vertexBuffer;
			vertexBufferSize = vertexCount;
			vertexBuffer = mnew Vertex[vertexBufferSize];
		}

		ULong dcolor = color.ABGR();

		float angleSeg = 2.0f * Math::PI() / (float)(segCount - 1);
		for (int i = 0; i < segCount + 1; i++)
		{
			float a = (float)i * angleSeg;
			vertexBuffer[i] = Vertex(Vec2F::Rotated(a) * radius + pos, dcolor, 0, 0);
		}

		DrawPolyLine(vertexBuffer, segCount + 1);
	}

	void Render::DrawFilledCircle(const Vec2F& pos, float radius /*= 5*/, const Color4& color /*= Color4::White()*/,
								  int segCount /*= 20*/)
	{
		static int vertexBufferSize = segCount + 1;
		static Vertex* vertexBuffer = mnew Vertex[vertexBufferSize];

		int vertexCount = segCount + 1;
		if (vertexCount > vertexBufferSize)
		{
			delete[] vertexBuffer;
			vertexBufferSize = vertexCount;
			vertexBuffer = mnew Vertex[vertexBufferSize];
		}

		ULong dcolor = color.ABGR();

		float angleSeg = 2.0f * Math::PI() / (float)(segCount - 1);
		for (int i = 0; i < segCount + 1; i++)
		{
			float a = (float)i * angleSeg;
			vertexBuffer[i] = Vertex(Vec2F::Rotated(a) * radius + pos, dcolor, 0, 0);
		}

		DrawFilledPolygon(vertexBuffer, vertexCount);
	}

	void Render::DrawBezierCurve(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
								 const Color4& color /*= Color4::White()*/)
	{
		const int segCount = 20;
		Vertex v[segCount + 1];
		ULong dcolor = color.ABGR();

		for (int i = 0; i < segCount + 1; i++)
		{
			float coef = (float)i / (float)segCount;
			Vec2F p = Bezier(p1, p2, p3, p4, coef);
			v[i] = Vertex(p, dcolor, 0, 0);
		}

		DrawPolyLine(v, segCount + 1);
	}

	void Render::DrawBezierCurveArrow(const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, const Vec2F& p4,
									  const Color4& color /*= Color4::White()*/, const Vec2F& arrowSize /*= Vec2F(10, 10)*/)
	{
		const int segCount = 20;
		Vertex v[segCount + 1];
		ULong dcolor = color.ABGR();

		Vec2F lastp = p1;
		Vec2F dir;
		for (int i = 0; i < segCount + 1; i++)
		{
			float coef = (float)i / (float)segCount;
			Vec2F p = Bezier(p1, p2, p3, p4, coef);
			v[i] = Vertex(p, dcolor, 0, 0);
			dir = p - lastp;
			lastp = p;
		}

		DrawPolyLine(v, segCount + 1);

		dir.Normalize();
		Vec2F ndir = dir.Perpendicular();

		Vertex va[] =
		{
			Vertex(p4 - dir * arrowSize.x + ndir * arrowSize.y, dcolor, 0, 0),
			Vertex(p4, dcolor, 0, 0),
			Vertex(p4 - dir * arrowSize.x - ndir * arrowSize.y, dcolor, 0, 0)
		};
		DrawPolyLine(va, 3);
	}

	RectI Render::GetScissorRect() const
	{
		if (mStackScissors.IsEmpty())
			return RectI(-(int)(mCurrentResolution.x * 0.5f), -(int)(mCurrentResolution.y * 0.5f),
						 (int)(mCurrentResolution.x * 0.5f), (int)(mCurrentResolution.y * 0.5f));

		return (RectI)(mStackScissors.Last().scissorRect);
	}

	RectI Render::GetResScissorRect() const
	{
		if (mStackScissors.IsEmpty() || mStackScissors.Last().renderTarget)
			return RectI(INT_MIN, INT_MIN, INT_MAX, INT_MAX);

		return (RectI)(mStackScissors.Last().summaryScissorRect);
	}

	const Vector<Render::ScissorStackEntry>& Render::GetScissorsStack() const
	{
		return mStackScissors;
	}

	bool Render::IsScissorTestEnabled() const
	{
		return !mStackScissors.IsEmpty();
	}

	bool Render::IsClippedByScissor(const RectF& rect) const
	{
		return !GetScissorRect().IsIntersects(rect);
	}

	bool Render::IsClippedByScissor(const Vec2F& point) const
	{
		return !GetScissorRect().IsInside(point);
	}

	void Render::DrawMesh(Mesh* mesh)
	{
		if (mesh->polyCount > 0)
		{
			DrawBuffer(PrimitiveType::Polygon, mesh->GetVertexData(), mesh->vertexCount,
					   mesh->mVertexType, mesh->mIndexData, mesh->polyCount,
					   mesh->GetMaterial(), mesh->mTexture, mesh->mTextureSrcRect, true);
		}
	}

	void Render::DrawMeshWire(Mesh* mesh, const Color4& color /*= Color4::White()*/)
	{
		Vertex* vertices = mesh->GetVertices<Vertex>();
		DrawMeshBufferWire(vertices, mesh->vertexCount, mesh->mIndexData, mesh->polyCount, color);
	}

	void Render::DrawMeshBufferWire(Vertex* vertices, UInt verticesCount, VertexIndex* indexes, UInt elementsCount,
									const Color4& color /*= Color4::White()*/)
	{
		auto dcolor = color.ABGR();

		for (UInt i = 0; i < elementsCount; i++)
		{
			Vertex v[] =
			{
				vertices[indexes[i * 3]],
				vertices[indexes[i * 3 + 1]],
				vertices[indexes[i * 3 + 2]],
				vertices[indexes[i * 3]]
			};

			v[0].color = dcolor;
			v[1].color = dcolor;
			v[2].color = dcolor;
			v[3].color = dcolor;

			DrawPolyLine(v, 4);
		}
	}

	void Render::DrawPolyLine(Vertex* vertices, int count, float width /*= 1.0f*/)
	{
		DrawBuffer(PrimitiveType::Line, vertices, count, mHardLinesIndexData, count - 1, mDefaultMaterial, mSolidLineTexture);
	}

	void Render::DrawAAPolyLine(Vertex* vertices, int count, float width /*= 1.0f*/,
								LineType lineType /*= LineType::Solid*/,
								bool scaleToScreenSpace /*= true*/)
	{
		static Vertex* lineVerts = mnew Vertex[4096];
		static VertexIndex* lineIndexes = mnew VertexIndex[4096 * 3];
		static UInt lineMaxVertexCount = 4096;
		static UInt lineMaxPolyCount = 4096;

		UInt lineVertexCount = 0, linePolyCount = 0;

		TextureRef texture = lineType == LineType::Solid ? mSolidLineTexture : mDashLineTexture;
		Vec2I texSize = lineType == LineType::Solid ? Vec2I(1, 1) : mDashLineTexture->GetSize();

		if (scaleToScreenSpace)
		{
			Geometry::CreatePolyLineMesh(vertices, count,
										 lineVerts, lineVertexCount, lineMaxVertexCount,
										 lineIndexes, linePolyCount, lineMaxPolyCount,
										 width - 0.5f, 0.5f, 0.5f, texSize, mInvViewScale);
		}
		else
		{
			Geometry::CreatePolyLineMesh(vertices, count,
										 lineVerts, lineVertexCount, lineMaxVertexCount,
										 lineIndexes, linePolyCount, lineMaxPolyCount,
										 width, 0.5f, 0.5f, texSize, Vec2F(1, 1));
		}

		DrawBuffer(PrimitiveType::Polygon, lineVerts, lineVertexCount,
				   lineIndexes, linePolyCount, mDefaultMaterial, texture);
	}

	TextureRef Render::GetRenderTexture() const
	{
		return mCurrentRenderTarget;
	}

	Vec2I Render::GetMaxTextureSize() const
	{
		return mMaxTextureSize;
	}

	float Render::GetDrawingDepth()
	{
		mDrawingDepth += 1.0f;
		return mDrawingDepth;
	}

	const Vector<Render::ScissorInfo>& Render::GetScissorInfos() const
	{
		return mScissorInfos;
	}

	Render& Render::operator=(const Render& other)
	{
		return *this;
	}

	Render::ScissorInfo::ScissorInfo():
		beginDepth(0), endDepth(0)
	{}

	Render::ScissorInfo::ScissorInfo(const RectI& rect, float beginDepth):
		scissorRect(rect), beginDepth(beginDepth), endDepth(beginDepth)
	{}

	bool Render::ScissorInfo::operator==(const ScissorInfo& other) const
	{
		return Math::Equals(beginDepth, other.beginDepth) && Math::Equals(endDepth, other.endDepth) &&
			scissorRect == other.scissorRect;
	}

	Render::ScissorStackEntry::ScissorStackEntry()
	{}

	Render::ScissorStackEntry::ScissorStackEntry(const RectI& rect, const RectI& summaryRect, bool renderTarget /*= false*/):
		scissorRect(rect), summaryScissorRect(summaryRect), renderTarget(renderTarget)
	{}

	bool Render::ScissorStackEntry::operator==(const ScissorStackEntry& other) const
	{
		return scissorRect == other.scissorRect;
	}
}
