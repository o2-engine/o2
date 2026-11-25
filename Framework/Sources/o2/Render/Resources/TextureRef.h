#pragma once

#include "o2/Render/Resources/Texture.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class TextureRef : public IObject
    {
    public:
        // Default constructor, no reference
        TextureRef();

        // Nullptr constructor
        TextureRef(std::nullptr_t);

        // Copy constructor from other reference
        TextureRef(const TextureRef& other);

        // Move constructor from other reference
        TextureRef(TextureRef&& other) noexcept;

        // Constructor with texture pointer
        explicit TextureRef(Texture* ptr);

        // Constructor with size and format
        TextureRef(const Vec2I& size,
                   TextureFormat format = TextureFormat::R8G8B8A8,
                   Texture::Usage usage = Texture::Usage::Default);

        // Constructor from file @SCRIPTABLE
        explicit TextureRef(const String& fileName);

        // Constructor from bitmap
        explicit TextureRef(const Bitmap& bitmap);

        // Constructor from atlas page
        TextureRef(UID atlasAssetId, int page);

        // Constructor from atlas page
        TextureRef(const String& atlasAssetName, int page);

        // Copy constructor from other reference
        TextureRef(const Ref<Texture>& other);

        // Move constructor from other reference
        TextureRef(Ref<Texture>&& other);

        // Equality operator
        bool operator==(const TextureRef& other) const;

        // Equality operator
        bool operator==(const Texture* other) const;

        // Inequality operator
        bool operator!=(const TextureRef& other) const;

        // Inequality operator
        bool operator!=(const Texture* other) const;

        // Copy operator from other texture reference
        TextureRef& operator=(const TextureRef& other);

        // Move operator from other texture reference
        TextureRef& operator=(TextureRef&& other);

        // Move operator from other texture reference
        TextureRef& operator=(Ref<Texture>&& other);

        // Move operator from nullptr
        TextureRef& operator=(std::nullptr_t);

        // Less operator
        bool operator<(const TextureRef& other) const;

        // Returns is reference is valid
        bool IsValid() const;

        // Returns is reference is valid
        explicit operator bool() const;

        // Returns texture reference
        operator Ref<Texture>() const;

        // Returns texture reference
        Texture& operator*() const;

        // Returns texture pointer
        Texture* operator->() const;

        // Returns texture pointer 
        Texture* Get();

        // Returns texture pointer
        const Texture* Get() const;

        // Returns reference
        Ref<Texture>& GetRef();

        // Returns reference
        const Ref<Texture>& GetRef() const;

        // Returns empty texture
        static TextureRef Null();

        IOBJECT(TextureRef);

    protected:
        Ref<Texture> mTexture;

    protected:
        friend class Render;
        friend class Texture;
    };
}
// --- META ---

CLASS_BASES_META(o2::TextureRef)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(o2::TextureRef)
{
    FIELD().PROTECTED().NAME(mTexture);
}
END_META;
CLASS_METHODS_META(o2::TextureRef)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(std::nullptr_t);
    FUNCTION().PUBLIC().CONSTRUCTOR(const TextureRef&);
    FUNCTION().PUBLIC().CONSTRUCTOR(TextureRef&&);
    FUNCTION().PUBLIC().CONSTRUCTOR(Texture*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2I&, TextureFormat, Texture::Usage);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Bitmap&);
    FUNCTION().PUBLIC().CONSTRUCTOR(UID, int);
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, int);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<Texture>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(Ref<Texture>&&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsValid);
    FUNCTION().PUBLIC().SIGNATURE(Texture*, Get);
    FUNCTION().PUBLIC().SIGNATURE(const Texture*, Get);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Texture>&, GetRef);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Texture>&, GetRef);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(TextureRef, Null);
}
END_META;
// --- END META ---
