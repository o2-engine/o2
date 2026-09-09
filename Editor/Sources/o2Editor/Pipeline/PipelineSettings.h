#pragma once

#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------
    // Provider API keys. Stored in Work/Pipelines/PipelineSettings.json,
    // environment variables and the ImageGen tool key file are fallbacks
    // ------------------------------------------------------------------
    struct PipelineSettings
    {
        String geminiApiKey;     // Google Gemini API key
        String klingAccessKey;   // Kling access key
        String klingSecretKey;   // Kling secret key
        String elevenLabsApiKey; // ElevenLabs API key

    public:
        // Loads settings from the settings file (missing file gives empty keys)
        static PipelineSettings Load();

        // Saves settings to the settings file, creating the work folder when needed
        void Save() const;

        // Returns path of the settings file
        static String GetSettingsPath();

        // Returns effective Gemini key: stored key, GEMINI_API_KEY or the ImageGen tool key file
        String GetGeminiKey() const;

        // Returns effective ElevenLabs key: stored key or ELEVENLABS_API_KEY
        String GetElevenLabsKey() const;

        // Returns effective Kling access key: stored key or KLING_ACCESS_KEY
        String GetKlingAccessKey() const;

        // Returns effective Kling secret key: stored key or KLING_SECRET_KEY
        String GetKlingSecretKey() const;
    };
}
