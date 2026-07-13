#pragma once

#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Utils/Math/Math.h"

#include <vector>

namespace o2
{
    // Builds a PCM16 mono WAV file in memory
    inline std::vector<char> BuildTestWav(float duration, int sampleRate = 44100, float frequency = 440.0f)
    {
        int samplesCount = (int)(duration*(float)sampleRate);
        int dataSize = samplesCount*2;

        std::vector<char> result(44 + dataSize);
        char* out = result.data();

        auto writeBytes = [&](int offset, const void* data, int size) { memcpy(out + offset, data, size); };
        auto writeU32 = [&](int offset, unsigned int value) { writeBytes(offset, &value, 4); };
        auto writeU16 = [&](int offset, unsigned short value) { writeBytes(offset, &value, 2); };

        writeBytes(0, "RIFF", 4);
        writeU32(4, 36 + dataSize);
        writeBytes(8, "WAVE", 4);
        writeBytes(12, "fmt ", 4);
        writeU32(16, 16);
        writeU16(20, 1); // PCM
        writeU16(22, 1); // mono
        writeU32(24, sampleRate);
        writeU32(28, sampleRate*2);
        writeU16(32, 2);
        writeU16(34, 16);
        writeBytes(36, "data", 4);
        writeU32(40, dataSize);

        for (int i = 0; i < samplesCount; i++)
        {
            short sample = (short)(Math::Sin(2.0f*Math::PI()*frequency*(float)i/(float)sampleRate)*10000.0f);
            writeBytes(44 + i*2, &sample, 2);
        }

        return result;
    }

    // Creates sound asset with generated test WAV data
    inline Ref<SoundAsset> MakeTestSoundAsset(float duration, int sampleRate = 44100)
    {
        auto wav = BuildTestWav(duration, sampleRate);
        auto asset = mmake<SoundAsset>();
        asset->SetData(wav.data(), (UInt)wav.size());
        return asset;
    }
}
