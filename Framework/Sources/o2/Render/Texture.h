#pragma once

#if defined PLATFORM_WINDOWS
#include "o2/Render/Windows/TextureBase.h"
#elif defined PLATFORM_ANDROID
#include "o2/Render/Android/TextureBase.h"
#elif defined PLATFORM_MAC
#include "o2/Render/Mac/TextureBase.h"
#elif defined PLATFORM_IOS
#include "o2/Render/iOS/TextureBase.h"
#elif defined PLATFORM_WASM
#include "o2/Render/WebAssembly/TextureBase.h"
#elif defined(PLATFORM_LINUX)
#if defined(O2_RENDER_GLES2)
#include "o2/Render/Linux GLES2/TextureBase.h"
#else
#include "o2/Render/Linux/TextureBase.h"
#endif
#endif

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/System/Time/TimeStamp.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/UID.h"

namespace o2
{
    class Bitmap;

    // -------
    // Texture
    // -------
    class Texture : public TextureBase, public RefCounterable
    {
    public:
        // Texture usage
        enum class Usage { Default, RenderTarget };

        // Texture filter
        enum class Filter { Linear, Nearest };

        // Texture wrap mode (for U/V coordinates outside [0, 1])
        enum class Wrap { ClampToEdge, Repeat };

        static const Map<TextureFormat, String> formatFileExtensions; // Texture format file extensions

    public:
        PROPERTIES(Texture);
        GETTER(Vec2I, size, GetSize);             // Size of texture getter
        GETTER(TextureFormat, format, GetFormat); // texture format getter
        GETTER(Usage, usage, GetUsage);           // Texture usage getter
        GETTER(String, fileName, GetFileName);    // Texture file name getter

    public:
        // Default constructor
        Texture();

        // Constructor
        Texture(const Vec2I& size, TextureFormat format = TextureFormat::R8G8B8A8, Usage usage = Usage::Default);

        // Constructor from file
        Texture(const String& fileName);

        // Constructor from atlas page
        Texture(UID atlasAssetId, int page);

        // Constructor from atlas page
        Texture(const String& atlasAssetName, int page);

        // Constructor from bitmap
        Texture(const Bitmap& bitmap);

        // Destructor
        ~Texture();

        // Creates texture with size, format and usage
        void Create(const Vec2I& size, TextureFormat format = TextureFormat::R8G8B8A8, Usage usage = Usage::Default);

        // Creates texture with size, data, and format
        void Create(const Vec2I& size, Byte* data, TextureFormat format = TextureFormat::R8G8B8A8);

        // Creates texture from file
        void Create(const String& fileName);

        // Creates texture from atlas page
        void Create(UID atlasAssetId, int page);

        // Creates texture from atlas page
        void Create(const String& atlasAssetName, int page);

        // Creates texture from bitmap
        void Create(const Bitmap& bitmap);

        // Sets texture's data from bitmap
        void SetData(const Bitmap& bitmap);

        // Sets sub data from bitmap with offset
        void SetSubData(const Vec2I& offset, const Bitmap& bitmap);

        // Copies pixels from texture by rect
        void Copy(const Texture& from, const RectI& rect);

        // Gets texture's data to bitmap and returns it
        Ref<Bitmap> GetData();

        // Reloads texture
        void Reload();

        // Returns size of texture
        Vec2I GetSize() const;

        // Returns format
        TextureFormat GetFormat() const;

        // returns texture usage
        Usage GetUsage() const;

        // Returns texture file name
        String GetFileName() const;

        // Sets texture filter
        void SetFilter(Filter filter);

        // Returns texture filter
        Filter GetFilter() const;

        // Sets texture wrap mode (applies to both S and T axes)
        void SetWrap(Wrap wrap);

        // Returns texture wrap mode
        Wrap GetWrap() const;

        // Returns true when texture ready to use
        bool IsReady() const;

        // Returns is texture from atlas page
        bool IsAtlasPage() const;

        // Returns atlas asset id
        UID GetAtlasAssetId() const;

        // Returns atlas page
        int GetAtlasPage() const;

#if IS_EDITOR
        // Returns the file modification timestamp recorded at last load
        const TimeStamp& GetFileEditDate() const;

        // Sets the file modification timestamp
        void SetFileEditDate(const TimeStamp& date);
#endif

    protected:
        Vec2I mSize; // Size of texture

        Filter        mFilter = Filter::Linear;      // Min/Mag filter
        Wrap          mWrap = Wrap::ClampToEdge;     // S/T wrap mode
        TextureFormat mFormat;                       // Texture format
        Usage         mUsage;                        // Texture usage

        String mFileName; // Source file name

        UID mAtlasAssetId; // Atlas asset id. Equals 0 if it isn't atlas texture
        int mAtlasPage;    // Atlas page

        bool mReady; // Is texture ready to use

#if IS_EDITOR
        TimeStamp mFileEditDate; // File modification time at last load
#endif

    protected:
        // It is called after reference initialization at object construction, registers texture in render
        void PostRefConstruct();

        // Platform specific texture creating
        bool PlatformCreate();

        // Platform specific texture destroying
        void PlatformDestroy();

        // Platform specific texture uploading
        void PlatformUploadData(const Vec2I& size, Byte* data, TextureFormat format);

        // Platform specific texture uploading
        void PlatformUploadRegionData(const Vec2I& offset, const Vec2I& size, Byte* data, TextureFormat format);

        // Platform specific retrieving texture data
        void PlatformGetData(Byte* data);

        // Platform specific set filter
        void PlatformSetFilter();

        // Platform specific set wrap mode
        void PlatformSetWrap();

        // Loads texture from PNG file 
        void LoadPNG(const String& fileName);

        // Loads texture from DDS file
        void LoadDDS(const String& fileName);

        friend class Render;
        friend class TextureRef;

        FRIEND_REF_MAKE();
    };
}
// --- META ---

PRE_ENUM_META(o2::Texture::Usage);

PRE_ENUM_META(o2::Texture::Filter);

PRE_ENUM_META(o2::Texture::Wrap);
// --- END META ---
