#pragma once

#include "o2/Render/RenderDefs.h"
#include "o2/Render/Windows/OpenGL.h"

namespace o2
{
    class GLSLShader : public RefCounterable
    {
    public:
        bool LoadFromFile(const String& fileName);

        ShaderType GetShaderType() const { return mType; }
        GLuint GetId() const { return mId; }

    private:
        GLuint mId = 0;
        String mSource;
        ShaderType mType;
    };
}
