#pragma once

#include "o2/Animation/IAnimation.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/IRectDrawable.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Editor/Attributes/RangeAttribute.h"
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"

namespace o2
{
    class Bitmap;
    class VideoDecoder;

    // ---------------------------------------------------------------------------
    // Quad video drawable. Plays a video asset into a dynamic texture and draws
    // it like a sprite. Decoding goes through a VideoDecoder backend: mp4/mov -
    // platform hardware decoder (AVFoundation on Mac/iOS), mpg - pl_mpeg software
    // decoder. Playback is driven through the IAnimation interface, so it can be
    // used as an animation sub-track (its frame follows the animation time).
    // Optionally keys out a solid background color with a soft edge via the
    // ChromaKey material. The encoded data can be kept in memory or streamed from
    // disk (see streaming).
    // ---------------------------------------------------------------------------
    class Video: public IRectDrawable, public IAnimation
    {
    public:
        PROPERTIES(Video);
        PROPERTY(AssetRef<VideoAsset>, video, SetVideoAsset, GetVideoAsset); // Video asset property @SCRIPTABLE
        PROPERTY(bool, streaming, SetStreaming, IsStreaming);               // Stream from disk instead of memory property @SCRIPTABLE
        PROPERTY(bool, playOnAwake, SetPlayOnAwake, IsPlayOnAwake);         // Auto-play on start property @SCRIPTABLE

        PROPERTY(bool, chromaKey, SetChromaKeyEnabled, IsChromaKeyEnabled); // Chroma-key enable property @SCRIPTABLE
        PROPERTY(Color4, keyColor, SetKeyColor, GetKeyColor);              // Chroma-key color property @SCRIPTABLE
        PROPERTY(float, similarity, SetSimilarity, GetSimilarity);         // Chroma-key similarity property @SCRIPTABLE @RANGE(0, 1)
        PROPERTY(float, smoothness, SetSmoothness, GetSmoothness);         // Chroma-key edge smoothness property @SCRIPTABLE @RANGE(0, 1)
        PROPERTY(float, spill, SetSpill, GetSpill);                        // Chroma-key spill suppression property @SCRIPTABLE @RANGE(0, 1)
        PROPERTY(float, choke, SetChoke, GetChoke);                        // Chroma-key matte choke property @SCRIPTABLE @RANGE(0, 1)

    public:
        // Default constructor
        Video();

        // Constructor from video asset
        explicit Video(const AssetRef<VideoAsset>& video);

        // Copy-constructor
        Video(const Video& other);

        // Destructor
        ~Video();

        // Assign operator
        Video& operator=(const Video& other);

        // Draws video @SCRIPTABLE
        void Draw() override;

        // Sets video asset @SCRIPTABLE
        void SetVideoAsset(const AssetRef<VideoAsset>& asset);

        // Returns video asset @SCRIPTABLE
        AssetRef<VideoAsset> GetVideoAsset() const;

        // Enables streaming: decode from the asset file on disk instead of the in-memory bytes @SCRIPTABLE
        void SetStreaming(bool streaming);

        // Returns true if streaming from disk @SCRIPTABLE
        bool IsStreaming() const;

        // Returns video frame texture
        const TextureRef& GetTexture() const;

        // Returns video frame size in pixels
        Vec2I GetVideoSize() const;

        // Sets auto-play on start @SCRIPTABLE
        void SetPlayOnAwake(bool playOnAwake);

        // Returns true if auto-play on start is enabled @SCRIPTABLE
        bool IsPlayOnAwake() const;

        // Enables or disables chroma-key background removal @SCRIPTABLE
        void SetChromaKeyEnabled(bool enabled);

        // Returns true if chroma-key is enabled @SCRIPTABLE
        bool IsChromaKeyEnabled() const;

        // Sets the chroma-key color to remove @SCRIPTABLE
        void SetKeyColor(const Color4& color);

        // Returns the chroma-key color @SCRIPTABLE
        Color4 GetKeyColor() const;

        // Sets chroma-key similarity: chroma distance below which pixels are fully keyed out @SCRIPTABLE
        void SetSimilarity(float similarity);

        // Returns chroma-key similarity @SCRIPTABLE
        float GetSimilarity() const;

        // Sets chroma-key soft edge width @SCRIPTABLE
        void SetSmoothness(float smoothness);

        // Returns chroma-key soft edge width @SCRIPTABLE
        float GetSmoothness() const;

        // Sets chroma-key spill suppression width @SCRIPTABLE
        void SetSpill(float spill);

        // Returns chroma-key spill suppression width @SCRIPTABLE
        float GetSpill() const;

        // Sets chroma-key matte choke: how deep the edge is cut next to the backdrop @SCRIPTABLE
        void SetChoke(float choke);

        // Returns chroma-key matte choke @SCRIPTABLE
        float GetChoke() const;

        // Calling when serializing
        void OnSerialize(DataValue& node) const override;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Dynamic cast to RefCounterable, disambiguating the IRectDrawable/IAnimation diamond
        static Ref<RefCounterable> CastToRefCounterable(const Ref<Video>& ref);

