#include "o2/stdafx.h"
#include "SoundAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Debug/Log/LogStream.h"

#include "miniaudio.h"

namespace o2
{
    SoundAsset::SoundAsset()
    {}

    SoundAsset::SoundAsset(const SoundAsset& other):
        AssetWithDefaultMeta<SoundAsset>(other)
    {
        if (other.mDataSize > 0)
        {
            mDataSize = other.mDataSize;
            mData = mnew char[mDataSize];
            memcpy(mData, other.mData, mDataSize);
        }

        mDuration = other.mDuration;
        mChannels = other.mChannels;
        mSampleRate = other.mSampleRate;
    }

    SoundAsset::~SoundAsset()
    {
        if (mData)
            delete[] mData;
    }

    SoundAsset& SoundAsset::operator=(const SoundAsset& other)
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
        mChannels = other.mChannels;
        mSampleRate = other.mSampleRate;

        return *this;
    }

    char* SoundAsset::GetData() const
    {
        return mData;
    }

    UInt SoundAsset::GetDataSize() const
    {
        return mDataSize;
    }

    void SoundAsset::SetData(char* data, UInt size)
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
        mChannels = 0;
        mSampleRate = 0;
    }

    float SoundAsset::GetDuration() const
    {
        if (mDuration < 0.0f)
            ParseFormatInfo();

        return Math::Max(mDuration, 0.0f);
    }

    int SoundAsset::GetChannelsCount() const
    {
        if (mDuration < 0.0f)
            ParseFormatInfo();

        return mChannels;
    }

    int SoundAsset::GetSampleRate() const
    {
        if (mDuration < 0.0f)
            ParseFormatInfo();

        return mSampleRate;
    }

    Vector<String> SoundAsset::GetFileExtensions()
    {
        return { "wav", "ogg", "mp3", "flac" };
    }

    void SoundAsset::LoadData(const String& path)
    {
        InFile file(path);
        if (!file.IsOpened())
            GetAssetsLogStream()->Error("Failed to load sound asset data: can't open file " + path);

        mDataSize = file.GetDataSize();
        mData = mnew char[mDataSize];
        file.ReadFullData(mData);

        mDuration = -1.0f;
        mChannels = 0;
        mSampleRate = 0;
    }

    void SoundAsset::SaveData(const String& path) const
    {
        OutFile file(path);
        if (mDataSize > 0 && mData)
            file.WriteData(mData, mDataSize);
    }

    void SoundAsset::ParseFormatInfo() const
    {
        mDuration = 0.0f;
        mChannels = 0;
        mSampleRate = 0;

        if (!mData || mDataSize == 0)
            return;

        ma_decoder decoder;
        ma_decoder_config config = ma_decoder_config_init_default();
        if (ma_decoder_init_memory(mData, mDataSize, &config, &decoder) != MA_SUCCESS)
        {
            GetAssetsLogStream()->Error("Failed to decode sound asset: " + GetPath());
            return;
        }

        mChannels = (int)decoder.outputChannels;
        mSampleRate = (int)decoder.outputSampleRate;

        ma_uint64 lengthInFrames = 0;
        if (ma_decoder_get_length_in_pcm_frames(&decoder, &lengthInFrames) == MA_SUCCESS && decoder.outputSampleRate > 0)
            mDuration = (float)((double)lengthInFrames/(double)decoder.outputSampleRate);

        ma_decoder_uninit(&decoder);
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::SoundAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::SoundAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::SoundAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::SoundAsset>>);
// --- META ---

DECLARE_CLASS(o2::SoundAsset, o2__SoundAsset);
// --- END META ---
