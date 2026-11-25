#pragma once

#ifdef PLATFORM_MAC

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

#endif // PLATFORM_MAC
