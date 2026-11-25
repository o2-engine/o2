#pragma once

#ifdef PLATFORM_IOS

namespace o2
{
    struct MTLShaderImpl;

    class ShaderBase
    {
        friend class Render;
        friend class Material;

    protected:
        MTLShaderImpl* mImpl = nullptr;
    };
}

#endif // PLATFORM_IOS
