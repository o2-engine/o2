#pragma once

#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------
    // Provider API keys and local tools. Stored in
    // Work/Pipelines/PipelineSettings.json, environment variables and the
    // ImageGen tool key file are fallbacks
    // ------------------------------------------------------------------
    struct PipelineSettings
    {
        String geminiApiKey;     // Google Gemini API key
        String openAiApiKey;     // OpenAI API key
        String openRouterApiKey; // OpenRouter API key
        String klingAccessKey;   // Kling access key
        String klingSecretKey;   // Kling secret key
        String elevenLabsApiKey; // ElevenLabs API key

        String imageToUiPython; // Python 3 for AI image to UI, empty: the first python3 found

        bool   localAgentEnabled = true;   // The model menus offer the local agents found on this computer
        String localAgentPath;             // Claude Code executable, empty: found on PATH and in the usual install folders
        String localAgentModel;            // Model local:claude-code runs on (haiku, sonnet, opus or an id), empty: Claude Code's
        String localAgentApiKey;           // Anthropic API key Claude Code runs on in --bare mode, empty: its own login
        bool   localAgentThinking = false; // Let Claude Code think (more tokens); off sets MAX_THINKING_TOKENS=0
        String localCommand;               // Shell command of local:command: the prompt on stdin, the answer on stdout
        String localCodexPath;             // Codex executable, empty: found on PATH and in the usual install folders
        String localCodexModel;            // Model local:codex runs on, empty: Codex's own
        String localCodexEffort = "low";   // Codex reasoning effort (minimal, low, medium, high), empty: Codex's own
        String localCodexApiKey;           // OpenAI API key Codex runs on (API prices), empty: its ChatGPT login

    public:
        // Loads settings from the settings file (missing file gives empty keys)
        static PipelineSettings Load();

        // Saves settings to the settings file, creating the work folder when needed
        void Save() const;

        // Returns path of the settings file
        static String GetSettingsPath();

        // Returns effective Gemini key: stored key, GEMINI_API_KEY or the ImageGen tool key file
        String GetGeminiKey() const;

        // Returns effective OpenAI key: stored key or OPENAI_API_KEY
        String GetOpenAiKey() const;

        // Returns effective OpenRouter key: stored key or OPENROUTER_API_KEY
        String GetOpenRouterKey() const;

        // Returns effective ElevenLabs key: stored key or ELEVENLABS_API_KEY
        String GetElevenLabsKey() const;

        // Returns effective Kling access key: stored key or KLING_ACCESS_KEY
        String GetKlingAccessKey() const;

        // Returns effective Kling secret key: stored key or KLING_SECRET_KEY
        String GetKlingSecretKey() const;

        // Returns the Anthropic key Claude Code runs on: the stored key; empty: its own login (the environment's key is not
        // taken, so it never runs on API prices unasked)
        String GetLocalAgentApiKey() const;
    };
}
