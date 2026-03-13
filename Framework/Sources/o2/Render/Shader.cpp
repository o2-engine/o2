#include "o2/stdafx.h"
#include "Shader.h"

#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    Shader::Shader()
    {}

    Shader::Shader(const String& source, Type type)
    {
        Compile(source, type);
    }

    Shader::Shader(const Shader& other):
        mType(other.mType), mSource(other.mSource), mFileName(other.mFileName)
    {
        if (!mSource.IsEmpty())
            Compile(mSource, mType);
    }

    Shader::~Shader()
    {
        PlatformDestroy();
    }

    bool Shader::Compile(const String& source, Type type)
    {
        PlatformDestroy();

        mSource = source;
        mType = type;
        mReady = PlatformCompile(source, type);

        return mReady;
    }

    bool Shader::IsReady() const
    {
        return mReady;
    }

    Shader::Type Shader::GetShaderType() const
    {
        return mType;
    }

    const String& Shader::GetSource() const
    {
        return mSource;
    }

    const String& Shader::GetFileName() const
    {
        return mFileName;
    }

    void Shader::SetFileName(const String& fileName)
    {
        mFileName = fileName;
    }

    void Shader::PostRefConstruct()
    {}
}
// --- META ---

ENUM_META(o2::Shader::Type)
{
    ENUM_ENTRY(Fragment);
    ENUM_ENTRY(Vertex);
}
END_ENUM_META;
// --- END META ---
