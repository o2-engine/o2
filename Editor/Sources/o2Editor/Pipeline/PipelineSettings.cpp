#include "o2Editor/stdafx.h"
#include "PipelineSettings.h"

#include "o2/EngineSettings.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#include <cstdlib>

namespace Editor
{
    static String EnvOrEmpty(const char* name)
    {
        const char* v = std::getenv(name);
        return v ? String(v).Trimed(" \n\r\t") : String();
    }

    String PipelineSettings::GetSettingsPath()
    {
        return PipelineUtils::GetWorkPath() + "PipelineSettings.json";
    }

    PipelineSettings PipelineSettings::Load()
    {
        PipelineSettings res;
        DataDocument doc;
        if (!o2FileSystem.IsFileExist(GetSettingsPath()) || !doc.LoadFromFile(GetSettingsPath()))
            return res;

        auto read = [&](const char* key) -> String
        {
            auto v = doc.FindMember(key);
            return v && v->IsString() ? String(v->GetString()) : String();
        };

        res.geminiApiKey = read("gemini");
        res.klingAccessKey = read("klingAccessKey");
        res.klingSecretKey = read("klingSecretKey");
        res.elevenLabsApiKey = read("elevenlabs");
        return res;
    }

    void PipelineSettings::Save() const
    {
        DataDocument doc;
        doc.SetObject();
        doc["gemini"] = geminiApiKey;
        doc["klingAccessKey"] = klingAccessKey;
        doc["klingSecretKey"] = klingSecretKey;
        doc["elevenlabs"] = elevenLabsApiKey;

        o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
        doc.SaveToFile(GetSettingsPath());
    }

    String PipelineSettings::GetGeminiKey() const
    {
        if (!geminiApiKey.Trimed().IsEmpty())
            return geminiApiKey.Trimed();

        String env = EnvOrEmpty("GEMINI_API_KEY");
        if (!env.IsEmpty())
            return env;

        String keyFile = String(GetProjectRootPath()) + "o2/Tools/ImageGen/api_key.txt";
        if (o2FileSystem.IsFileExist(keyFile))
            return PipelineUtils::ReadFileBytes(keyFile).Trimed(" \n\r\t");

        return "";
    }

    String PipelineSettings::GetElevenLabsKey() const
    {
        if (!elevenLabsApiKey.Trimed().IsEmpty())
            return elevenLabsApiKey.Trimed();

        return EnvOrEmpty("ELEVENLABS_API_KEY");
    }

    String PipelineSettings::GetKlingAccessKey() const
    {
        if (!klingAccessKey.Trimed().IsEmpty())
            return klingAccessKey.Trimed();

        return EnvOrEmpty("KLING_ACCESS_KEY");
    }

    String PipelineSettings::GetKlingSecretKey() const
    {
        if (!klingSecretKey.Trimed().IsEmpty())
            return klingSecretKey.Trimed();

        return EnvOrEmpty("KLING_SECRET_KEY");
    }
}
