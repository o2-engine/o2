#pragma once

#if defined PLATFORM_WINDOWS
#include "o2/Render/Windows/MaterialBase.h"
#elif defined PLATFORM_ANDROID
// #include "o2/Render/Android/MaterialBase.h"
#elif defined PLATFORM_MAC
// #include "o2/Render/Mac/MaterialBase.h"
#elif defined PLATFORM_IOS
// #include "o2/Render/iOS/MaterialBase.h"
#elif defined(PLATFORM_LINUX)
// #include "o2/Render/Linux/MaterialBase.h"
#endif

#include "o2/Render/Shader.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"

#include <functional>

namespace o2
{
    class Render;
    class Material;

    // -----------------------------------------------------------------------
    // Shader parameter interface. Stores a named value to be passed
    // as a uniform to the GPU shader program. Supports serialization
    // so material parameters can be saved/loaded through the asset system.
    // -----------------------------------------------------------------------
    class IShaderParam : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        PROPERTIES(IShaderParam);
        PROPERTY(String, name, SetName, GetName); // Uniform name property

    public:
        // Virtual destructor
        virtual ~IShaderParam() {}

        // Returns the uniform name this parameter is bound to
        const String& GetName() const;

        // Sets the uniform name this parameter is bound to
        void SetName(const String& name);

        // Computes a hash of the parameter name and value for batch comparison
        virtual size_t ComputeHash() const;

        SERIALIZABLE(IShaderParam);
        CLONEABLE_REF(IShaderParam);

    protected:
        String mName; // Uniform name in the shader program @SERIALIZABLE
    };

    // -------------------------------------------------------
    // Float shader parameter. Maps to a GLSL float uniform.
    // -------------------------------------------------------
    class ShaderParamFloat : public IShaderParam
    {
    public:
        PROPERTIES(ShaderParamFloat);
        PROPERTY(float, value, SetValue, GetValue);  // Float value property

    public:
        // Default constructor
        ShaderParamFloat();

        // Constructor with uniform name and initial value
        ShaderParamFloat(const String& name, float value);

        // Returns the float value
        float GetValue() const;

        // Sets the float value
        void SetValue(float value);

        // Computes hash from name and float value
        size_t ComputeHash() const override;

        SERIALIZABLE(ShaderParamFloat);
        CLONEABLE_REF(ShaderParamFloat);

    protected:
        float mValue = 0.0f; // Float uniform value @SERIALIZABLE
    };

    // -------------------------------------------------------
    // Vec2 shader parameter. Maps to a GLSL vec2 uniform.
    // -------------------------------------------------------
    class ShaderParamVec2 : public IShaderParam
    {
    public:
        PROPERTIES(ShaderParamVec2);
        PROPERTY(Vec2F, value, SetValue, GetValue);    // Vec2 value property

    public:
        // Default constructor
        ShaderParamVec2();

        // Constructor with uniform name and initial value
        ShaderParamVec2(const String& name, const Vec2F& value);

        // Returns the Vec2F value
        const Vec2F& GetValue() const;

        // Sets the Vec2F value
        void SetValue(const Vec2F& value);

        // Computes hash from name and vec2 value
        size_t ComputeHash() const override;

        SERIALIZABLE(ShaderParamVec2);
        CLONEABLE_REF(ShaderParamVec2);

    protected:
        Vec2F mValue; // Vec2 uniform value @SERIALIZABLE
    };

    // -------------------------------------------------------
    // Color shader parameter. Maps to a GLSL vec4 uniform
    // representing an RGBA color.
    // -------------------------------------------------------
    class ShaderParamColor : public IShaderParam
    {
    public:
        PROPERTIES(ShaderParamColor);
        PROPERTY(Color4, value, SetValue, GetValue);     // Color value property

    public:
        // Default constructor
        ShaderParamColor();

        // Constructor with uniform name and initial color value
        ShaderParamColor(const String& name, const Color4& value);

        // Returns the Color4 value
        const Color4& GetValue() const;

        // Sets the Color4 value
        void SetValue(const Color4& value);

        // Computes hash from name and color ABGR representation
        size_t ComputeHash() const override;

        SERIALIZABLE(ShaderParamColor);
        CLONEABLE_REF(ShaderParamColor);

    protected:
        Color4 mValue = Color4::White(); // Color uniform value @SERIALIZABLE
    };

    // -------------------------------------------------------
    // Integer shader parameter. Maps to a GLSL int uniform.
    // -------------------------------------------------------
    class ShaderParamInt : public IShaderParam
    {
    public:
        PROPERTIES(ShaderParamInt);
        PROPERTY(int, value, SetValue, GetValue);    // Int value property

    public:
        // Default constructor
        ShaderParamInt();

        // Constructor with uniform name and initial value
        ShaderParamInt(const String& name, int value);

        // Returns the integer value
        int GetValue() const;

        // Sets the integer value
        void SetValue(int value);

        // Computes hash from name and int value
        size_t ComputeHash() const override;

        SERIALIZABLE(ShaderParamInt);
        CLONEABLE_REF(ShaderParamInt);

    protected:
        int mValue = 0; // Int uniform value @SERIALIZABLE
    };

    // -----------------------------------------------------------------------
    // Material render primitive. Combines a vertex shader, a fragment shader,
    // an optional texture, and a set of shader parameters into a single
    // rendering state. Links shaders into a GPU program and caches uniform
    // and attribute locations. Provides a hash for efficient batch comparison.
    // -----------------------------------------------------------------------
    class Material : public MaterialBase, public RefCounterable
    {
    public:
        PROPERTIES(Material);
        GETTER(bool, ready, IsReady);                                // Ready state getter
        PROPERTY(TextureRef, texture, SetTexture, GetTexture);      // Material texture property
        PROPERTY(BlendMode, blendMode, SetBlendMode, GetBlendMode);   // Blend mode for rendering @SCRIPTABLE

    public:
        // Default constructor
        Material();

        // Copy-constructor, clones parameters and rebuilds the program
        Material(const Material& other);

        // Destructor, releases GPU program
        ~Material();

        // Sets the vertex shader. Invalidates the compiled program
        void SetVertexShader(const Ref<Shader>& shader);

        // Returns the vertex shader reference
        const Ref<Shader>& GetVertexShader() const;

        // Sets the fragment shader. Invalidates the compiled program
        void SetFragmentShader(const Ref<Shader>& shader);

        // Returns the fragment shader reference
        const Ref<Shader>& GetFragmentShader() const;

        // Sets the material texture
        void SetTexture(const TextureRef& texture);

        // Returns the material texture
        const TextureRef& GetTexture() const;

        // Sets blend mode for rendering
        void SetBlendMode(BlendMode blendMode);

        // Returns blend mode
        BlendMode GetBlendMode() const;

        // Adds a shader parameter, replacing any existing parameter with the same name
        void AddParam(const Ref<IShaderParam>& param);

        // Removes a shader parameter by uniform name
        void RemoveParam(const String& name);

        // Returns all shader parameters
        const Vector<Ref<IShaderParam>>& GetParams() const;

        // Replaces all shader parameters
        void SetParams(const Vector<Ref<IShaderParam>>& params);

        // Links the vertex and fragment shaders into a GPU program. Returns true on success
        bool Build();

        // Returns true when the shader program is compiled and ready for rendering
        bool IsReady() const;

        // Returns the cached material hash, recomputing if dirty
        size_t GetHash();

        // Marks the hash as needing recomputation (call after changing params at runtime)
        void InvalidateHash();

        // Applies custom shader parameters as uniforms on the currently active program
        void ApplyParams() const;

        // Returns transform matrix uniform location
        int GetTransformUniform() const;

        // Returns texture sampler uniform location
        int GetTextureUniform() const;

        // Returns vertex position attribute location
        int GetPositionAttribute() const;

        // Returns vertex color attribute location
        int GetColorAttribute() const;

        // Returns texture coordinate attribute location
        int GetTexCoordsAttribute() const;

    protected:
		Ref<Shader> mVertexShader;   // Vertex shader reference
		Ref<Shader> mFragmentShader; // Fragment shader reference

        TextureRef mTexture; // Optional material texture

        BlendMode mBlendMode = BlendMode::Normal; // Blend mode for rendering

        Vector<Ref<IShaderParam>> mParams; // Shader parameter list

        size_t mHash = 0;         // Cached material state hash
        bool   mHashDirty = true; // True when hash needs recomputation
        bool   mReady = false;    // True when program is linked successfully

    protected:
        // Called after reference creation
        void PostRefConstruct();

        // Platform-specific program linking
        bool PlatformBuild();

        // Platform-specific program destruction
        void PlatformDestroy();

        // Platform-specific uniform application
        void PlatformApplyParams() const;

        // Computes the material state hash using std::hash
        size_t ComputeHash() const;

        friend class Render;
        friend class Ref<Material>;
        FRIEND_REF_MAKE();
    };
}
// --- META ---

