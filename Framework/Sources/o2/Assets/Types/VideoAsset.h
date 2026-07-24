#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
    // --------------------------------------------------
    // Video asset. Stores encoded MPEG-1 video file data
    // --------------------------------------------------
    class VideoAsset: public AssetWithDefaultMeta<VideoAsset>
    {
    public:
        PROPERTIES(VideoAsset);
        GETTER(char*, data, GetData);         // Data getter
        GETTER(UInt, dataSize, GetDataSize);  // Data size getter
        GETTER(float, duration, GetDuration); // Duration getter

    public:
        // Default constructor
        VideoAsset();

        // Copy-constructor
        VideoAsset(const VideoAsset& asset);

        // Destructor
        ~VideoAsset();

        // Check equals operator
        VideoAsset& operator=(const VideoAsset& asset);

        // Returns encoded video data pointer
        char* GetData() const;

        // Returns encoded video data size
        UInt GetDataSize() const;

        // Sets encoded video data
        void SetData(char* data, UInt size);

        // Returns video duration in seconds
        float GetDuration() const;

        // Returns video frame size in pixels
        Vec2I GetImageSize() const;

        // Returns video frame rate in frames per second
        float GetFrameRate() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 94; }

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_video_icon.png"; }

        SERIALIZABLE(VideoAsset);
        CLONEABLE_REF(VideoAsset);

    protected:
        char* mData = nullptr; // Encoded video data
        UInt  mDataSize = 0;   // Encoded video data size

        mutable float mDuration = -1.0f; // Decoded duration in seconds, -1 if not parsed yet
        mutable int   mWidth = 0;        // Frame width in pixels, 0 if not parsed yet
        mutable int   mHeight = 0;       // Frame height in pixels, 0 if not parsed yet
        mutable float mFrameRate = 0.0f; // Frame rate, 0 if not parsed yet

    protected:
        // Loads asset data from file
        void LoadData(const String& path) override;

        // Saves asset data to file
        void SaveData(const String& path) const override;

        // Decodes format info: duration, size, frame rate
        void ParseFormatInfo() const;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::VideoAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<VideoAsset>);
}
END_META;
CLASS_FIELDS_META(o2::VideoAsset)
{
    FIELD().PUBLIC().NAME(data);
    FIELD().PUBLIC().NAME(dataSize);
    FIELD().PUBLIC().NAME(duration);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mData);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDataSize);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mDuration);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mHeight);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mFrameRate);
}
END_META;
CLASS_METHODS_META(o2::VideoAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const VideoAsset&);
    FUNCTION().PUBLIC().SIGNATURE(char*, GetData);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetDataSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetData, char*, UInt);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetImageSize);
    FUNCTION().PUBLIC().SIGNATURE(float, GetFrameRate);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, ParseFormatInfo);
}
END_META;
// --- END META ---
