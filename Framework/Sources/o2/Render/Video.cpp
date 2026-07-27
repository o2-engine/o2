#include "o2/stdafx.h"
#include "Video.h"

#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    Video::Video():
        mMesh(nullptr, 4, 2)
    {
        UpdateMesh();
    }

    Video::Video(const AssetRef<VideoAsset>& video):
        mMesh(nullptr, 4, 2)
    {
        SetVideoAsset(video);

        if (mVideoSize.x > 0 && mVideoSize.y > 0)
            SetSize2D((Vec2F)mVideoSize);
    }

    Video::Video(const Video& other):
        IRectDrawable(other), IAnimation(other), mMesh(nullptr, 4, 2),
        mStreaming(other.mStreaming), mPlayOnAwake(other.mPlayOnAwake),
        mChromaKeyEnabled(other.mChromaKeyEnabled), mKeyColor(other.mKeyColor),
        mSimilarity(other.mSimilarity), mSmoothness(other.mSmoothness), mSpill(other.mSpill)
    {
        SetVideoAsset(other.mVideoAsset);
        UpdateColor();
    }

    Video::~Video()
    {
        ReleaseDecoder();
    }

    Video& Video::operator=(const Video& other)
    {
        IRectDrawable::operator=(other);
        IAnimation::operator=(other);

        mStreaming = other.mStreaming;
        mPlayOnAwake = other.mPlayOnAwake;
        mChromaKeyEnabled = other.mChromaKeyEnabled;
        mKeyColor = other.mKeyColor;
        mSimilarity = other.mSimilarity;
        mSmoothness = other.mSmoothness;
        mSpill = other.mSpill;

        SetVideoAsset(other.mVideoAsset);
        UpdateColor();

        return *this;
    }

    void Video::Draw()
    {
        if (!mEnabled || !mTexture)
            return;

        mMesh.Draw();
        OnDrawn();
    }

    void Video::SetVideoAsset(const AssetRef<VideoAsset>& asset)
    {
        ReleaseDecoder();
        mVideoAsset = asset;
        CreateDecoder();
        UpdateDuration();
        UpdateChromaMaterial();
        SetTime(0.0f);
    }

    AssetRef<VideoAsset> Video::GetVideoAsset() const
    {
        return mVideoAsset;
    }

    void Video::SetStreaming(bool streaming)
    {
        if (mStreaming == streaming)
            return;

        mStreaming = streaming;

        if (mVideoAsset)
        {
            float time = GetTime();
            CreateDecoder();
            UpdateDuration();
            SetTime(time);
        }
    }

    bool Video::IsStreaming() const
    {
        return mStreaming;
    }

    const TextureRef& Video::GetTexture() const
    {
        return mMesh.GetTexture();
    }

    Vec2I Video::GetVideoSize() const
    {
        return mVideoSize;
    }

    void Video::SetPlayOnAwake(bool playOnAwake)
    {
        mPlayOnAwake = playOnAwake;
    }

    bool Video::IsPlayOnAwake() const
    {
        return mPlayOnAwake;
    }

    void Video::SetChromaKeyEnabled(bool enabled)
    {
        mChromaKeyEnabled = enabled;
        UpdateChromaMaterial();
    }

    bool Video::IsChromaKeyEnabled() const
    {
        return mChromaKeyEnabled;
    }

    void Video::SetKeyColor(const Color4& color)
    {
        mKeyColor = color;
        UpdateChromaMaterial();
    }

    Color4 Video::GetKeyColor() const
    {
        return mKeyColor;
    }

    void Video::SetSimilarity(float similarity)
    {
        mSimilarity = similarity;
        UpdateChromaMaterial();
    }

    float Video::GetSimilarity() const
    {
        return mSimilarity;
    }

    void Video::SetSmoothness(float smoothness)
    {
        mSmoothness = smoothness;
        UpdateChromaMaterial();
    }

    float Video::GetSmoothness() const
    {
        return mSmoothness;
    }

    void Video::SetSpill(float spill)
    {
        mSpill = spill;
        UpdateChromaMaterial();
    }

    float Video::GetSpill() const
    {
        return mSpill;
    }

    void Video::SetChoke(float choke)
    {
        mChoke = choke;
        UpdateChromaMaterial();
    }

    float Video::GetChoke() const
    {
        return mChoke;
    }

    void Video::BasisChanged()
    {
        UpdateMesh();
    }

    void Video::OnColorChanged()
    {
        UpdateMesh();
    }

    void Video::OnMaterialChanged()
    {
        mMesh.SetMaterial(GetMaterial());
        IRectDrawable::OnMaterialChanged();
    }

    void Video::Evaluate()
    {
        if (!mDecoder && !mDecoderFailed)
            CreateDecoder();

        if (!mDecoder)
            return;

        // Backends with async setup (wasm video element) report their size later
        if (mVideoSize.x <= 0)
        {
            mVideoSize = mDecoder->GetSize();
            if (mVideoSize.x <= 0)
                return;

            CreateFrameResources();
            UpdateDuration();
            UpdateChromaMaterial();

            if (GetSize2D() == Vec2F())
                SetSize2D((Vec2F)mVideoSize);
        }

        if (!mFrameBitmap || !mTexture)
            return;

        float target = mInDurationTime;
        if (!(target == target) || target < 0.0f) // guard NaN/negative from an unresolved duration
            target = 0.0f;

        float time = 0.0f;
        bool decoded = false;

        float frameRate = mDecoder->GetFrameRate();
        float frameDuration = frameRate > 0.0f ? 1.0f/frameRate : 1.0f/30.0f;

        // The shown frame covers [mFrameTime, mFrameTime + frameDuration): decode forward only
        // when the next frame is due, seek only on a real backward scrub or a far jump. Decoding
        // past the target would force a seek (decoder recreation) on every following update
        bool backward = target < mFrameTime - frameDuration*0.5f;
        bool farAhead = target - mFrameTime > 1.0f;

        if (mFrameTime < 0.0f || backward || farAhead)
        {
            PROFILE_SAMPLE("Video seek");

            if (mDecoder->SeekFrame(Math::Max(target, 0.0f), time))
            {
                decoded = true;
                mFrameTime = time;
            }
        }
        else
        {
            PROFILE_SAMPLE("Video decode");

            int guard = 0;
            while (mFrameTime + frameDuration <= target + 0.0001f && guard++ < 4096)
            {
                if (!mDecoder->DecodeNextFrame(time))
                    break;

                decoded = true;
                mFrameTime = time;
            }
        }

        if (decoded && !mDecoder->UploadLastFrame(mTexture))
        {
            if (mDecoder->ReadLastFrame(*mFrameBitmap))
                mTexture->SetData(*mFrameBitmap);
        }
    }

    void Video::UpdateMesh()
    {
        BuildDefaultMesh();

        float z = mTransform.origin.z;
        if (!Math::Equals(z, 0.0f))
        {
            Vertex* verts = mMesh.GetVertices<Vertex>();
            for (UInt i = 0; i < mMesh.vertexCount; i++)
                verts[i].z = z;
        }
    }

    void Video::BuildDefaultMesh()
    {
        const Basis transform = mTransform.ToBasis();
        Vec2F normal = transform.xv.Normalized();
        ULong rcc = mResultColor.ABGR();

        static VertexIndex indexes[] = { 0, 1, 2, 0, 2, 3 };

        Vertex* verts = mMesh.GetVertices<Vertex>();
        verts[0].Set(transform.origin + transform.yv,                rcc, 0.0f, 0.0f, normal);
        verts[1].Set(transform.origin + transform.yv + transform.xv, rcc, 1.0f, 0.0f, normal);
        verts[2].Set(transform.origin + transform.xv,                rcc, 1.0f, 1.0f, normal);
        verts[3].Set(transform.origin,                               rcc, 0.0f, 1.0f, normal);

        memcpy(mMesh.GetIndexes(), indexes, sizeof(VertexIndex)*6);

        mMesh.vertexCount = 4;
        mMesh.polyCount = 2;
    }

    void Video::CreateDecoder()
    {
        ReleaseDecoder();

        if (!mVideoAsset)
            return;

        mDecoder = CreateVideoDecoder(mVideoAsset, mStreaming);
        if (!mDecoder)
        {
            mDecoderFailed = true;
            return;
        }

        mVideoSize = mDecoder->GetSize();
        mFrameTime = -1.0f;

        CreateFrameResources();

        if (GetSize2D() == Vec2F() && mVideoSize.x > 0 && mVideoSize.y > 0)
            SetSize2D((Vec2F)mVideoSize);
    }

    void Video::ReleaseDecoder()
    {
        mDecoder = nullptr;
        mDecoderFailed = false;
        mFrameTime = -1.0f;
    }

    void Video::CreateFrameResources()
    {
        if (mVideoSize.x <= 0 || mVideoSize.y <= 0)
        {
            mTexture = TextureRef();
            mFrameBitmap = nullptr;
            return;
        }

        mFrameBitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, mVideoSize);
        mFrameBitmap->Fill(Color4::Black());

        mTexture = TextureRef(mVideoSize, TextureFormat::R8G8B8A8, Texture::Usage::Default);
        mMesh.SetTexture(mTexture);

        mTextureSrcRect = RectI(Vec2I(), mVideoSize);
        mMesh.SetTextureSrcRect(mTextureSrcRect);

        UpdateMesh();
    }

    void Video::UpdateDuration()
    {
        float duration = 0.0f;
        if (mDecoder)
            duration = mDecoder->GetDuration();
        else if (mVideoAsset)
            duration = mVideoAsset->GetDuration();

        mDuration = duration;
        ResetBounds();
    }

    void Video::UpdateChromaMaterial()
    {
        if (!mChromaKeyEnabled)
        {
            if (GetMaterial() == mChromaMaterial)
                SetMaterial(nullptr);

            return;
        }

        if (!mChromaMaterial)
        {
            mChromaMaterial = Material::CreateFromBuiltinShaders("ChromaKey");
            if (mChromaMaterial)
            {
                mChromaMaterial->AddParam(mmake<ShaderParamColor>("u_keyColor", mKeyColor));
                mChromaMaterial->AddParam(mmake<ShaderParamFloat>("u_similarity", mSimilarity));
                mChromaMaterial->AddParam(mmake<ShaderParamFloat>("u_smoothness", mSmoothness));
                mChromaMaterial->AddParam(mmake<ShaderParamFloat>("u_spill", mSpill));
                mChromaMaterial->AddParam(mmake<ShaderParamFloat>("u_choke", mChoke));
                mChromaMaterial->Build();
            }
        }
        else
        {
            if (auto p = DynamicCast<ShaderParamColor>(mChromaMaterial->GetShaderParam("u_keyColor")))
                p->SetValue(mKeyColor);
            if (auto p = DynamicCast<ShaderParamFloat>(mChromaMaterial->GetShaderParam("u_similarity")))
                p->SetValue(mSimilarity);
            if (auto p = DynamicCast<ShaderParamFloat>(mChromaMaterial->GetShaderParam("u_smoothness")))
                p->SetValue(mSmoothness);
            if (auto p = DynamicCast<ShaderParamFloat>(mChromaMaterial->GetShaderParam("u_spill")))
                p->SetValue(mSpill);
            if (auto p = DynamicCast<ShaderParamFloat>(mChromaMaterial->GetShaderParam("u_choke")))
                p->SetValue(mChoke);

            mChromaMaterial->InvalidateHash();
        }

        if (mChromaMaterial && mChromaMaterial->IsReady())
            SetMaterial(mChromaMaterial);
    }

    Ref<RefCounterable> Video::CastToRefCounterable(const Ref<Video>& ref)
    {
        return DynamicCast<IAnimation>(ref);
    }

    void Video::OnSerialize(DataValue& node) const
    {
        IRectDrawable::OnSerialize(node);
        IAnimation::OnSerialize(node);
    }

    void Video::OnDeserialized(const DataValue& node)
    {
        IRectDrawable::OnDeserialized(node);
        IAnimation::OnDeserialized(node);

        CreateDecoder();
        UpdateDuration();
        UpdateChromaMaterial();
        UpdateColor();
        UpdateMesh();
        SetTime(mTime);
    }
}
// --- META ---

DECLARE_CLASS(o2::Video, o2__Video);
// --- END META ---
