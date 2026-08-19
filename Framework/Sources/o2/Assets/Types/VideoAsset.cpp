#include "o2/stdafx.h"
#include "VideoAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include "pl_mpeg.h"

namespace o2
{
    VideoAsset::VideoAsset()
    {}

    VideoAsset::VideoAsset(const VideoAsset& other):
        AssetWithDefaultMeta<VideoAsset>(other)
    {
        if (other.mDataSize > 0)
        {
            mDataSize = other.mDataSize;
            mData = mnew char[mDataSize];
            memcpy(mData, other.mData, mDataSize);
        }

        mDuration = other.mDuration;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mFrameRate = other.mFrameRate;
    }

    VideoAsset::~VideoAsset()
    {
        if (mData)
            delete[] mData;
    }

    VideoAsset& VideoAsset::operator=(const VideoAsset& other)
    {
        Asset::operator=(other);

        if (mData)
            delete[] mData;

        if (other.mDataSize > 0)
        {
            mDataSize = other.mDataSize;
            mData = mnew char[mDataSize];
            memcpy(mData, other.mData, mDataSize);
        }
        else
        {
            mDataSize = 0;
            mData = nullptr;
        }

        mDuration = other.mDuration;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mFrameRate = other.mFrameRate;

        return *this;
    }

    char* VideoAsset::GetData() const
    {
        return mData;
    }

    UInt VideoAsset::GetDataSize() const
    {
        return mDataSize;
    }

    void VideoAsset::SetData(char* data, UInt size)
    {
        if (mData)
            delete[] mData;

        if (size > 0)
        {
            mDataSize = size;
            mData = mnew char[mDataSize];
            memcpy(mData, data, mDataSize);
        }
        else
        {
            mDataSize = 0;
            mData = nullptr;
        }

        mDuration = -1.0f;
        mWidth = 0;
        mHeight = 0;
        mFrameRate = 0.0f;
    }

    float VideoAsset::GetDuration() const
    {
        if (mDuration < 0.0f)
            ParseFormatInfo();

        return Math::Max(mDuration, 0.0f);
    }

    Vec2I VideoAsset::GetImageSize() const
    {
        if (mDuration < 0.0f)
            ParseFormatInfo();

        return Vec2I(mWidth, mHeight);
    }

    float VideoAsset::GetFrameRate() const
    {
        if (mDuration < 0.0f)
            ParseFormatInfo();

        return mFrameRate;
    }

    Vector<String> VideoAsset::GetFileExtensions()
    {
        return { "mpg", "mpeg", "mp4", "mov", "m4v" };
    }

    void VideoAsset::LoadData(const String& path)
    {
        InFile file(path);
        if (!file.IsOpened())
            GetAssetsLogStream()->Error("Failed to load video asset data: can't open file " + path);

        mDataSize = file.GetDataSize();
        mData = mnew char[mDataSize];
        file.ReadFullData(mData);

        mDuration = -1.0f;
        mWidth = 0;
        mHeight = 0;
        mFrameRate = 0.0f;
    }

    void VideoAsset::SaveData(const String& path) const
    {
        OutFile file(path);
        if (mDataSize > 0 && mData)
            file.WriteData(mData, mDataSize);
    }

    void VideoAsset::ParseFormatInfo() const
    {
        mDuration = 0.0f;
        mWidth = 0;
        mHeight = 0;
        mFrameRate = 0.0f;

        String ext = FileSystem::GetFileExtension(GetPath());
        bool hardwareFormat = ext == "mp4" || ext == "mov" || ext == "m4v";

        // In-memory MPEG-1 bytes parse directly; hardware formats and file-only assets parse from the built file
        if (mData && mDataSize > 0 && !hardwareFormat)
        {
            plm_t* plm = plm_create_with_memory((uint8_t*)mData, (size_t)mDataSize, 0 /* don't free, asset owns data */);
            if (!plm)
            {
                GetAssetsLogStream()->Error("Failed to decode video asset: " + GetPath());
                return;
            }

            plm_set_audio_enabled(plm, 0);

            mWidth = plm_get_width(plm);
            mHeight = plm_get_height(plm);
            mFrameRate = (float)plm_get_framerate(plm);
            mDuration = (float)plm_get_duration(plm);

            plm_destroy(plm);
            return;
        }

        Vec2I size;
        float frameRate = 0.0f, duration = 0.0f;
        if (ParseVideoFileFormatInfo(GetBuiltFullPath(), size, frameRate, duration))
        {
            mWidth = size.x;
            mHeight = size.y;
            mFrameRate = frameRate;
            mDuration = duration;
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::VideoAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::VideoAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::VideoAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::VideoAsset>>);
// --- META ---

DECLARE_CLASS(o2::VideoAsset, o2__VideoAsset);
// --- END META ---
