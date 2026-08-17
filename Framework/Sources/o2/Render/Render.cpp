#include "o2/stdafx.h"
#include "o2/Render/Render.h"

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/MaterialAsset.h"
#include "o2/Assets/Types/ShaderAsset.h"
#include "o2/Integration.h"
#include "o2/Render/Font.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Shader.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
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

#if IS_EDITOR
		o2Application.onActivated += MakeFunction(this, &Render::ReloadAssetsOnActivation);
#endif

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

		auto fontsToDelete = mFonts;
		mFonts.Clear();
		fontsToDelete.Clear();

		auto texturesToDelete = mTextures;
		mTextures.Clear();
		texturesToDelete.Clear();

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
		// Default ClampToEdge wrap collapses the dash UV (which ramps from 0 to
		// segLength/32 along the line) into a single solid block past U=1. Repeat
		// makes the 32-px transparent/white pattern actually tile along the line.
		mDashLineTexture->SetWrap(Texture::Wrap::Repeat);
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

		if (mMultithreadedRender)
		{
			if (!mRenderThread.IsRunning())
				mRenderThread.Start();

			// Rendezvous: wait for the render thread to finish the previous frame before reusing the buffers
			mRenderThread.WaitFrameDone();
			mCommandBuffer.Reset();
		}

		mCurrentDrawTexture = nullptr;
		mLastDrawVertex = 0;
		mLastDrawIdx = 0;
		mTrianglesCount = 0;
		mFrameTrianglesCount = 0;
		mDrawCallsCount = 0;
		mSceneDrawCallsCount = 0;
		mSceneTrianglesCount = 0;
		mCurrentPrimitiveType = PrimitiveType::Polygon;
		mCurrentBatchVertexType = Vertex::Type();
		mDrawingDepth = 0.0f;
		mClippingEverything = false;

		mScissorInfos.Clear();
		mStackScissors.Clear();

		BindMaterial(mDefaultMaterial);

		if (mMultithreadedRender)
			PlatformBeginRecording(); // PlatformBegin itself runs on the render thread now
		else
			PlatformBegin();

		mDepthTestEnabled = false;
		mDepthWriteEnabled = true;
		PlatformSetDepthTest(false, true);

		SetupViewMatrix(mResolution);
		UpdateCameraTransforms();

		if (!mCaptureCallback.IsEmpty())
		{
			mCaptureTarget = TextureRef(mResolution, TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
			BindRenderTexture(mCaptureTarget);
		}

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

		// The draw call material wins only when it targets the currently bound attachments:
		// pass-specific content (custom G-buffer materials) beats the pass override, while
		// incompatible materials fall back to the override or the default one
		Ref<Material> drawMaterial;
		if (material && IsMaterialCompatibleWithCurrentTargets(material))
			drawMaterial = material;
		else
			drawMaterial = mOverrideMaterial ? mOverrideMaterial : mDefaultMaterial;

		if (!drawMaterial)
		{
			mLog->Error("DrawBuffer skipped: no material is available for the current draw call");
			return;
		}

		// Skinned vertex layout can be consumed only by skinned-aware shaders: when the resolved
		// material (e.g. a pass override) doesn't support it, the draw is skipped
		if (vertexType.HasParam(VertexParam::BoneIndices) && !drawMaterial->IsVertexLayoutSkinned())
		{
			if (!mSkinnedMaterialMismatchWarned)
			{
				mLog->Warning("Skinned geometry draw skipped: current material doesn't support the skinned vertex layout");
				mSkinnedMaterialMismatchWarned = true;
			}

			return;
		}

		TextureRef texture = overrideTexture ? overrideTexture : drawMaterial->GetTexture();

		// Determine batch vertex type: expand with extra texcoords if material needs them
		VertexType batchVertexType = PlatformResolveBatchVertexType(vertexType, drawMaterial);

		// Geometry bigger than the batch buffers is split into re-indexed chunks
		UInt vertexCapacity = (UInt)(mVertexBufferByteSize/batchVertexType.GetStride());
		if (verticesCount >= vertexCapacity || indexesCount >= mIndexBufferSize)
		{
			DrawBufferChunked(primitiveType, vertices, verticesCount, vertexType, indexes, elementsCount,
							  material, overrideTexture, texSrcRect, allowVertexConversion, vertexCapacity);
			return;
		}

		if (CheckBatchBreak(texture, primitiveType, drawMaterial, batchVertexType, verticesCount, indexesCount))
		{
			DrawPrimitives();

			mCurrentDrawTexture = texture;
			mCurrentPrimitiveType = primitiveType;
			mCurrentBatchVertexType = batchVertexType;

			BindMaterial(drawMaterial);
		}

		// Buffer overrun guard: oversized geometry must have been chunked above
		if (mLastDrawVertex + verticesCount >= vertexCapacity || mLastDrawIdx + indexesCount >= mIndexBufferSize)
		{
			mLog->Error("DrawBuffer skipped: batch buffers overflow");
			return;
		}

		UploadBuffers(vertices, verticesCount, vertexType, indexes, indexesCount, texSrcRect, texture, allowVertexConversion);

		mLastDrawVertex += verticesCount;
		mLastDrawIdx += indexesCount;

		if (primitiveType != PrimitiveType::Line)
			mTrianglesCount += elementsCount;
	}

	void Render::DrawBufferChunked(PrimitiveType primitiveType, const UInt8* vertices, UInt verticesCount,
								   const VertexType& vertexType, VertexIndex* indexes, UInt elementsCount,
								   const Ref<Material>& material, const TextureRef& overrideTexture,
								   const RectI& texSrcRect, bool allowVertexConversion, UInt vertexCapacity)
	{
		UInt indicesPerElement = primitiveType == PrimitiveType::Line ? 2 : 3;
		size_t srcStride = vertexType.GetStride();

		UInt maxChunkVertices = vertexCapacity - 1;
		UInt maxChunkIndices = mIndexBufferSize - 1;

		static std::vector<UInt8> chunkVertices;
		static std::vector<VertexIndex> chunkIndexes;
		static std::vector<int> vertexRemap;

		chunkVertices.resize((size_t)maxChunkVertices*srcStride);
		chunkIndexes.resize(maxChunkIndices);
		vertexRemap.assign(verticesCount, -1);

		UInt chunkVertexCount = 0, chunkIndexCount = 0;

		auto flushChunk = [&]()
		{
			if (chunkIndexCount == 0)
				return;

			DrawBuffer(primitiveType, chunkVertices.data(), chunkVertexCount, vertexType,
					   chunkIndexes.data(), chunkIndexCount/indicesPerElement, material, overrideTexture,
					   texSrcRect, allowVertexConversion);

			chunkVertexCount = 0;
			chunkIndexCount = 0;
			std::fill(vertexRemap.begin(), vertexRemap.end(), -1);
		};

		UInt totalIndexes = elementsCount*indicesPerElement;
		for (UInt i = 0; i + indicesPerElement <= totalIndexes; i += indicesPerElement)
		{
			if (chunkIndexCount + indicesPerElement > maxChunkIndices ||
				chunkVertexCount + indicesPerElement > maxChunkVertices)
			{
				flushChunk();
			}

			for (UInt j = 0; j < indicesPerElement; j++)
			{
				VertexIndex sourceIndex = indexes[i + j];
				if (sourceIndex >= verticesCount)
					continue;

				int mapped = vertexRemap[sourceIndex];
				if (mapped < 0)
				{
					mapped = (int)chunkVertexCount;
					memcpy(&chunkVertices[(size_t)chunkVertexCount*srcStride], vertices + (size_t)sourceIndex*srcStride, srcStride);
					vertexRemap[sourceIndex] = mapped;
					chunkVertexCount++;
				}

				chunkIndexes[chunkIndexCount++] = (VertexIndex)mapped;
			}
		}

		flushChunk();
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

	VertexType Render::ResolveBatchVertexTypeByMaterial(const VertexType& sourceVertexType,
											 const Ref<Material>& material) const
	{
		VertexType batchVertexType = sourceVertexType;

		int materialTexChannels = material ? material->GetTotalTextureChannelsCount() : 1;
		if (materialTexChannels > 1 && !batchVertexType.HasParam(VertexParam::TexCoord1))
			batchVertexType = Vertex2Tex::Type();

		if (materialTexChannels > 2 && !batchVertexType.HasParam(VertexParam::TexCoord2))
			batchVertexType = Vertex3Tex::Type();

		return batchVertexType;
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

			// Layout offsets and sampler rects are the same for every vertex of the batch, and resolving
			// them per vertex (through the vertex type's offset function) costs more than the copying
			bool copyPosition = mCurrentBatchVertexType.HasParam(VertexParam::Position) &&
				srcVertexType.HasParam(VertexParam::Position);
			size_t dstPositionOffset = copyPosition ? mCurrentBatchVertexType.GetParamOffset(VertexParam::Position) : 0;
			size_t srcPositionOffset = copyPosition ? srcVertexType.GetParamOffset(VertexParam::Position) : 0;

			bool copyColor = mCurrentBatchVertexType.HasParam(VertexParam::Color) &&
				srcVertexType.HasParam(VertexParam::Color);
			size_t dstColorOffset = copyColor ? mCurrentBatchVertexType.GetParamOffset(VertexParam::Color) : 0;
			size_t srcColorOffset = copyColor ? srcVertexType.GetParamOffset(VertexParam::Color) : 0;

			bool copyNormal = mCurrentBatchVertexType.HasParam(VertexParam::Normal) &&
				srcVertexType.HasParam(VertexParam::Normal);
			size_t dstNormalOffset = copyNormal ? mCurrentBatchVertexType.GetParamOffset(VertexParam::Normal) : 0;
			size_t srcNormalOffset = copyNormal ? srcVertexType.GetParamOffset(VertexParam::Normal) : 0;

			bool dstHasUV = mCurrentBatchVertexType.HasParam(VertexParam::TexCoord0);
			size_t dstUVOffset = dstHasUV ? mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0) : 0;

			struct SamplerUV
			{
				size_t dstOffset;
				RectI  srcRect;
				Vec2F  invTexSize;
				bool   remap;
			};

			static const UInt texCoordParams[] = { VertexParam::TexCoord1, VertexParam::TexCoord2 };
			SamplerUV samplerUVs[2];
			int samplerUVsCount = 0;

			for (int s = 0; s < samplers.Count() && s < 2; s++)
			{
				if (!mCurrentBatchVertexType.HasParam(texCoordParams[s]))
					continue;

				SamplerUV& samplerUV = samplerUVs[samplerUVsCount++];
				samplerUV.dstOffset = mCurrentBatchVertexType.GetParamOffset(texCoordParams[s]);
				samplerUV.srcRect = samplers[s].GetSrcRect();
				samplerUV.invTexSize.Set(1.0f, 1.0f);
				samplerUV.remap = samplerUV.srcRect != RectI();

				if (samplerUV.remap)
				{
					if (TextureRef samplerTex = samplers[s].GetTexture())
					{
						Vec2F sz = samplerTex->GetSize();
						samplerUV.invTexSize.Set(1.0f/sz.x, 1.0f/sz.y);
					}
				}
			}

			UInt8* dst = &mVertexData[mLastDrawVertex * dstStride];

			for (UInt i = 0; i < verticesCount; i++)
			{
				const UInt8* srcVtx = vertices + i * srcStride;
				UInt8* dstVtx = dst + i * dstStride;

				if (copyPosition)
				{
					memcpy(dstVtx + dstPositionOffset, srcVtx + srcPositionOffset,
						   VertexParam::ParamSize(VertexParam::Position));
				}

				if (copyColor)
				{
					memcpy(dstVtx + dstColorOffset, srcVtx + srcColorOffset,
						   VertexParam::ParamSize(VertexParam::Color));
				}

				float srcU = 0.0f, srcV = 0.0f;
				if (srcHasUV)
				{
					srcU = *(const float*)(srcVtx + srcUVOffset);
					srcV = *(const float*)(srcVtx + srcUVOffset + sizeof(float));
				}

				// TexCoord0: remap through primary texture srcRect
				if (dstHasUV)
				{
					float* dstUV = (float*)(dstVtx + dstUVOffset);
					if (needsUVRemap)
						RemapUV(srcU, srcV, texSrcRect, primaryInvTexSize, dstUV[0], dstUV[1]);
					else
					{
						dstUV[0] = srcU;
						dstUV[1] = srcV;
					}
				}

				// TexCoord1, TexCoord2: remap through additional sampler srcRects
				for (int s = 0; s < samplerUVsCount; s++)
				{
					const SamplerUV& samplerUV = samplerUVs[s];
					float* dstUV = (float*)(dstVtx + samplerUV.dstOffset);

					if (samplerUV.remap)
						RemapUV(srcU, srcV, samplerUV.srcRect, samplerUV.invTexSize, dstUV[0], dstUV[1]);
					else
					{
						dstUV[0] = srcU;
						dstUV[1] = srcV;
					}
				}

				if (copyNormal)
				{
					memcpy(dstVtx + dstNormalOffset, srcVtx + srcNormalOffset,
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

		if (mMultithreadedRender)
			RecordDrawCommand();
		else
			PlatformDrawPrimitives();

		mFrameTrianglesCount += mTrianglesCount;

		// Split off what the scene costs: in the editor everything but the Game window rendering happens
		// inside an editor scope, and mixing the two makes the numbers unreadable
		if (!EditorScope::IsInScope())
		{
			mSceneTrianglesCount += mTrianglesCount;
			mSceneDrawCallsCount++;
		}

		mLastDrawVertex = mTrianglesCount = mLastDrawIdx = 0;

		mDrawCallsCount++;

		if (IsRenderDrawCallsDebugEnabled())
			mLog->OutStr("#DC " + (String)mDrawCallsCount + "; with texture\"" + mCurrentDrawTexture->GetFileName() + "\"");
	}

	void Render::CheckVertexBufferTexCoordFlipByTextureFormat()
	{
		PROFILE_SAMPLE_FUNC();

		// All block-compressed files (DDS, ASTC) store rows top-down, unlike the engine's
		// bottom-up png textures, so their V coordinate is mirrored at draw time
		if (mCurrentDrawTexture && Texture::IsFormatCompressed(mCurrentDrawTexture->GetFormat()))
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

		if (mCaptureTarget)
			UnbindRenderTexture();

		DrawPrimitives();

		if (mMultithreadedRender)
		{
			// The main thread hands the recorded frame to the render thread and waits for it to submit
			// and present — the two threads rendezvous here every frame
			PlatformAcquireFrameTarget();
			mRenderThread.DispatchFrame([this] { SubmitRecordedFrame(); });
			mRenderThread.WaitFrameDone();

			if (mCaptureTarget)
				DeliverFrameCapture();
		}
		else
		{
			PlatformEnd();

			if (mCaptureTarget)
				DeliverFrameCapture();
		}

		CheckTexturesUnloading();
		CheckFontsUnloading();
	}

	bool Render::IsMultithreadedRenderSupported()
	{
		return PlatformSupportsMultithreadedRender();
	}

	void Render::SetMultithreadedRenderEnabled(bool enabled)
	{
		mMultithreadedRender = enabled && IsMultithreadedRenderSupported();
	}

	bool Render::IsMultithreadedRenderEnabled() const
	{
		return mMultithreadedRender;
	}

	void Render::RecordDrawCommand()
	{
		RenderDrawCommand& command = mCommandBuffer.Emplace();

		UInt stride = mCurrentBatchVertexType.GetStride();
		UInt vertexBytes = mLastDrawVertex * stride;

		// Grow-only: the used length is carried by vertexCount/indexCount, so the pooled storage is kept
		// at its high-water mark instead of being re-sized (and re-zeroed) every frame
		if (command.vertexData.Count() < (int)vertexBytes)
			command.vertexData.Resize(vertexBytes);

		if (vertexBytes > 0)
			memcpy(command.vertexData.data(), mVertexData, vertexBytes);

		if (command.indexData.Count() < (int)mLastDrawIdx)
			command.indexData.Resize(mLastDrawIdx);

		if (mLastDrawIdx > 0)
			memcpy(command.indexData.data(), mVertexIndexData, mLastDrawIdx * sizeof(VertexIndex));

		command.vertexCount = mLastDrawVertex;
		command.indexCount = mLastDrawIdx;
		command.trianglesCount = mTrianglesCount;
		command.clearOnly = false;
		command.vertexStride = (int)stride;
		command.primitiveType = (int)mCurrentPrimitiveType;
		command.drawTexture = mCurrentDrawTexture;
		command.material = mCurrentMaterial;
		command.depthTestEnabled = mDepthTestEnabled;
		command.depthWriteEnabled = mDepthWriteEnabled;
		command.renderTarget = mCurrentRenderTarget;
		command.extraRenderTargets = mExtraRenderTargets;
		command.resolution = mCurrentResolution;

		// Platform-specific submit state (mvp matrix, scissor rect, clear flags)
		PlatformSnapshotDrawState(command);
	}

	void Render::SubmitRecordedFrame()
	{
		PROFILE_SAMPLE("o2 Render Replay");

		PlatformBeginThreaded();

		for (int i = 0; i < mCommandBuffer.Count(); i++)
			PlatformReplayDrawCommand(mCommandBuffer.Get(i));

		PlatformEndThreaded();
	}

	void Render::CaptureNextFrame(const Function<void(const Ref<Bitmap>&)>& onCaptured)
	{
		mCaptureCallback = onCaptured;
	}

	void Render::DeliverFrameCapture()
	{
		// The render-target y-flip in the camera transforms already matches the Bitmap bottom-up
		// row convention: the PNG saver mirrors rows itself, so an extra flip here would double up
		// and save screenshots upside down
		Ref<Bitmap> bitmap = mCaptureTarget->GetData();

		auto callback = mCaptureCallback;
		mCaptureCallback.Clear();
		mCaptureTarget = nullptr;

		callback(bitmap);
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

	void Render::SetOverrideMaterial(const Ref<Material>& material)
	{
		if (mOverrideMaterial == material)
			return;

		DrawPrimitives();
		mOverrideMaterial = material;
	}

	const Ref<Material>& Render::GetOverrideMaterial() const
	{
		return mOverrideMaterial;
	}

	bool Render::IsMaterialCompatibleWithCurrentTargets(const Ref<Material>& material) const
	{
		if (!material)
			return false;

		auto& formats = material->GetColorAttachmentFormats();
		int materialAttachments = Math::Max(1, formats.Count());
		int currentAttachments = 1 + (mCurrentRenderTarget ? mExtraRenderTargets.Count() : 0);
		if (materialAttachments != currentAttachments)
			return false;

		for (int i = 0; i < materialAttachments; i++)
		{
			TextureFormat materialFormat = i < formats.Count() ? formats[i] : TextureFormat::R8G8B8A8;

			TextureFormat targetFormat = TextureFormat::R8G8B8A8;
			if (i == 0)
			{
				if (mCurrentRenderTarget)
					targetFormat = mCurrentRenderTarget->GetFormat();
			}
			else
				targetFormat = mExtraRenderTargets[i - 1]->GetFormat();

			if (materialFormat != targetFormat)
				return false;
		}

		return true;
	}

	bool Render::IsMRTSupported() const
	{
		return PlatformSupportsMRT();
	}

	void Render::BindRenderTargets(const Vector<TextureRef>& renderTargets)
	{
		if (renderTargets.IsEmpty())
		{
			UnbindRenderTexture();
			return;
		}

		Vector<TextureRef> extraTargets;
		if (renderTargets.Count() > 1)
		{
			if (!IsMRTSupported())
			{
				if (!mMRTUnsupportedWarned)
				{
					mLog->WarningStr("Multiple render targets are not supported on this platform, binding only the first target");
					mMRTUnsupportedWarned = true;
				}
			}
			else
			{
				for (int i = 1; i < renderTargets.Count(); i++)
				{
					const TextureRef& target = renderTargets[i];
					if (!target || target->mUsage != Texture::Usage::RenderTarget || !target->IsReady())
					{
						mLog->Error("Can't bind extra render target: not a ready render target texture");
						continue;
					}

					extraTargets.Add(target);
				}
			}
		}

		BindRenderTexture(renderTargets[0]);

		if (mCurrentRenderTarget)
			mExtraRenderTargets = extraTargets;
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
		mDepthTestEnabled = false;
		mDepthWriteEnabled = true;

		PlatformResetState();
		// Deliberately NOT resetting the view matrix to the window resolution here:
		// ResetState runs after external renderers (cocos) finish inside a render
		// target pass, and the current resolution must stay the target's one —
		// otherwise everything drawn after (editor handles) gets a window-sized
		// projection. External renderers trash matrices behind our cache, so the
		// camera transforms are recomputed unconditionally
		UpdateCameraTransforms(true);
	}

	void Render::EndCustomRender()
	{
		ResetState();
	}

	void Render::UpdateCameraTransforms(bool force /*= false*/)
	{
		PROFILE_SAMPLE_FUNC();

		// The render target y-flip is baked into the transforms, so a target binding
		// change must trigger recomputation even when camera and resolution are the same
		bool renderingToTarget = mCurrentRenderTarget != nullptr;
		if (!force && mCurrentResolution == mPrevResolution && mCamera == mPrevCamera &&
			renderingToTarget == mPrevTransformsToTarget)
		{
			return;
		}

		mPrevTransformsToTarget = renderingToTarget;

		DrawPrimitives();

		if (mCamera.projection != Camera::Projection::Orthographic)
		{
			Mat4 proj = mCamera.GetProjectionMatrix((Vec2F)mCurrentResolution);
			Mat4 view = mCamera.GetViewMatrix3D();
			Mat4 model;

			mViewScale = Vec2F(1.0f, 1.0f);
			mInvViewScale = Vec2F(1.0f, 1.0f);

			PlatformSetupCameraTransforms(model.m, view.m, proj.m);

			mPrevCamera = mCamera;
			mPrevResolution = mCurrentResolution;
			return;
		}

		Vec2F resf = (Vec2F)mCurrentResolution;
		Vec2F halfRes(Math::Round(resf.x / 2.0f), Math::Round(resf.y / 2.0f));

		float projMatrix[16];
		Math::OrthoProjMatrix(projMatrix, 0.0f, (float)mCurrentResolution.x, (float)mCurrentResolution.y, 0.0f,
							  -Camera::ortho2DHalfDepth, Camera::ortho2DHalfDepth);

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

		// World z is kept so depth test can order 3D content drawn with the 2D camera
		float viewMatrix[16] =
		{
			camTransf.xv.x,     camTransf.xv.y,     0, 0,
			camTransf.yv.x,     camTransf.yv.y,     0, 0,
			0,                  0,                  1, 0,
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
			// The stack may hold only render-target entries, which add no scissor infos
			if (!mScissorInfos.IsEmpty())
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

			// The stack may hold only render-target entries, which add no scissor infos
			if (!mScissorInfos.IsEmpty())
				mScissorInfos.Last().endDepth = mDrawingDepth;
		}
		else
		{
			if (mStackScissors.Count() == 1)
			{
				PlatformDisableScissorTest();
				mStackScissors.PopBack();

				if (!mScissorInfos.IsEmpty())
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
		PlatformFlushPendingClear();

		if (!mStackScissors.IsEmpty())
		{
			// The stack may hold only render-target entries, which add no scissor infos
			if (!mScissorInfos.IsEmpty())
				mScissorInfos.Last().endDepth = mDrawingDepth;

			PlatformDisableScissorTest();
		}

		mStackScissors.Add(ScissorStackEntry(RectI(), RectI(), true));

		mCurrentRenderTarget = renderTarget;
		mExtraRenderTargets.Clear();

		PlatformBindRenderTarget(renderTarget);
		SetupViewMatrix(renderTarget->GetSize());
	}

	void Render::UnbindRenderTexture()
	{
		if (!mCurrentRenderTarget)
			return;

		DrawPrimitives();
		PlatformFlushPendingClear();
		PlatformBindRenderTarget(nullptr);

		mCurrentRenderTarget = TextureRef();
		mExtraRenderTargets.Clear();

		SetupViewMatrix(mResolution);
		SetCamera(Camera());

		DisableScissorTest(true);
		mStackScissors.PopBack();
		RestoreScissorStateFromStack();
	}

	void Render::RestoreScissorStateFromStack()
	{
		if (mStackScissors.IsEmpty() || mStackScissors.Last().renderTarget)
		{
			PlatformDisableScissorTest();
			mClippingEverything = false;
			return;
		}

		auto clipRect = mStackScissors.Last().summaryScissorRect;

		PlatformEnableScissorTest();
		PlatformSetScissorRect(clipRect);

		mClippingEverything = clipRect == RectI();
	}

	void Render::PushRenderTargets(const Vector<TextureRef>& renderTargets)
	{
		mRenderTargetsStack.Add(mCurrentRenderTarget);
		BindRenderTargets(renderTargets);
	}

	void Render::PopRenderTargets()
	{
		if (mRenderTargetsStack.IsEmpty())
		{
			mLog->WarningStr("Can't pop render targets - stack is empty");
			return;
		}

		TextureRef previousTarget = mRenderTargetsStack.PopBack();
		if (!previousTarget)
		{
			UnbindRenderTexture();
			return;
		}

		DrawPrimitives();
		PlatformFlushPendingClear();

		// Remove unclosed scissors and the stack entry added by the paired push, keeping the stack balanced
		while (!mStackScissors.IsEmpty() && !mStackScissors.Last().renderTarget)
			mStackScissors.PopBack();

		if (!mStackScissors.IsEmpty())
			mStackScissors.PopBack();

		mCurrentRenderTarget = previousTarget;
		mExtraRenderTargets.Clear();

		PlatformBindRenderTarget(previousTarget);
		SetupViewMatrix(previousTarget->GetSize());

		RestoreScissorStateFromStack();
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

	int Render::GetSceneDrawCallsCount() const
	{
		return mSceneDrawCallsCount;
	}

	int Render::GetSceneDrawnPrimitives() const
	{
		return mSceneTrianglesCount;
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

		// Reused buffer: gizmos draw thousands of short lines per frame, one allocation each would
		// cost more than the drawing
		if (mAALineVertices.Count() < points.Count())
			mAALineVertices.Resize(points.Count());

		for (int i = 0; i < points.Count(); i++)
			mAALineVertices[i] = Vertex(points[i], dcolor, 0, 0);

		DrawAAPolyLine(mAALineVertices.data(), points.Count(), width, lineType, scaleToScreenSpace);
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

	void Render::SetDepthTestEnabled(bool enabled, bool writeEnabled /*= true*/)
	{
		if (mDepthTestEnabled == enabled && mDepthWriteEnabled == writeEnabled)
			return;

		DrawPrimitives();

		mDepthTestEnabled = enabled;
		mDepthWriteEnabled = writeEnabled;
		PlatformSetDepthTest(enabled, writeEnabled);
	}

	bool Render::IsDepthTestEnabled() const
	{
		return mDepthTestEnabled;
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

#if IS_EDITOR
	void Render::ReloadAssetsOnActivation()
	{
		o2Assets.RebuildAssets();

		int reloadedShaders = 0;
		for (auto& asset : o2Assets.mCachedAssets)
		{
			auto shaderAsset = DynamicCast<ShaderAsset>(asset.GetRef());
			if (!shaderAsset || !shaderAsset->GetShader())
				continue;

			String path = Shader::ResolvePlatformSourcePath(shaderAsset->GetFullPath());
			TimeStamp currentDate = o2FileSystem.GetFileInfo(path).editDate;
			if (currentDate == shaderAsset->GetShader()->GetFileEditDate())
				continue;

			String source = o2FileSystem.ReadFile(path);
			if (source.IsEmpty())
				continue;

			auto shader = shaderAsset->GetShader();
			if (shader->Compile(source, shader->GetShaderType()))
			{
				shader->SetFileEditDate(currentDate);
				reloadedShaders++;
			}
			else
				mLog->Error("Failed to recompile shader: " + shaderAsset->GetPath());
		}

		if (reloadedShaders > 0)
		{
			for (auto& asset : o2Assets.mCachedAssets)
			{
				auto materialAsset = DynamicCast<MaterialAsset>(asset.GetRef());
				if (materialAsset)
					materialAsset->Build();
			}

			if (mDefaultMaterial)
				mDefaultMaterial->Build();

			mCurrentMaterial = nullptr;
		}

		int reloadedTextures = 0;
		for (auto& tex : mTextures)
		{
			if (!tex || tex->GetFileName().IsEmpty())
				continue;

			TimeStamp currentDate = o2FileSystem.GetFileInfo(tex->GetFileName()).editDate;
			if (currentDate == tex->GetFileEditDate())
				continue;

			tex->Reload();
			reloadedTextures++;
		}

		if (reloadedTextures > 0)
		{
			for (auto& atlas : mAtlases)
				atlas->ReloadPages();

			for (auto& spr : mSprites)
				spr->ReloadImage();
		}

		if (reloadedShaders > 0 || reloadedTextures > 0)
			mLog->Out("Reloaded " + (String)reloadedShaders + " shaders, " + (String)reloadedTextures + " textures");
	}
#endif
}
