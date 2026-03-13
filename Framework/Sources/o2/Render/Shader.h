#pragma once

#if defined PLATFORM_WINDOWS
#include "o2/Render/Windows/ShaderBase.h"
#elif defined PLATFORM_ANDROID
// #include "o2/Render/Android/ShaderBase.h"
#elif defined PLATFORM_MAC
// #include "o2/Render/Mac/ShaderBase.h"
#elif defined PLATFORM_IOS
// #include "o2/Render/iOS/ShaderBase.h"
#elif defined(PLATFORM_LINUX)
// #include "o2/Render/Linux/ShaderBase.h"
#endif

#include "o2/Utils/Property.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Render;
    class Material;

    // -----------------------------------------------------------------
    // Shader render primitive. Represents a single GPU shader stage
    // (vertex or fragment). Holds source code and a compiled GL handle.
    // -----------------------------------------------------------------
    class Shader : public ShaderBase, public RefCounterable
    {
    public:
        // Shader stage type
        enum class Type { Vertex, Fragment };

    public:
        PROPERTIES(Shader);
        GETTER(Type, shaderType, GetShaderType);   // Shader stage type getter
        GETTER(String, source, GetSource);         // Source code getter
        GETTER(String, fileName, GetFileName);     // File name getter
        GETTER(bool, ready, IsReady);              // Ready state getter

    public:
        // Default constructor
        Shader();

        // Constructor that compiles from source
        Shader(const String& source, Type type);

        // Copy-constructor, recompiles from source
        Shader(const Shader& other);

        // Destructor, releases GPU resources
        ~Shader();

        // Compiles the shader from GLSL source string. Returns true on success
        bool Compile(const String& source, Type type);

        // Returns true when shader is compiled and ready for use
        bool IsReady() const;

        // Returns shader stage type (vertex or fragment)
        Type GetShaderType() const;

        // Returns shader GLSL source code
        const String& GetSource() const;

        // Returns shader file name (set when loaded from file)
        const String& GetFileName() const;

        // Sets shader file name for identification
        void SetFileName(const String& fileName);

    protected:
        Type   mType = Type::Vertex; // Shader stage type
        String mSource;              // GLSL source code
        String mFileName;            // Source file name for logging
        bool   mReady = false;       // True when compiled successfully

    protected:
        // Called after reference creation
        void PostRefConstruct();

        // Platform-specific compilation
        bool PlatformCompile(const String& source, Type type);

        // Platform-specific destruction
        void PlatformDestroy();

        friend class Render;
        friend class Material;
        friend class Ref<Shader>;
        FRIEND_REF_MAKE();
    };
}
// --- META ---

PRE_ENUM_META(o2::Shader::Type);
// --- END META ---