CLASS_BASES_META(o2::IShaderParam)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::IShaderParam)
{
    FIELD().PUBLIC().NAME(name);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mName);
}
END_META;
CLASS_METHODS_META(o2::IShaderParam)
{

    FUNCTION().PUBLIC().SIGNATURE(const String&, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(size_t, ComputeHash);
}
END_META;

CLASS_BASES_META(o2::ShaderParamFloat)
{
    BASE_CLASS(o2::IShaderParam);
}
END_META;
CLASS_FIELDS_META(o2::ShaderParamFloat)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mValue);
}
END_META;
CLASS_METHODS_META(o2::ShaderParamFloat)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, float);
    FUNCTION().PUBLIC().SIGNATURE(size_t, ComputeHash);
}
END_META;

CLASS_BASES_META(o2::ShaderParamVec2)
{
    BASE_CLASS(o2::IShaderParam);
}
END_META;
CLASS_FIELDS_META(o2::ShaderParamVec2)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mValue);
}
END_META;
CLASS_METHODS_META(o2::ShaderParamVec2)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vec2F&, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(size_t, ComputeHash);
}
END_META;

CLASS_BASES_META(o2::ShaderParamColor)
{
    BASE_CLASS(o2::IShaderParam);
}
END_META;
CLASS_FIELDS_META(o2::ShaderParamColor)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mValue);
}
END_META;
CLASS_METHODS_META(o2::ShaderParamColor)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(size_t, ComputeHash);
}
END_META;

CLASS_BASES_META(o2::ShaderParamInt)
{
    BASE_CLASS(o2::IShaderParam);
}
END_META;
CLASS_FIELDS_META(o2::ShaderParamInt)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mValue);
}
END_META;
CLASS_METHODS_META(o2::ShaderParamInt)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, int);
    FUNCTION().PUBLIC().SIGNATURE(size_t, ComputeHash);
}
END_META;
// --- END META ---
