#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"

namespace o2
{
    // --------------------------------------------
    // Sound asset. Stores encoded audio file data
    // --------------------------------------------
    class SoundAsset: public AssetWithDefaultMeta<SoundAsset>
    {
    public:
        PROPERTIES(SoundAsset);
        GETTER(char*, data, GetData);        // Data getter
        GETTER(UInt, dataSize, GetDataSize); // Data size getter
        GETTER(float, duration, GetDuration); // Duration getter

    public:
        // Default constructor
        SoundAsset();

        // Copy-constructor
        SoundAsset(const SoundAsset& asset);

        // Destructor
        ~SoundAsset();

        // Check equals operator
        SoundAsset& operator=(const SoundAsset& asset);

        // Returns encoded audio data pointer
        char* GetData() const;

        // Returns encoded audio data size
        UInt GetDataSize() const;

        // Sets encoded audio data
        void SetData(char* data, UInt size);

        // Returns sound duration in seconds
        float GetDuration() const;

        // Returns channels count
        int GetChannelsCount() const;

        // Returns sample rate
        int GetSampleRate() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 95; }

        SERIALIZABLE(SoundAsset);
        CLONEABLE_REF(SoundAsset);

    protected:
        char* mData = nullptr; // Encoded audio data
        UInt  mDataSize = 0;   // Encoded audio data size

        mutable float mDuration = -1.0f; // Decoded duration in seconds, -1 if not parsed yet
        mutable int   mChannels = 0;     // Channels count, 0 if not parsed yet
        mutable int   mSampleRate = 0;   // Sample rate, 0 if not parsed yet

    protected:
        // Loads asset data from file
        void LoadData(const String& path) override;

        // Saves asset data to file
        void SaveData(const String& path) const override;

        // Decodes format info: duration, channels, sample rate
        void ParseFormatInfo() const;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::SoundAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<SoundAsset>);
}
END_META;
CLASS_FIELDS_META(o2::SoundAsset)
{
    FIELD().PUBLIC().NAME(data);
    FIELD().PUBLIC().NAME(dataSize);
    FIELD().PUBLIC().NAME(duration);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mData);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDataSize);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mDuration);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mChannels);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSampleRate);
}
END_META;
CLASS_METHODS_META(o2::SoundAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SoundAsset&);
    FUNCTION().PUBLIC().SIGNATURE(char*, GetData);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetDataSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetData, char*, UInt);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(int, GetChannelsCount);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSampleRate);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, ParseFormatInfo);
}
END_META;
// --- END META ---
