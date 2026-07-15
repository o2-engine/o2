#include "o2/stdafx.h"
#include "ImageCompressor.h"

#include "AstcCompressor.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/DdsFormat.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    void ImageCompressor::CompressImage(const String& path, const String& outPath, TextureCompression compression, int quality)
    {
        if (compression == TextureCompression::None)
            return; // uncompressed pngs are used as is

        o2Debug.Log("Compress image from " + path + " to " + outPath + " compression " +
                    o2Reflection.GetEnumName(compression) + " quality " + (String)quality);

        String command = mConfig.formatCommands[::GetEnginePlatform()][compression];
        if (command.IsEmpty())
        {
            // No external tool configured for this host: use the built-in encoders, so
            // compression works the same on every platform
            Bitmap bitmap;
            if (!bitmap.Load(path, Bitmap::ImageType::Png))
            {
                o2Debug.LogError("Failed to load image for compression: " + path);
                return;
            }

            TextureFormat format = Texture::FormatOfCompression(compression);
            String extension = Texture::formatFileExtensions.Get(format);

            bool saved = compression == TextureCompression::ASTC4x4
                ? SaveAstc4x4(bitmap, outPath + "." + extension, quality)
                : SaveDds(bitmap, outPath + "." + extension, format, quality);

            if (saved)
                o2FileSystem.FileDelete(path);
            else
                o2Debug.LogError("Built-in compression failed for " + path);

            return;
        }

        if (::GetEnginePlatform() == Platform::Windows)
            command = "\"" + command + "\"";

        command.ReplaceAll("{quality}", String(quality));
        command.ReplaceAll("{input}", path);
        command.ReplaceAll("{output}", outPath);

        o2Debug.Log("Run compress command:" + command);
        int res = system(command.c_str());

        if (res != 0)
            o2Debug.Log("Something wrong, non-zero result");

        o2FileSystem.FileDelete(path);
    }

    void ImageCompressor::LoadConfig(const String& path)
    {
        DataDocument doc;
        o2Debug.Log("Load compressions config: " + path);
        doc.LoadFromFile(path);
        mConfig = doc;
    }

    void ImageCompressor::GenerateDefaultConfig()
    {
        mConfig.formatCommands =
        {
            { 
                Platform::Windows, 
                {
                    { TextureCompression::DXT5, "\"../../deps/o2/AssetsBuildTool/Bin/nvcompress.exe\" -nomips -bc3 -alpha \"{input}\" \"{output}.dds\"" }
                } 
            }
        };

        DataDocument doc;
        doc = mConfig;
        doc.SaveToFile(::GetProjectRootPath() + String("deps/o2/CompressToolsConfig.json"));
    }

    ImageCompressor::Config ImageCompressor::mConfig;
}
// --- META ---

DECLARE_CLASS(o2::ImageCompressor::Config, o2__ImageCompressor__Config);
// --- END META ---
