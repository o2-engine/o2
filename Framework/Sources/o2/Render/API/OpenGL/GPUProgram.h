#pragma once
#include "GLSLShader.h"
#include "o2/Utils/Types/WeakRef.h"

namespace o2
{
    class GPUProgram : public RefCounterable
    {
    public:
        GPUProgram(Ref<GLSLShader> vertexShader, Ref<GLSLShader> fragmentShader);

        bool Link();
        void Release();

        bool HasParameter(const String& name) const;
        GLuint GetParameter(const String& name) const;

        GLuint GetId() const { return mId; }

    private:
        void UpdateParameters();

    private:
        GLuint mId;

        WeakRef<GLSLShader> mVertexShader;
        WeakRef<GLSLShader> mFragmentShader;

        std::unordered_map<String, GLuint> mParameters;
    };
}