        SERIALIZABLE(Video);
        CLONEABLE_REF(Video);
        REF_COUNTERABLE_IMPL(IRectDrawable, IAnimation);

    protected:
        AssetRef<VideoAsset> mVideoAsset; // Video asset @SERIALIZABLE

        bool mStreaming = false;   // Decode from disk file instead of in-memory bytes @SERIALIZABLE
        bool mPlayOnAwake = true;  // Auto-play on start @SERIALIZABLE

        bool   mChromaKeyEnabled = false;   // Chroma-key enable @SERIALIZABLE
        Color4 mKeyColor = Color4::Green();  // Chroma-key color @SERIALIZABLE
        float  mSimilarity = 0.05f;          // Key distance noise floor: below it the pixel is pure backdrop @SERIALIZABLE
        float  mSmoothness = 0.45f;          // Matte ramp width: distance span from backdrop to full opacity @SERIALIZABLE
        float  mSpill = 1.0f;                // Despill strength @SERIALIZABLE
        float  mChoke = 0.0f;                // Matte black clip @SERIALIZABLE

        RectI mTextureSrcRect; // Texture source rectangle
        Vec2I mVideoSize;      // Decoded frame size in pixels

        Mesh mMesh; // Drawing mesh

        TextureRef  mTexture;     // Dynamic frame texture
        Ref<Bitmap> mFrameBitmap; // Reused RGBA frame bitmap

        Ref<Material> mChromaMaterial; // Chroma-key material (runtime)

        Ref<VideoDecoder> mDecoder;        // Decoder backend over asset data or file
        bool              mDecoderFailed = false; // True when decoder creation failed, to not retry every frame

        float mFrameTime = -1.0f; // Time of the last decoded frame in seconds

    protected:
        // Called when basis was changed
        void BasisChanged() override;

        // Called when color was changed
        void OnColorChanged() override;

        // Called when material was changed (propagates to mesh)
        void OnMaterialChanged() override;

        // Decodes and uploads the frame at the current animation time (IAnimation)
        void Evaluate() override;

        // Updates mesh geometry
        void UpdateMesh();

        // Builds the quad mesh
        void BuildDefaultMesh();

        // Creates the decoder (memory or streaming) and the frame texture
        void CreateDecoder();

        // Destroys the decoder
        void ReleaseDecoder();

        // Creates the dynamic frame texture and bitmap at video size
        void CreateFrameResources();

        // Updates animation duration from the video and resets bounds
        void UpdateDuration();

        // Rebuilds or updates the chroma-key material and applies it
        void UpdateChromaMaterial();

        friend class Render;
    };
}
// --- META ---

CLASS_BASES_META(o2::Video)
{
    BASE_CLASS(o2::IRectDrawable);
    BASE_CLASS(o2::IAnimation);
}
END_META;
CLASS_FIELDS_META(o2::Video)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(video);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(streaming);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(playOnAwake);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(chromaKey);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(keyColor);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 1).SCRIPTABLE_ATTRIBUTE().NAME(similarity);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 1).SCRIPTABLE_ATTRIBUTE().NAME(smoothness);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 1).SCRIPTABLE_ATTRIBUTE().NAME(spill);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 1).SCRIPTABLE_ATTRIBUTE().NAME(choke);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mVideoAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mStreaming);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mPlayOnAwake);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mChromaKeyEnabled);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::Green()).NAME(mKeyColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.05f).NAME(mSimilarity);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.45f).NAME(mSmoothness);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mSpill);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mChoke);
    FIELD().PROTECTED().NAME(mTextureSrcRect);
    FIELD().PROTECTED().NAME(mVideoSize);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().NAME(mTexture);
    FIELD().PROTECTED().NAME(mFrameBitmap);
    FIELD().PROTECTED().NAME(mChromaMaterial);
    FIELD().PROTECTED().NAME(mDecoder);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDecoderFailed);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mFrameTime);
}
END_META;
CLASS_METHODS_META(o2::Video)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetRef<VideoAsset>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Video&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetVideoAsset, const AssetRef<VideoAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(AssetRef<VideoAsset>, GetVideoAsset);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetStreaming, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsStreaming);
    FUNCTION().PUBLIC().SIGNATURE(const TextureRef&, GetTexture);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetVideoSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPlayOnAwake, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsPlayOnAwake);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetChromaKeyEnabled, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsChromaKeyEnabled);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetKeyColor, const Color4&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Color4, GetKeyColor);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSimilarity, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetSimilarity);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSmoothness, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetSmoothness);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSpill, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetSpill);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetChoke, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetChoke);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<Video>&);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnColorChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMaterialChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildDefaultMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateDecoder);
    FUNCTION().PROTECTED().SIGNATURE(void, ReleaseDecoder);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateFrameResources);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDuration);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateChromaMaterial);
}
END_META;
// --- END META ---
