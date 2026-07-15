#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class ImageCompressor
    {
    public:
        // Compresses image
        static void CompressImage(const String& path, const String& outPath, TextureCompression compression, int quality);

        // Loads config
        static void LoadConfig(const String& path);

        // Generates default config
        static void GenerateDefaultConfig();

    public:
        // -------------------------
        // Compressions tools config
        // -------------------------
        struct Config : public ISerializable
        {
            // The format is:
            // path/to/executable custom parameters {quality 0-100} {input} {output}

            Map<Platform, Map<TextureCompression, String>> formatCommands; // Compression tool commands per host platform @SERIALIZABLE

            SERIALIZABLE(Config);
        };

    public:
        static Config mConfig;
    };
}
// --- META ---

CLASS_BASES_META(o2::ImageCompressor::Config)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::ImageCompressor::Config)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(formatCommands);
}
END_META;
CLASS_METHODS_META(o2::ImageCompressor::Config)
{
}
END_META;
// --- END META ---
